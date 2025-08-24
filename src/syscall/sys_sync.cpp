// Hamster fsync and fdatasync system calls

#include <syscall/syscall.hpp>
#include <process/scheduler.hpp>
#include <filesystem/vfs.hpp>
#include <errno/errno.h>

namespace Hamster
{
    int32_t sys_fsync(int32_t fd)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        int vfs_fd = current_task->get_vfs_fd(fd);
        if (vfs_fd < 0)
            return cvt_error();

        if (vfs.sync(vfs_fd) < 0)
            return cvt_error();

        return 0;
    }

    int32_t sys_fdatasync(int32_t fd)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        int vfs_fd = current_task->get_vfs_fd(fd);
        if (vfs_fd < 0)
            return cvt_error();

        if (vfs.datasync(vfs_fd) < 0)
            return cvt_error();

        return 0;
    }
} // namespace Hamster

