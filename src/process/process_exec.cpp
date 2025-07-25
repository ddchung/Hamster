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
            if (mem_sp.memcpy(sp, &data, sizeof(uint32_t)) != 0)
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
                if (mem_sp.memcpy(sp, str, len) < 0)
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

    int Process::exec_elf(File file, const char *const *argv, const char *const *envp)
    {
        // Kill all threads, except the thread group leader
        for (uint32_t tid : tasks)
        {
            if (tid != pid)
            {
                Task *t = scheduler.get_task(tid);
                if (t)
                {
                    t->exit(make_wait_terminated(H_SIGKILL));
                }
            }
        }
        tasks.clear();
        tasks.push_back(pid);

        Task *leader = scheduler.get_task(pid);

        if (!leader)
        {
            return -1;
        }

        MemorySpace &memory_space = leader->memory->obj.memory;

        uint64_t entry_point = 0;
        uint64_t ph_num = 0;
        uint64_t brk = 0;
        if (Hamster::load_elf(file, memory_space, entry_point, ph_num, brk) < 0)
        {
            return -1;
        }

        this->brk = brk;

        // Load stack
        uint64_t sp = HAMSTER_STACK_TOP;

        // some zeros
        push_stack(memory_space, sp, 0);
        push_stack(memory_space, sp, 0);


        const char *exec_fn[2]{nullptr, nullptr};

        if (argv && argv[0])
        {
            exec_fn[0] = argv[0];
        }
        else
        {
            exec_fn[0] = "(Kernel): exec_fn without argv not implemented yet";
        }

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
            memory_space.write_byte(sp + i, (uint8_t)(rand() % 256));

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

    int Process::exec(File file, const char *const *argv, const char *const *envp)
    {
        // Either run a script or an executable

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
            // TODO: Handle script execution
            // For now, we just return an error
            error = ENOEXEC;
            return -1;
        }
        else if (memcmp(magic, ELFMAG, SELFMAG) == 0)
        {
            // It's an ELF file, so we can use the exec_elf function
            return exec_elf(file, argv, envp);
        }
        else
        {
            // Not a script or ELF file
            error = ENOEXEC;
            return -1;
        }
    }
} // namespace Hamster

