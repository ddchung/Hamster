// Hamster process

#include <process/process.hpp>
#include <filesystem/vfs.hpp>
#include <elf/elf_loader.hpp>
#include <errno/errno.h>
#include <fcntl.h>
#include <elf.h>
#include <string.h>

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
    } // namespace
    
    int Process::load_elf(const char *path, const char *const *argv, const char *const *envp)
    {
        if (!path)
        {
            error = EINVAL;
            return -1;
        }

        int fd = vfs.open(path, O_RDONLY);
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
        uint64_t stack_top = HAMSTER_STACK_TOP;

        // auxv

        // Note that this is reversed, as it will be pushed onto the stack
        uint32_t auxv_data[] = {
            0, 0, // NULL terminator
            HAMSTER_PAGE_SIZE, 3, // AT_PAGESZ
            (uint32_t)ph_num, 19, // AT_PHNUM
            sizeof(Elf32_Phdr), 18, // AT_PHENT
            HAMSTER_STACK_TOP + 1, 17, // AT_PHDR
            euid, 12, // AT_EUID
            uid, 11, // AT_UID
        };

        for (uint32_t val : auxv_data)
        {
            if (push_stack(memory_space, stack_top, val) < 0)
            {
                error = ENOMEM;
                return -1;
            }
        }

        // envp

        // null terminator
        if (push_stack(memory_space, stack_top, 0) < 0)
        {
            error = ENOMEM;
            return -1;
        }

        if (envp)
        {
            for (const char *env = *envp; env; ++envp, env = *envp)
            {
                size_t len = strlen(env) + 1; // +1 for null terminator
                if (len > HAMSTER_STACK_TOP - stack_top)
                {
                    error = ENOMEM;
                    return -1;
                }
                if (memory_space.memcpy(stack_top - len, env, len) != 0)
                {
                    error = EIO;
                    return -1;
                }
                stack_top -= len;
            }
        }

        // argv

        // null terminator
        if (push_stack(memory_space, stack_top, 0) < 0)
        {
            error = ENOMEM;
            return -1;
        }

        // Push program name
        const char *prog_name = strrchr(path, '/');
        if (!prog_name)
            prog_name = path; // No '/' found, use the whole path
        else
            ++prog_name; // Skip the '/' character
        size_t prog_name_len = strlen(prog_name) + 1; // +1 for null terminator
        if (prog_name_len > HAMSTER_STACK_TOP - stack_top)
        {
            error = ENOMEM;
            return -1;
        }
        if (memory_space.memcpy(stack_top - prog_name_len, prog_name, prog_name_len) != 0)
        {
            error = EIO;
            return -1;
        }
        stack_top -= prog_name_len;

        if (argv)
        {
            for (const char *arg = *argv; arg; ++argv, arg = *argv)
            {
                size_t len = strlen(arg) + 1; // +1 for null terminator
                if (len > HAMSTER_STACK_TOP - stack_top)
                {
                    error = ENOMEM;
                    return -1;
                }
                if (memory_space.memcpy(stack_top - len, arg, len) != 0)
                {
                    error = EIO;
                    return -1;
                }
                stack_top -= len;
            }
        }

        threads.clear();

        Thread &t = threads.emplace_back(this, 0);
        t.set_pc(entry_point);
        t.get_regs()[2] = stack_top; // Set stack pointer
        t.get_regs()[1] = 0; // Set return address to 0 (no return)

        if (fds.empty())
        {
            for (int i = 0; i < 3; ++i)
            {
                int console_fd = vfs.open("/dev/console", O_RDWR);
                fds.push_back({console_fd, 0}); // Add stdin, stdout, stderr
            }
        }

        return 0;
    }

    Process::Process(const Process &other)
        : memory_space(other.memory_space), reserved_mem(other.reserved_mem), threads(other.threads),
          fds(other.fds), cwd(other.cwd), pid(other.pid), ppid(other.ppid), pgid(other.pgid), sid(other.sid),
          uid(other.uid), gid(other.gid), euid(other.euid), egid(other.egid), exit_code(other.exit_code)
    {
        // Set all the thread's process to this
        for (Thread &thread : threads)
        {
            thread.set_process(this);
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
        exit_code = other.exit_code;

        // Set all the thread's process to this
        for (Thread &thread : threads)
        {
            thread.set_process(this);
        }

        return *this;
    }

    Process::~Process()
    {
        for (auto [fd, _] : fds)
        {
            if (fd != -1)
                vfs.close(fd);
        }
        fds.clear();
    }
} // namespace Hamster

