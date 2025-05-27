// Hamster process

#include <process/process.hpp>
#include <filesystem/vfs.hpp>
#include <errno/errno.h>
#include <fcntl.h>

namespace Hamster
{
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

