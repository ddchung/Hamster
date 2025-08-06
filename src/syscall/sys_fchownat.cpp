// Hamster fchownat syscall implementation

#include <syscall/syscall.hpp>
#include <process/scheduler.hpp>
#include <filesystem/vfs.hpp>
#include <errno/errno.h>

namespace Hamster
{
    int32_t sys_fchownat(int32_t dirfd, uint32_t path_loc, int32_t uid, int32_t gid, int32_t flags)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        // Check if the path is valid
        if (path_loc == 0)
        {
            return -EFAULT;
        }

        char *path = current_task->get_memory().get_string(path_loc);
        if (!path)
        {
            error = EFAULT;
            return cvt_error();
        }

        int vfs_rel_dfd = current_task->get_relative_fd(path, dirfd);
        if (vfs_rel_dfd < 0)
        {
            dealloc(path);
            return cvt_error();
        }

        // Perform the fchownat operation

        int res;

        if (flags & H_AT_SYMLINK_NOFOLLOW)
            res = vfs.lchownat(vfs_rel_dfd, path, uid, gid);
        else
            res = vfs.chownat(vfs_rel_dfd, path, uid, gid);

        dealloc(path);
        vfs.close(vfs_rel_dfd);

        if (res < 0)
            return cvt_error();
        return 0;
    }
} // namespace Hamster

