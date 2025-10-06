// Hamster unlinkat system call

#include <syscall/syscall.hpp>
#include <process/scheduler.hpp>
#include <filesystem/vfs.hpp>
#include <errno/errno.h>

namespace Hamster
{
    int32_t sys_unlinkat(int32_t thread_dfd, uint32_t path_loc, int32_t flags)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        // Get the path
        char *path_str = current_task->get_memory().get_string(path_loc);
        if (path_str == nullptr)
        {
            error = H_EFAULT;
            return cvt_error();
        }

        int vfs_rel_fd = current_task->get_relative_fd(path_str, thread_dfd);
        if (vfs_rel_fd < 0)
        {
            dealloc(path_str);
            return cvt_error();
        }

        // Check if it's a directory
        sys_stat statbuf;
        int result = vfs.lstatat(vfs_rel_fd, path_str, &statbuf);
        if (result == 0)
        {
            int err = 0;
            if (is_directory(statbuf.mode) && (flags & H_AT_REMOVEDIR) == 0)
                err = H_EISDIR;
            else if (!is_directory(statbuf.mode) && (flags & H_AT_REMOVEDIR) != 0)
                err = H_ENOTDIR;
            
            if (err)
            {
                dealloc(path_str);
                vfs.close(vfs_rel_fd);
                return -err;
            }
        }
        else
        {
            _trace("WARN: sys_unlinkat: lstatat failed for path '%s' with unexpected error %d, return value %d\n",
                   path_str, error, result);
            _trace("WARN: sys_unlinkat: continuing anyway with remove operation..\n");
            _trace("WARN: sys_unlinkat: note: from %s:%d\n", __FILE__, __LINE__);
        }

        // Unlink the file or directory
        result = vfs.removeat(vfs_rel_fd, path_str);
        dealloc(path_str);
        vfs.close(vfs_rel_fd);
        
        if (result < 0)
            return cvt_error();
        return 0;
    }
}
