// Hamster faccessat and faccessat2

#include <syscall/syscall.hpp>
#include <process/scheduler.hpp>
#include <filesystem/vfs.hpp>
#include <errno/errno.h>
#include <algorithm>

namespace Hamster
{
    int32_t sys_faccessat(int32_t dirfd, uint32_t path_loc, int32_t mode)
    {
        // call faccessat2 with flags=0
        return sys_faccessat2(dirfd, path_loc, mode, 0);
    }

    int32_t sys_faccessat2(int32_t dirfd, uint32_t path_loc, int32_t requested, int32_t flags)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);
    
        // Get the path
        char *path_str = current_task->get_memory().get_string(path_loc);

        int vfs_fd;

        if (!path_str || path_str[0] == '\0')
        {
            // null or empty path
            dealloc(path_str);
            if (!(flags & H_AT_EMPTY_PATH))
                return -H_EINVAL;
            vfs_fd = current_task->get_vfs_fd(dirfd);
            vfs_fd = vfs.dup(vfs_fd);
        }
        else
        {
            int vfs_rel_fd = current_task->get_relative_fd(path_str, dirfd);
            if (vfs_rel_fd < 0)
            {
                dealloc(path_str);
                return -H_EBADF;
            }

            vfs_fd = vfs.openat(vfs_rel_fd, path_str, OPEN_RDONLY);
            dealloc(path_str);
            vfs.close(vfs_rel_fd);
        }

        if (vfs_fd < 0)
            return cvt_error();

        uint32_t mode = vfs.get_mode(vfs_fd);
        uint32_t uid = vfs.get_uid(vfs_fd);
        uint32_t gid = vfs.get_gid(vfs_fd);
        vfs.close(vfs_fd);

        uint32_t proc_uid, proc_gid;
        auto &proc_sup_gids = current_task->process->obj.supplementary_gids;

        if (flags & H_AT_EACCESS)
        {
            proc_uid = current_task->process->obj.euid;
            proc_gid = current_task->process->obj.egid;
        }
        else
        {
            proc_uid = current_task->process->obj.uid;
            proc_gid = current_task->process->obj.gid;
        }

        // Do the permission checking
        if (uid == 0 && (mode & 0111))
            mode |= 0111; // Root can execute anything, if owner|group|world can execute
        
        if (uid == proc_uid)
            mode >>= 6; // owner
        else if (gid == proc_gid)
            mode >>= 3; // direct group
        else if (std::find(proc_sup_gids.begin(), proc_sup_gids.end(), gid) != proc_sup_gids.end())
            mode >>= 3; // supplementary group
        else
            mode >>= 0; // other

        mode &= 07;
        requested &= 07;

        if ((requested & ~mode) != 0)
            return -H_EACCES;

        return 0;
    }
}
