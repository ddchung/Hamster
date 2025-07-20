// Hamster fcntl syscall implementation

#include <syscall/syscall.hpp>
#include <abi/values.hpp>
#include <filesystem/vfs.hpp>
#include <process/process.hpp>
#include <errno/errno.h>

namespace Hamster
{
    int sys_fcntl(Thread &thread)
    {
        int32_t thread_fd = get_arg(thread, 0);
        int32_t cmd = get_arg(thread, 1);
        uint32_t arg = get_arg(thread, 2);

        Process *process = thread.get_process();

        switch (cmd)
        {
        case FILE_DUPFD:
        case FILE_DUPFD_CLOEXEC:
        {
            int vfs_fd = deref_fd(thread, thread_fd);
            if (vfs_fd < 0)
            {
                error = EBADF;
                return transfer_error(thread);
            }

            if ((int32_t)arg < 0)
            {
                error = EINVAL;
                return transfer_error(thread);
            }

            fd_refcount[vfs_fd]++;

            for (size_t it = arg; it < process->fds.size(); ++it)
            {
                if (process->fds[it].fd < 0)
                {
                    // Found empty slot
                    process->fds[it].fd = vfs_fd;
                    
                    // 0x0 or 0x1 for FILE_DUPFD and FILE_DUPFD_CLOEXEC respectively
                    process->fds[it].fd_flags = (cmd == FILE_DUPFD_CLOEXEC);
                    return set_return(thread, it);
                }
            }

            // No empty slot found, allocate a new one
            process->fds.push_back({vfs_fd, (cmd == FILE_DUPFD_CLOEXEC)});
            return set_return(thread, process->fds.size() - 1);
        }
        case FILE_GETFD:
        {
            if (thread_fd < 0 || thread_fd >= (int32_t)process->fds.size())
            {
                error = EBADF;
                return transfer_error(thread);
            }
            return set_return(thread, process->fds[thread_fd].fd_flags);
        }
        case FILE_SETFD:
        {
            if (thread_fd < 0 || thread_fd >= (int32_t)process->fds.size())
            {
                error = EBADF;
                return transfer_error(thread);
            }
            process->fds[thread_fd].fd_flags = arg;
            return set_return(thread, 0);
        }
        case FILE_GETFL:
        {
            int vfs_fd = deref_fd(thread, thread_fd);
            if (vfs_fd < 0)
            {
                error = EBADF;
                return transfer_error(thread);
            }

            int flags = vfs.get_flags(vfs_fd);
            if (flags == -1)
            {
                error = EBADF;
                return transfer_error(thread);
            }

            return set_return(thread, flags);
        }
        case FILE_SETFL:
        {
            int vfs_fd = deref_fd(thread, thread_fd);
            if (vfs_fd < 0)
            {
                error = EBADF;
                return transfer_error(thread);
            }

            int old_flags = vfs.get_flags(vfs_fd);

            if (old_flags == -1)
            {
                error = EBADF;
                return transfer_error(thread);
            }

            constexpr int changeable_flags = OPEN_APPEND | OPEN_NONBLOCK | OPEN_ASYNC;

            old_flags &= ~changeable_flags; // Clear changeable flags
            old_flags |= (arg & changeable_flags); // Set new changeable flags

            if (vfs.set_flags(vfs_fd, old_flags) == -1)
            {
                error = EBADF;
                return transfer_error(thread);
            }

            return set_return(thread, 0);
        }
        default:
        {
            error = ENOTSUP;
            return transfer_error(thread);
        }
        }
    }
} // namespace Hamster

