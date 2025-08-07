// Hamster fchmodat system call

#include <syscall/syscall.hpp>
#include <process/scheduler.hpp>
#include <filesystem/vfs.hpp>
#include <errno/errno.h>

namespace Hamster
{
    int32_t sys_fchmodat(int32_t dirfd, uint32_t path_loc, uint32_t mode, int32_t flags)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        if (path_loc == 0)
            return -EFAULT; // Invalid path location
        
        if (flags & H_AT_SYMLINK_NOFOLLOW)
            return -ENOTSUP; // Cannot change mode of symlinks directly
        else if (flags != 0)
            return -EINVAL; // Unsupported flags
        
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

        // Perform the fchmodat operation

        int fd = vfs.openat(vfs_rel_dfd, path, OPEN_RDWR);
        vfs.close(vfs_rel_dfd);
        dealloc(path);

        if (fd < 0)
            return cvt_error();

        int res = vfs.chmod(fd, mode);
        vfs.close(fd);

        if (res < 0)
            return cvt_error();
        return 0;
    }

} // namespace Hamster
