// Hamster process ELF loading and script execution

#include <process/task.hpp>
#include <process/scheduler.hpp>
#include <elf/elf_loader.hpp>
#include <memory/memory_space.hpp>
#include <abi/values.hpp>
#include <errno/errno.h>
#include <cstring>
#include <elf.h>

namespace Hamster
{
    namespace
    {
        int push_stack(MemorySpace &mem_sp, uint64_t &sp, uint32_t data)
        {
            if (sp < sizeof(uint32_t))
            {
                // Not enough space on stack
                return -1;
            }

            sp -= sizeof(uint32_t);
            if (mem_sp.memcpy_alloc(sp, &data, sizeof(uint32_t)) != 0)
            {
                // Memory copy failed
                return -1;
            }
            return 0;
        }

        void push_strings(MemorySpace &mem_sp, uint64_t &sp, const char *const *strings, Deque<uint64_t> *locs = nullptr)
        {

            size_t count = 0;
            for (const char *const *it = strings; *it; ++it)
                ++count;
            size_t it = count;
            while (it-- > 0)
            {
                const char *str = strings[it];
                size_t len = strlen(str) + 1;
                if (sp < len)
                    return; // OOM
                sp -= len;
                if (mem_sp.memcpy_alloc(sp, str, len) < 0)
                    return; // fail
                if (locs)
                    locs->push_back(sp);
            }
        }

        void push_auxv(MemorySpace &mem_sp, uint64_t &sp, Elf32_auxv_t *ents, size_t count)
        {
            for (size_t it = 0; it < count; ++it)
            {
                Elf32_auxv_t &ent = ents[it];

                // push val first
                if (push_stack(mem_sp, sp, ent.a_type) < 0 ||
                    push_stack(mem_sp, sp, ent.a_un.a_val) < 0)
                    return; // fail
            }
        }
    } // namespace

    int Process::exec_elf(const char *path, const char *const *argv, const char *const *envp, int dirfd)
    {
        if (!path)
        {
            error = EINVAL;
            return -1;
        }

        // Open file

        // Note: its fine if this fails, file would be replaced anyway
        File file{vfs.dup(dirfd)};

        if (file.openat_replace(path, OPEN_RDONLY) < 0)
            return -1;
        
        Task *leader = nullptr;

        // Kill all threads, except the thread group leader
        for (Task *task : tasks)
        {
            if (task->tid != pid)
            {
                task->exit(make_wait_terminated(H_SIGKILL));
            }
            else
            {
                leader = task;
            }
        }
        tasks.clear();
        tasks.push_back(leader);

        assert(leader != nullptr);

        leader->is_dead = false;
        leader->is_paused = false;

        MemorySpace &memory_space = leader->memory->obj.memory;

        uint64_t entry_point = 0;
        uint64_t ph_num = 0;
        uint64_t brk = 0;
        if (Hamster::load_elf(file, memory_space, entry_point, ph_num, brk) < 0)
        {
            return -1;
        }

        leader->program_brk->obj = brk;

        // Load stack
        uint64_t sp = HAMSTER_STACK_TOP;

        // map in a 1 MB stack
        if (memory_space.map_anonymous(sp - 1024 * 1024, 1024 * 1024, PERM_READ | PERM_WRITE) < 0)
        {
            return -1;
        }

        // some zeros
        push_stack(memory_space, sp, 0);
        push_stack(memory_space, sp, 0);


        const char *exec_fn[]{path, nullptr};

        push_strings(memory_space, sp, exec_fn);

        uint64_t exec_fn_loc = sp;

        Deque<uint64_t> envp_locs;

        push_strings(memory_space, sp, envp, &envp_locs);

        Deque<uint64_t> argv_locs;
        push_strings(memory_space, sp, argv, &argv_locs);

        // Pad to 16B boundary
        sp &= ~0xFU;

        const char *arch[]{"riscv32", nullptr};

        push_strings(memory_space, sp, arch);

        uint64_t arch_loc = sp;

        size_t random_data_size = sp - (sp & ~0xFU);

        sp -= random_data_size;

        for (size_t i = 0; i < random_data_size; ++i)
            memory_space.memset_alloc(sp + i, (uint8_t)(rand() % 256), 1);

        uint64_t random_data_loc = sp;

        Elf32_auxv_t auxv[]{
            {AT_NULL, 0},
            {AT_PLATFORM, (uint32_t)arch_loc},
            {
                AT_EXECFN,
                (uint32_t)exec_fn_loc,
            },
            {AT_RANDOM, (uint32_t)random_data_loc},
            {AT_SECURE, 0},
            {AT_EGID, egid},
            {AT_GID, gid},
            {AT_EUID, euid},
            {AT_UID, uid},
            {AT_ENTRY, (uint32_t)entry_point},
            {AT_FLAGS, 0},
            {AT_PHNUM, (uint32_t)ph_num},
            {AT_PHENT, sizeof(Elf32_Phdr)},

            // The ELF loader loads the program headers here
            {AT_PHDR, HAMSTER_STACK_TOP + 1},
            {AT_CLKTCK, 100},
            {AT_PAGESZ, HAMSTER_PAGE_SIZE},
            {AT_HWCAP, 4393}, // RISC-V RV32IMAFD
        };

        push_auxv(memory_space, sp, auxv, sizeof(auxv) / sizeof(Elf32_auxv_t));

        push_stack(memory_space, sp, 0);

        for (uint64_t &val : envp_locs)
            push_stack(memory_space, sp, val);

        push_stack(memory_space, sp, 0);

        for (uint64_t &val : argv_locs)
            push_stack(memory_space, sp, val);

        // Get the number of argv
        size_t argc = 0;
        for (const char *const *it = argv; *it; ++it)
            ++argc;

        push_stack(memory_space, sp, argc);

        leader->emulator.pc = entry_point;
        leader->emulator.x[2] = sp;
        leader->emulator.x[1] = 0; // Set return address to 0 (no return)
        return 0;
    }

    int Process::exec(const char *path, const char *const *argv, const char *const *envp, int dirfd)
    {
        // Either run a script or an executable

        // Note: its fine if this fails, file would be replaced anyway
        File file{vfs.dup(dirfd)};

        if (file.openat_replace(path, OPEN_RDONLY) < 0)
            return -1;

        if (file.seek(0, H_SEEK_SET) < 0)
        {
            return -1;
        }

        // Read the first few bytes to determine if it's a script or an ELF file
        char magic[4];
        if (file.read(magic, sizeof(magic)) != sizeof(magic))
        {
            return -1;
        }

        if (memcmp(magic, "#!", 2) == 0)
        {
            String interpreter;

            interpreter.reserve(32);

            if (file.seek(1, H_SEEK_SET) < 0)
                return -1;
            
            char c;

            // Skip leading whitespace
            while (file.read(&c, 1) == 1 && (c == ' ' || c == '\t'))
                ;

            while (file.read(&c, 1) == 1)
            {
                if (isspace(c))
                    break;
                interpreter.push_back(c);
            }

            String arg;

            arg.reserve(4);

            // Skip more whitespace
            while ((c == ' ' || c == '\t') && file.read(&c, 1) == 1)
                ;
            if (!isspace(c))
                while (file.read(&c, 1) == 1)
                {
                    if (isspace(c))
                        break;
                    arg.push_back(c);
                }

            bool has_arg = !arg.empty();
            if (interpreter.empty())
            {
                error = ENOEXEC;
                return -1;
            }

            // Generate argv for the interpreter

            Vector<const char *> interp_argv;
            interp_argv.push_back(interpreter.c_str());
            if (has_arg)
            {
                interp_argv.push_back(arg.c_str());
            }
            interp_argv.push_back(path);
            interp_argv.push_back(nullptr);

            return exec_elf(interpreter.c_str(), interp_argv.data(), envp);
        }
        else if (memcmp(magic, ELFMAG, SELFMAG) == 0)
        {
            // It's an ELF file, so we can use the exec_elf function
            return exec_elf(path, argv, envp, dirfd);
        }
        else
        {
            // Not a script or ELF file
            error = ENOEXEC;
            return -1;
        }
    }
} // namespace Hamster

