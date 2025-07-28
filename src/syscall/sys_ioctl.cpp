// Hamster ioctl system call

#include <syscall/syscall.hpp>
#include <process/scheduler.hpp>
#include <filesystem/vfs.hpp>
#include <errno/errno.h>

namespace Hamster
{
    namespace
    {
        alignas(8) uint8_t IOCTL_BUF[512];
    } // namespace

    int32_t sys_ioctl(int32_t fd, int32_t request, uint32_t arg)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        int vfs_fd = current_task->get_vfs_fd(fd);
        if (vfs_fd < 0)
            return cvt_error();

        // Copy a potential pointer into the buffer
        if (arg != 0)
        {
            current_task->memory->obj.memory.memcpy(IOCTL_BUF, arg, sizeof(IOCTL_BUF));
        }

        IoctlArg ioctl_arg;
        ioctl_arg.i = arg;
        ioctl_arg.p = IOCTL_BUF;

        int result = vfs.ioctl(vfs_fd, request, ioctl_arg);

        // Copy the result back to userspace
        if (arg != 0)
        {
            current_task->memory->obj.memory.memcpy(arg, IOCTL_BUF, sizeof(IOCTL_BUF));
        }

        return result;
    }
    
} // namespace Hamster

