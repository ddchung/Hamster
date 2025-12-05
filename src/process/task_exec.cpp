
#include <process/task.hpp>
#include <filesystem/vfs.hpp>
#include <elf/elf_loader.hpp>
#include <elf.h>
#include <cassert>

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
                if (push_stack(mem_sp, sp, ent.a_un.a_val) < 0 ||
                    push_stack(mem_sp, sp, ent.a_type) < 0)
                    return; // fail
            }
        }
    } // namespace

    Task *spawn(const char *path, const char *const *argv, const char *const *envp)
    {
        int fd = vfs.open(path, OPEN_RDONLY);
        if (fd < 0)
            return nullptr;
        Task *res = Task::create_task(fd, argv, envp);
        vfs.close(fd);
        return res;
    }

    int Task::exec(int fd, const char *const *argv, const char *const *envp)
    {
        close_cloexec_fds();
        if (is_vfork && parent)
            parent->interrupt_block();
        return process->exec(fd, this, argv, envp);
    }

    int Process::exec(int fd, Task *new_leader, const char *const *argv, const char *const *envp)
    {
        static const char *empty[] = {nullptr};

        if (!argv)
            argv = empty;
        if (!envp)
            envp = empty;

        assert(tasks.size() > 0);

        if (vfs.seek(fd, 0, H_SEEK_SET) != 0)
            return -1;

        int64_t size = vfs.size(fd);
        if (size < 0)
            return -1;
        
        if (!tasks.contains(new_leader))
        {
            error = H_ESRCH;
            return -1;
        }

        leader = new_leader;

        // Erase all other tasks
        for (auto it = tasks.begin(); it != tasks.end();)
        {
            if (*it != leader)
                // This also removes the task
                (*it++)->exit(0);
            else
                ++it;
        }

        assert(tasks.size() == 1);
        assert(*tasks.begin() == leader);

        return leader->load_executable(fd, argv, envp);
    }

    int Task::load_executable(int fd, const char *const *argv, const char *const *envp)
    {
        // TODO: Execute scripts (#!)
        // TODO: Handle setuid/setgid

        auto &memory = this->memory->ms;

        // Clear all instruction caches
        emulator.flush_caches();

        uint64_t entry_point = 0, ph_num = 0, brk = 0, ph_loc = 0;
        if (load_elf(fd, memory, entry_point, ph_num, brk, ph_loc) < 0)
            return -1;
        
        this->memory->brk = brk;

        emulator.pc = entry_point;
        ::memset(emulator.x, 0, sizeof(emulator.x));

        // Load stack

        uint64_t sp = HAMSTER_STACK_TOP;

        if (memory.map_anonymous(sp - HAMSTER_STACK_SIZE, HAMSTER_STACK_SIZE, PERM_READ | PERM_WRITE | PERM_EXEC) < 0)
            return -1;

        // some zero padding
        push_stack(memory, sp, 0);
        push_stack(memory, sp, 0);

        // TODO: exec_fn
        const char *exec_fn[] {"program", nullptr};

        push_strings(memory, sp, exec_fn);

        uint64_t exec_fn_loc = sp;

        Deque<uint64_t> envp_locs, argv_locs;

        push_strings(memory, sp, envp, &envp_locs);
        push_strings(memory, sp, argv, &argv_locs);

        // Pad
        sp &= ~0xFUL;

        const char *arch[]{"riscv32", nullptr};

        push_strings(memory, sp, arch);

        uint64_t arch_loc = sp;

        size_t random_data_size = sp - (sp & ~0xFU);

        sp -= random_data_size;

        for (size_t i = 0; i < random_data_size; ++i)
            memory.memset_alloc(sp + i, (uint8_t)(rand() % 256), 1);

        uint64_t random_data_loc = sp;

        int uid, euid, gid, egid;
        get_uid(&uid, &euid, nullptr);
        get_gid(&gid, &egid, nullptr);

        Elf32_auxv_t auxv[]{
            {AT_NULL, 0},
            {AT_PLATFORM, (uint32_t)arch_loc},
            {
                AT_EXECFN,
                (uint32_t)exec_fn_loc,
            },
            {AT_RANDOM, (uint32_t)random_data_loc},
            {AT_SECURE, 0},
            {AT_EGID, (uint32_t)egid},
            {AT_GID, (uint32_t)gid},
            {AT_EUID, (uint32_t)euid},
            {AT_UID, (uint32_t)uid},
            {AT_ENTRY, (uint32_t)entry_point},
            {AT_FLAGS, 0},
            {AT_PHNUM, (uint32_t)ph_num},
            {AT_PHENT, sizeof(Elf32_Phdr)},
            {AT_PHDR, (uint32_t)ph_loc},
            {AT_CLKTCK, 100},
            {AT_PAGESZ, HAMSTER_PAGE_SIZE},
            {AT_HWCAP, 4393}, // RISC-V RV32IMAFD
        };

        push_auxv(memory, sp, auxv, sizeof(auxv) / sizeof(Elf32_auxv_t));

        push_stack(memory, sp, 0);

        for (uint64_t &val : envp_locs)
            push_stack(memory, sp, val);

        push_stack(memory, sp, 0);

        for (uint64_t &val : argv_locs)
            push_stack(memory, sp, val);

        // Get the number of argv
        size_t argc = 0;
        for (const char *const *it = argv; *it; ++it)
            ++argc;

        push_stack(memory, sp, argc);

        emulator.x[2] = sp;
        emulator.x[1] = 0; // Set return address to 0 (no return)
        // Set mmap allocation start to the 1/4 point of the free space
        // 
        // Before
        // [ program ] [ free space > < stack ]
        //
        // After
        // [ program ] [ brk space ] [ mmap allocation >*< stack ]
        //                                              *
        //                            sliding  boundary *
        memory.set_next_mmap((brk + (HAMSTER_STACK_TOP - brk) / 4) & ~(HAMSTER_PAGE_SIZE - 1));

        return 0;
    }
} // namespace Hamster
