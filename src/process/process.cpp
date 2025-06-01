// Hamster process

#include <process/process.hpp>
#include <filesystem/vfs.hpp>
#include <elf/elf_loader.hpp>
#include <errno/errno.h>
#include <fcntl.h>

namespace Hamster
{
    int Process::load_elf(const char *path)
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
        if (Hamster::load_elf(fd, memory_space, entry_point) < 0)
        {
            error = EIO;
            return -1;
        }

        threads.clear();

        Thread &t = threads.emplace_back(this, 0);
        t.set_pc(entry_point);

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

