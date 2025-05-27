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

        return 0;
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

