// Hamster statx system call

#include <syscall/syscall.hpp>
#include <filesystem/vfs.hpp>
#include <process/scheduler.hpp>
#include <memory/allocator.hpp>
#include <abi/structs.hpp>
#include <errno/errno.h>

namespace Hamster
{
    int32_t sys_statx(int32_t dirfd, uint32_t pathname_loc, int32_t flags, uint32_t mask, uint32_t statxbuf_loc)
    {
        Task *task = scheduler.get_current_task();
        assert(task != nullptr);

        // Check if we support the things in `mask`
        if (mask & ~H_STATX_BASIC_STATS)
        {
            error = ENOSYS; // Not implemented
            return cvt_error();
        }

        // Get the path from the task's memory
        char *path_str = task->memory->obj.memory.get_string(pathname_loc);

        sys_stat statbuf = {};
        int ret = 0;

        if (!path_str || path_str[0] == '\0')
        {
            // Empty path
            dealloc(path_str);
            if (flags & H_AT_EMPTY_PATH)
            {
                int vfs_fd = task->get_vfs_fd(dirfd);
                if (vfs_fd < 0)
                    return cvt_error();
                ret = vfs.stat(vfs_fd, &statbuf);
            }
            else
            {
                error = path_str ? ENOENT : EINVAL;
                return cvt_error();
            }
        }
        else
        {
            // Normal path
            int vfs_relfd = task->get_relative_fd(path_str, dirfd);
            if (vfs_relfd < 0)
            {
                dealloc(path_str);
                return cvt_error();
            }
            
            if (flags & H_AT_SYMLINK_NOFOLLOW)
                ret = vfs.lstatat(vfs_relfd, path_str, &statbuf);
            else
                ret = vfs.statat(vfs_relfd, path_str, &statbuf);
            dealloc(path_str);
            vfs.close(vfs_relfd);
        }

        if (ret < 0)
        {
            return cvt_error();
        }

        // Fill the statxbuf with the basic stats
        struct sys_statx statxbuf = {};
        statxbuf.mask = H_STATX_BASIC_STATS;
        statxbuf.rdev_major = statbuf.rdev >> 20;
        statxbuf.rdev_minor = statbuf.rdev & 0xFFFFF;
        statxbuf.ino = statbuf.ino;
        statxbuf.mode = statbuf.mode;
        statxbuf.nlink = statbuf.nlink;
        statxbuf.uid = statbuf.uid;
        statxbuf.gid = statbuf.gid;
        statxbuf.dev_major = statbuf.dev >> 20;
        statxbuf.dev_minor = statbuf.dev & 0xFFFFF;
        statxbuf.size = statbuf.size;
        statxbuf.blksize = statbuf.blksize;
        statxbuf.blocks = statbuf.blocks;
        statxbuf.atime.sec = statbuf.atime;
        statxbuf.atime.nsec = statbuf.atime_nsec;
        statxbuf.mtime.sec = statbuf.mtime;
        statxbuf.mtime.nsec = statbuf.mtime_nsec;
        statxbuf.ctime.sec = statbuf.ctime;
        statxbuf.ctime.nsec = statbuf.ctime_nsec;

        // Copy to user memory
        if (task->memory->obj.memory.memcpy_alloc(statxbuf_loc, &statxbuf, sizeof(statxbuf)) < 0)
        {
            error = EFAULT; // Bad address
            return cvt_error();
        }

        return 0;
    }
} // namespace Hamster

