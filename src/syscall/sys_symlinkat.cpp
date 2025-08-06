// Hamster symlinkat system call

#include <syscall/syscall.hpp>
#include <process/scheduler.hpp>
#include <memory/memory_space.hpp>
#include <errno/errno.h>

namespace Hamster
{
    int32_t sys_symlinkat(uint32_t target_loc, int32_t thread_dfd, uint32_t linkpath_loc)
    {
        Task *task = scheduler.get_current_task();
        assert(task != nullptr);

        // Get the target path
        char *target_str = task->memory->obj.memory.get_string(target_loc);
        if (!target_str)
        {
            error = EFAULT;
            return cvt_error();
        }

        // Get the link path
        char *linkpath_str = task->memory->obj.memory.get_string(linkpath_loc);
        if (!linkpath_str)
        {
            dealloc(target_str);
            error = EFAULT;
            return cvt_error();
        }

        int vfs_rel_fd = task->get_relative_fd(linkpath_str, thread_dfd);
        if (vfs_rel_fd < 0)
        {
            dealloc(target_str);
            dealloc(linkpath_str);
            return cvt_error();
        }

        // Create the symlink
        int ret = vfs.symlinkat(vfs_rel_fd, linkpath_str, target_str);
        dealloc(target_str);
        dealloc(linkpath_str);
        vfs.close(vfs_rel_fd);

        if (ret < 0)
            return cvt_error();
        return 0; // Success
    }
} // namespace Hamster

