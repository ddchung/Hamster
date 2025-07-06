// Hamster process

#include <process/process.hpp>
#include <filesystem/vfs.hpp>
#include <elf/elf_loader.hpp>
#include <memory/stl_sequential.hpp>
#include <memory/stl_map.hpp>
#include <memory/allocator.hpp>
#include <errno/errno.h>
#include <fcntl.h>
#include <elf.h>
#include <string.h>
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

    int Process::load_elf(const char *path, const char *const *argv, const char *const *envp, int dirfd)
    {
        static const char *empty[] = {0};
        if (!argv)
            argv = empty;
        if (!envp)
            envp = empty;
        if (!path)
        {
            error = EINVAL;
            return -1;
        }

        int fd;
        if (dirfd >= 0)
            fd = vfs.openat(dirfd, path, OPEN_RDONLY);
        else
            fd = vfs.open(path, OPEN_RDONLY);
        
        if (fd < 0)
            return -1;
        uint64_t entry_point = 0;
        uint64_t ph_num = 0;
        if (Hamster::load_elf(fd, memory_space, entry_point, ph_num) < 0)
        {
            error = EIO;
            return -1;
        }

        // Load stack
        uint64_t sp = HAMSTER_STACK_TOP;

        // some zeros
        push_stack(memory_space, sp, 0);
        push_stack(memory_space, sp, 0);

        const char *exec_fn[]{path, nullptr};

        push_strings(memory_space, sp, exec_fn);

        uint64_t exec_fn_loc = sp;

        Deque<uint64_t> envp_locs;

        push_strings(memory_space, sp, envp, &envp_locs);

        Deque<uint64_t> argv_locs;

        const char *last = strrchr(path, '/');
        if (!last)
            last = path;
        else
            ++last;

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

        for (auto &thread : threads)
            thread.set_state(ThreadState::ENDED);

        Thread &t = threads.emplace_back(this, 0);
        t.set_pc(entry_point);
        t.get_regs()[2] = sp;
        t.get_regs()[1] = 0; // Set return address to 0 (no return)

        // uncomment to open console if no fds are set
        // Note that this is not POSIX compliant, so if you're running an init system,
        // let it open the console for userland.
        
        if (fds.empty())
        {
            int console_fd = vfs.open("/dev/console", O_RDWR);
            fd_refcount[console_fd] = 3; 
            fds.push_back({console_fd, 0}); // stdin
            fds.push_back({console_fd, 0}); // stdout
            fds.push_back({console_fd, 0}); // stderr
        }

        uint32_t tp = 0xFFFF0000;
        uint32_t dtv = 0xFFFE0000;
        uint32_t tls = 0xFFFC0000;

        t.get_regs()[3] = tp; // set tp
        uint32_t i = 1;

        memory_space.memcpy(tp, &dtv, 4); // *(uint32_t*)tp = dtv
        memory_space.memcpy(dtv, &i, 4);       // dtv[0] = 1
        memory_space.memcpy(dtv + 4, &tls, 4);               // dtv[1] = tls_base
        memory_space.memset(tls, 0, HAMSTER_PAGE_SIZE);      // init TLS memory
        return 0;
    }

    Process::Process(const Process &other)
        : memory_space(other.memory_space), reserved_mem(other.reserved_mem), threads(other.threads),
          fds(other.fds), cwd(other.cwd), pid(other.pid), ppid(other.ppid), pgid(other.pgid), sid(other.sid),
          uid(other.uid), gid(other.gid), euid(other.euid), egid(other.egid), exit_status(other.exit_status)
    {
        // Set all the thread's process to this
        for (Thread &thread : threads)
        {
            thread.set_process(this);
        }

        // Increment file descriptor reference counts
        for (const auto &[fd, _] : fds)
        {
            if (fd < 0)
                continue; // Skip invalid file descriptors
            
            fd_refcount[fd]++;
        }
    }

    Process &Process::operator=(const Process &other)
    {
        if (this == &other)
            return *this;

        memory_space = other.memory_space;
        reserved_mem = other.reserved_mem;
        threads = other.threads;
        fds = other.fds;
        cwd = other.cwd;
        pid = other.pid;
        ppid = other.ppid;
        pgid = other.pgid;
        sid = other.sid;
        uid = other.uid;
        gid = other.gid;
        euid = other.euid;
        egid = other.egid;
        exit_status = other.exit_status;

        // Set all the thread's process to this
        for (Thread &thread : threads)
        {
            thread.set_process(this);
        }

        // Increment file descriptor reference counts
        for (const auto &[fd, _] : fds)
        {
            if (fd < 0)
                continue; // Skip invalid file descriptors
            
            fd_refcount[fd]++;
        }

        return *this;
    }

    Process::~Process()
    {
        for (auto [fd, _] : fds)
        {
            if (fd < 0)
                continue; // Skip invalid file descriptors
            
            auto it = fd_refcount.find(fd);
            assert(it != fd_refcount.end());
            if (--it->second == 0)
            {
                vfs.close(fd);
                fd_refcount.erase(it);
            }
        }
        fds.clear();
    }
} // namespace Hamster
