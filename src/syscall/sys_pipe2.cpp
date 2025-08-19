// Hamster pipe2 system call

#include <syscall/syscall.hpp>
#include <process/scheduler.hpp>
#include <memory/allocator.hpp>
#include <errno/errno.h>

namespace Hamster
{
    int32_t sys_pipe2(uint32_t pipefd_loc, int32_t flags)
    {
        if (flags & ~(OPEN_NONBLOCK | OPEN_CLOEXEC))
            return -ENOTSUP;

        if (!pipefd_loc)
            return -EFAULT;
        
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        UserFDPipe *pipe = alloc<UserFDPipe>();

        pipe->readers = 1;
        pipe->writers = 1;

        UserFD *user_fd;
        int unused_slots[2];
        int fd_flags = 0;

        if (flags & OPEN_CLOEXEC) fd_flags |= H_FD_CLOEXEC;
        if (flags & OPEN_NONBLOCK) fd_flags |= USER_FD_PIPE_NONBLOCK;

        unused_slots[0] = current_task->get_unused_fd_index();
        user_fd = current_task->get_user_fd(unused_slots[0]);
        if (!user_fd)
            return cvt_error();
        
        user_fd->type = UserFDType::PIPE_READ;
        user_fd->flags = fd_flags;
        user_fd->pipe = pipe;

        unused_slots[1] = current_task->get_unused_fd_index();
        user_fd = current_task->get_user_fd(unused_slots[1]);
        if (!user_fd)
            return cvt_error();

        user_fd->type = UserFDType::PIPE_WRITE;
        user_fd->flags = fd_flags;
        user_fd->pipe = pipe;

        // Copy to user

        if (current_task->copy_to_user(unused_slots, pipefd_loc))
            return -EFAULT;

        return 0;
    }
} // namespace Hamster
