// Hamster task filesystem functions

#include <process/task.hpp>
#include <memory/allocator.hpp>

namespace Hamster
{
    int Task::accessat(int dirfd, const char *pathname, int mode, int flags)
    {
        int uid, gid, *groups;

        if (flags & H_AT_EACCESS)
        {
            process->get_uid(nullptr, &uid, nullptr);
            process->get_gid(nullptr, &gid, nullptr);
        }
        else
        {
            process->get_uid(&uid, nullptr, nullptr);
            process->get_gid(&gid, nullptr, nullptr);
        }

        // Put `gid` into groups
        groups = alloc<int>(process->get_groups().size() + 1);
        if (process->get_groups().size() > 0)
            ::memcpy(groups, process->get_groups().data(), process->get_groups().size() * sizeof(int));
        groups[process->get_groups().size()] = gid;

        // Only forward AT_SYMLINK_NOFOLLOW
        int res = vfs.accessat(dirfd, pathname, uid, groups, process->get_groups().size() + 1, mode, flags & H_AT_SYMLINK_NOFOLLOW);
        dealloc(groups);
        return res;
    }

    int Task::open_rel_fd(int thread_dfd, const char *path)
    {
        BaseTaskFD *fd;
        if (thread_dfd >= 0)
            fd = get_fd(thread_dfd);
        else if (thread_dfd == H_AT_FDCWD)
            fd = nullptr;
        else
        {
            error = H_EBADF;
            return -1;
        }

        return process->get_fs_info()->open_rel_fd(path, fd);
    }

    int Task::open_rel_file(int thread_dfd, const char *path, int flags)
    {
        if ((flags & H_AT_EMPTY_PATH) && (!path || !path[0]))
        {
            // Operate on `thread_dfd`
            BaseTaskFD *tfd = get_fd(thread_dfd);
            if (!tfd)
                return -1;
            int vfs_fd = tfd->get_vfs_fd();
            if (vfs_fd < 0)
            {
                error = H_EBADF;
                return -1;
            }
            return vfs.dup(vfs_fd);
        }
        else
        {
            if (!path || !path[0])
            {
                error = H_EINVAL;
                return -1;
            }

            // TODO: permission checking

            int rel_fd = open_rel_fd(thread_dfd, path);
            if (rel_fd < 0)
                return -1;
            
            int file = vfs.openat(rel_fd, path, flags & ~(OPEN_CREAT | H_AT_EMPTY_PATH | 0x03));
            vfs.close(rel_fd);

            return file;
        }
    }

    BaseTaskFD *Task::get_fd(int fd) const
    {
        return fd_table->get_fd(fd);
    }

    int Task::get_fd_flags(int fd) const
    {
        return fd_table->get_fd_flags(fd);
    }

    int Task::set_fd_flags(int fd, int flags)
    {
        return fd_table->set_fd_flags(fd, flags);
    }

    int Task::get_vfs_fd(int task_fd)
    {
        BaseTaskFD *fd = get_fd(task_fd);
        if (!fd)
            return -1;
        return fd->get_vfs_fd();
    }

    int Task::close_fd(int fd)
    {
        return fd_table->close(fd);
    }

    int Task::allocate_fd(int start)
    {
        return fd_table->allocate_fd(start);
    }

    int Task::set_fd(BaseTaskFD *task_fd, int fd)
    {
        return fd_table->set_fd(task_fd, fd);
    }

    void Task::close_cloexec_fds()
    {
        fd_table->close_cloexec();
    }

    int Task::mask_mode(int mode) const
    {
        return process->get_fs_info()->mask_mode(mode);
    }

    int Task::dup_fd(int fd, int new_fd)
    {
        return fd_table->dup(fd, new_fd);
    }

    void Task::clear_fds()
    {
        fd_table->clear();
    }

    int Task::chroot(const char *path)
    {
        int dirfd = open_rel_fd(H_AT_FDCWD, path);
        if (dirfd < 0)
            return -1;
        int res = accessat(dirfd, path, PERM_EXEC, 0);
        vfs.close(dirfd);
        if (res < 0)
            return -1;
        return process->get_fs_info()->chroot(path);
    }

    int Task::chdir(const char *path)
    {
        int dirfd = open_rel_fd(H_AT_FDCWD, path);
        if (dirfd < 0)
            return -1;
        int res = accessat(dirfd, path, PERM_EXEC, 0);
        vfs.close(dirfd);
        if (res < 0)
            return -1;
        return process->get_fs_info()->chdir(path);
    }

    char *Task::getcwd()
    {
        return process->get_fs_info()->getcwd();
    }

    char *Task::get_abs_cwd()
    {
        return process->get_fs_info()->get_abs_cwd();
    }

    int Task::get_umask() const
    {
        return process->get_fs_info()->get_umask();
    }

    void Task::set_umask(int new_umask)
    {
        process->get_fs_info()->set_umask(new_umask);
    }
} // namespace Hamster

