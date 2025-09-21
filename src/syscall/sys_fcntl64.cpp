// Hamster fcntl64 system call

#include <syscall/syscall.hpp>
#include <process/scheduler.hpp>
#include <filesystem/vfs.hpp>
#include <abi/values.hpp>
#include <errno/errno.h>

namespace Hamster
{
    int32_t sys_fcntl64(int32_t fd, int32_t cmd, uint32_t arg)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        UserFD *user_fd = current_task->get_user_fd(fd);
        if (!user_fd)
            return cvt_error();

        switch (cmd)
        {
        case FILE_DUPFD:
        case FILE_DUPFD_CLOEXEC:
        {
            uint32_t new_index = current_task->get_unused_fd_index(arg);
            return sys_dup3(fd, new_index, cmd == FILE_DUPFD_CLOEXEC ? OPEN_CLOEXEC : 0);
        }
        case FILE_SETFD:
        {
            user_fd->flags = arg;
            return 0;
        }
        case FILE_GETFD:
            return user_fd->flags;
        case FILE_GETFL:
        {
            int flags = 0;
            switch (user_fd->type)
            {
            case UserFDType::VFS:
            {
                int vfs_fd = user_fd->vfs_fd;
                if (vfs_fd < 0)
                    return -H_EBADF; // Closed or invalid file descriptor
                flags = vfs.get_flags(vfs_fd);
                if (flags < 0)
                    return cvt_error();
                return flags;
            }
            case UserFDType::PIPE_WRITE:
                flags = OPEN_WRONLY;
                [[fallthrough]];
            case UserFDType::PIPE_READ:
                // Note: OPEN_RDONLY == 0

                if (user_fd->flags & USER_FD_PIPE_NONBLOCK)
                    flags |= OPEN_NONBLOCK;
                return flags;
            default:
                return -H_EBADF;
            }
        }
        case FILE_SETFL:
        {
            constexpr int changeable_flags = OPEN_APPEND | OPEN_NONBLOCK;
            switch (user_fd->type)
            {
            case UserFDType::VFS:
            {
                int vfs_fd = user_fd->vfs_fd;
                if (vfs_fd < 0)
                    return -H_EBADF; // Closed or invalid file descriptor
                int old_flags = vfs.get_flags(vfs_fd);
                if (old_flags < 0)
                    return cvt_error();
                int new_flags = (old_flags & ~changeable_flags) | (arg & changeable_flags);
                if (vfs.set_flags(vfs_fd, new_flags) < 0)
                    return cvt_error();
                return 0;
            }
            case UserFDType::PIPE_READ:
            case UserFDType::PIPE_WRITE:
                if (arg & OPEN_NONBLOCK)
                    user_fd->flags |= USER_FD_PIPE_NONBLOCK;
                else
                    user_fd->flags &= ~USER_FD_PIPE_NONBLOCK;
                return 0;
            default:
                return -H_EBADF;
            }
        }
        default:
            // Unsupported command
            error = H_ENOSYS;
            return -1;
        }
    }
} // namespace Hamster
