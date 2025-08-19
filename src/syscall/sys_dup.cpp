// Hamster dup system calls

#include <syscall/syscall.hpp>
#include <process/scheduler.hpp>
#include <filesystem/vfs.hpp>
#include <abi/values.hpp>
#include <errno/errno.h>

namespace Hamster
{
    int32_t sys_dup(int32_t fd)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        UserFD *user_fd = current_task->get_user_fd(fd);
        if (!user_fd)
            return cvt_error();

        uint32_t new_index = current_task->get_unused_fd_index();

        UserFD *new_fd = current_task->get_user_fd(new_index);
        assert(new_fd != nullptr);

        *new_fd = *user_fd;

        switch (user_fd->type)
        {
        case UserFDType::VFS:
            ++fd_refcount[user_fd->vfs_fd];
            break;
        case UserFDType::PIPE_READ:
            ++user_fd->pipe->readers;
            break;
        case UserFDType::PIPE_WRITE:
            ++user_fd->pipe->writers;
            break;
        default:
            break;
        }
        
        // Clear FD_CLOEXEC
        new_fd->flags &= ~H_FD_CLOEXEC;

        return new_index;
    }

    int32_t sys_dup3(int32_t oldfd, int32_t newfd, int32_t flags)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        if (oldfd == newfd)
            return -EINVAL;

        UserFD *old_user_fd = current_task->get_user_fd(oldfd);
        if (!old_user_fd)
            return cvt_error();
        
        // Silently close it, ignoring any errors
        sys_close(newfd);

        UserFD *new_user_fd = current_task->get_user_fd(newfd);
        if (!new_user_fd)
            return cvt_error();
        *new_user_fd = *old_user_fd;

        switch (old_user_fd->type)
        {
        case UserFDType::VFS:
            ++fd_refcount[old_user_fd->vfs_fd];
            break;
        case UserFDType::PIPE_READ:
            ++old_user_fd->pipe->readers;
            break;
        case UserFDType::PIPE_WRITE:
            ++old_user_fd->pipe->writers;
            break;
        default:
            break;
        }
        
        new_user_fd->flags &= ~H_FD_CLOEXEC; // Clear FD_CLOEXEC
        if (flags & OPEN_CLOEXEC) // Note: OPEN_CLOEXEC isn't FD_CLOEXEC!
            new_user_fd->flags |= H_FD_CLOEXEC; // Set FD_CLOEXEC if requested
        
        return newfd;
    }
} // namespace Hamster

