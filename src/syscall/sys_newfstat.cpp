// Hamster newfstat system call

#include <syscall/syscall.hpp>
#include <filesystem/vfs.hpp>
#include <process/scheduler.hpp>
#include <abi/structs.hpp>
#include <errno/errno.h>

namespace Hamster
{
    int32_t sys_newfstat(int32_t fd, uint32_t statbuf_loc)
    {
        if (statbuf_loc == 0)
        {
            error = EFAULT; // Bad address
            return -1;
        }

        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        int vfs_fd = current_task->get_vfs_fd(fd);
        if (vfs_fd < 0)
            return cvt_error();
        
        sys_stat statbuf;
        
        if (vfs.stat(vfs_fd, &statbuf) < 0)
            return cvt_error();

        // Copy the statbuf to userspace
        if (current_task->memory->obj.memory.memcpy_alloc(statbuf_loc, &statbuf, sizeof(statbuf)) < 0)
        {
            error = EFAULT; // Bad address
            return -1;
        }

        return 0;
    }
} // namespace Hamster


