// Hamster fchown system call

#include <syscall/syscall.hpp>
#include <process/scheduler.hpp>
#include <filesystem/vfs.hpp>
#include <errno/errno.h>

namespace Hamster
{
    int32_t sys_fchown(int32_t fd, int32_t uid, int32_t gid)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        int vfs_fd = current_task->get_vfs_fd(fd);
        if (vfs_fd < 0)
            return cvt_error();

        int res = vfs.chown(vfs_fd, uid, gid);

        if (res < 0)
            return cvt_error();
        return 0;
    }
} // namespace Hamster

