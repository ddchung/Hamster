// Hamster linkat system call

#include <syscall/syscall.hpp>
#include <process/scheduler.hpp>
#include <filesystem/vfs.hpp>
#include <errno/errno.h>
#include <abi/values.hpp>

namespace Hamster
{
    int32_t sys_linkat(int32_t olddirfd, uint32_t oldpath_loc, int32_t newdirfd, uint32_t newpath_loc, int flags)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        // We don't support any flags right now
        if (flags != 0)
            return -H_EINVAL;

        // Get the old path
        char *old_path_str = current_task->get_memory().get_string(oldpath_loc);
        if (old_path_str == nullptr)
        {
            error = H_EFAULT;
            return cvt_error();
        }

        // Get the new path
        char *new_path_str = current_task->get_memory().get_string(newpath_loc);
        if (new_path_str == nullptr)
        {
            dealloc(old_path_str);
            error = H_EFAULT;
            return cvt_error();
        }

        int vfs_old_rel_fd = current_task->get_relative_fd(old_path_str, olddirfd);
        if (vfs_old_rel_fd < 0)
        {
            dealloc(old_path_str);
            dealloc(new_path_str);
            return cvt_error();
        }

        int vfs_new_rel_fd = current_task->get_relative_fd(new_path_str, newdirfd);
        if (vfs_new_rel_fd < 0)
        {
            dealloc(old_path_str);
            dealloc(new_path_str);
            vfs.close(vfs_old_rel_fd);
            return cvt_error();
        }

        // Create the hard link
        int result = vfs.linkat(vfs_old_rel_fd, old_path_str, vfs_new_rel_fd, new_path_str);
        
        dealloc(old_path_str);
        dealloc(new_path_str);
        vfs.close(vfs_old_rel_fd);
        vfs.close(vfs_new_rel_fd);

        if (result < 0)
            return cvt_error();
        
        return 0;
    }
} // namespace Hamster

