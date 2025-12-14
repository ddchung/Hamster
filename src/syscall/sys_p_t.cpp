// Hamster P-T system calls

#include <syscall/syscall.hpp>
#include <process/task.hpp>
#include <process/task_vfs_fd.hpp>
#include <process/task_pipe.hpp>
#include <abi/values.hpp>
#include <abi/structs.hpp>

namespace Hamster
{
    namespace
    {
        bool is_fd_set(uint32_t *fds, int fd)
        {
            return (fds[fd / 32] & (1u << (fd % 32))) != 0;
        }
    } // namespace

    int32_t sys_read(Task &task, int32_t fd, uint32_t buf_loc, uint32_t count)
    {
        // Ensure registers are correct for block()
        task.get_emulator().x[10] = fd; // a0
        task.get_emulator().x[11] = buf_loc; // a1
        task.get_emulator().x[12] = count; // a2

        task.block([](Task &task, uint32_t task_fd, uint32_t buf_loc, uint32_t count, uint32_t, uint32_t, uint32_t) -> int {
            BaseTaskFD *fd = task.get_fd(task_fd);
            if (!fd)
                return -1;

            bool is_nonblock = fd->get_flags() & OPEN_NONBLOCK;

            // Check if it is readable
            switch (fd->poll(POLL_READ))
            {
            case 0:
                if (is_nonblock)
                    return -H_EAGAIN;
                error = H_EAGAIN;
                return -1;
            case 1:
                break;
            default:
                return -1;
            }

            // Read up to end of VM page

            uint32_t to_read = std::min(count, HAMSTER_PAGE_SIZE - (buf_loc % HAMSTER_PAGE_SIZE));

            void *it = task.mem_make_iterator(buf_loc);
            if (!it)
                return -1;
            
            int res = fd->read(it, to_read);
            if (res == -1 && error == H_EAGAIN && is_nonblock)
                return -H_EAGAIN;
            return res;
        });

        return 0;
    }

    int32_t sys_setpgid(Task &task, int32_t pid, int32_t pgid)
    {
        if (pid == 0)
            pid = task.get_pid();
        if (pgid == 0)
            pgid = pid;
        
        if ((uint32_t)pid == task.get_pid())
        {
            int res = task.set_pgid(pgid);
            if (res < 0)
                return cvt_error();
            return 0;
        }
        else
        {
            // Find the child process with the given PID
            const Set<Process *> &children = task.get_children_processes();
            for (Process *child : children)
            {
                if (child->get_pid() == (uint32_t)pid)
                {
                    int res = child->set_pgid(pgid);
                    if (res < 0)
                        return cvt_error();
                    return 0;
                }
            }

            // not found
            return -H_ESRCH;
        }
    }

    int32_t sys_setsid(Task &task)
    {
        int res = task.setsid();
        if (res < 0)
            return cvt_error();
        return task.get_sid();
    }

    int32_t sys_sched_yield(Task &task)
    {
        // no-op for now
        return 0;
    }

    int32_t sys_statx(Task &task, int32_t dirfd, uint32_t pathname_loc, int32_t flags, uint32_t mask, uint32_t statxbuf_loc)
    {
        // Note: this syscall will be implemented with VFS stat and lstat, since there is no `statx` support yet.
        //       This means that statx fields that aren't part of normal stat won't be supported

        char *path = task.mem_get_string(pathname_loc);

        sys_stat statbuf = {};
        int res;

        // Check if we should stat a path or the file specifed by `dirfd`
        if ((flags & H_AT_EMPTY_PATH) && (!path || !path[0]))
        {
            dealloc(path);

            // Use `dirfd` as the file
            
            BaseTaskFD *file = task.get_fd(dirfd);
            if (!file)
                return cvt_error();
            
            res = file->stat(&statbuf);
        }
        else
        {
            // Check file pointed to by `path`

            if (!path || !path[0])
            {
                dealloc(path);
                return -H_EINVAL;
            }
            
            int rel_fd = task.open_rel_fd(dirfd, path);
            if (rel_fd < 0)
            {
                dealloc(path);
                return -1;
            }

            res = flags & H_AT_SYMLINK_NOFOLLOW ? vfs.lstatat(rel_fd, path, &statbuf) : vfs.statat(rel_fd, path, &statbuf);
            vfs.close(rel_fd);
            dealloc(path);
        }

        if (res < 0)
            return cvt_error();
        
        // Convert to statx buffer, and advertise only basic stat support

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

        if (task.copy_to_memory(statxbuf_loc, statxbuf) < 0)
            return cvt_error();
        
        return 0;
    }

    int32_t sys_pselect6_time64(Task &task, int32_t nfds, uint32_t readfds_loc, uint32_t writefds_loc, uint32_t exceptfds_loc, uint32_t timeout_loc, uint32_t sigmask_loc)
    {
        task.block([](Task &task, uint32_t nfds, uint32_t readfds_loc, uint32_t writefds_loc, uint32_t exceptfds_loc, uint32_t timeout_loc, uint32_t sigmask_loc) {
            // Prepare file descriptor sets
            // Note that we will not support exceptfds for now
            uint32_t fdset_size = (nfds + 31) / 32; // Number of 32-bit words needed
            
            // Prevent arbitrary values from causing stack overflow
            if (fdset_size > 256)
            {
                error = H_EINVAL;
                return -1;
            }

            uint32_t *read_fds = (uint32_t*)alloca(fdset_size * sizeof(uint32_t));
            uint32_t *write_fds = (uint32_t*)alloca(fdset_size * sizeof(uint32_t));

            if (readfds_loc != 0)
            {
                if (task.copy_from_memory(*read_fds, readfds_loc, fdset_size * sizeof(uint32_t)) < 0)
                    return -1;
            }
            if (writefds_loc != 0)
            {
                if (task.copy_from_memory(*write_fds, writefds_loc, fdset_size * sizeof(uint32_t)) < 0)
                    return -1;
            }
            if (exceptfds_loc != 0)
            {
                // Mark all of them as not ready
                if (task.memset(exceptfds_loc, 0, fdset_size * sizeof(uint32_t)) < 0)
                    return -1;
            }

            // check timeout
            if (timeout_loc != 0)
            {
                sys_timespec timeout = {};
                if (task.copy_from_memory(timeout, timeout_loc) < 0)
                    return -1;
                // Check if we ran out of time
                uint64_t now = _get_sys_time();

                if (task.get_last_tick() + timespec_to_systick(timeout) + 5 <= now)
                {
                    // Timeout reached, mark all as not ready
                    if ((readfds_loc != 0 &&
                        task.memset(readfds_loc, 0, fdset_size * sizeof(uint32_t)) < 0)
                        || (writefds_loc != 0 &&
                        task.memset(writefds_loc, 0, fdset_size * sizeof(uint32_t)) < 0))
                    {
                        return -1;
                    }
                    return 0; // No file descriptors ready, return 0
                }
            }

            // Note: signal mask isn't implemented yet

            for (int i = 0; i < (int32_t)nfds; ++i)
            {
                bool ready_read, ready_write;

                BaseTaskFD *task_fd = task.get_fd(i);
                if (!task_fd)
                    return -1;

                ready_read = is_fd_set(read_fds, i) && task_fd->poll(POLL_READ) == 1;
                ready_write = is_fd_set(write_fds, i) && task_fd->poll(POLL_WRITE) == 1;

                if (!ready_read && !ready_write)
                    continue; // Not ready, skip

                memset(read_fds, 0, fdset_size * sizeof(uint32_t));
                memset(write_fds, 0, fdset_size * sizeof(uint32_t));
                
                // File descriptor is ready, update the sets
                if (ready_read)
                {
                    read_fds[i / 32] |= (1u << (i % 32));
                }
                if (ready_write)
                {
                    write_fds[i / 32] |= (1u << (i % 32));
                }

                // copy to user memory
                if (readfds_loc != 0 &&
                    task.copy_to_memory(readfds_loc, *read_fds, fdset_size * sizeof(uint32_t)) < 0)
                {
                    return -1;
                }
                if (writefds_loc != 0 &&
                    task.copy_to_memory(writefds_loc, *write_fds, fdset_size * sizeof(uint32_t)) < 0)
                {
                    return -1;
                }

                // Return the number of ready file descriptors
                return ready_read + ready_write;
            }

            // Block
            error = H_EAGAIN;
            return -1;
        });
        return 0;
    }

    int32_t sys_rt_sigaction(Task &task, int32_t signo, uint32_t act_loc, uint32_t oldact_loc, uint32_t sigset_size)
    {
        if (signo < 1 || signo > H_SIGRTMAX || sigset_size != sizeof(sys_sigset))
            return -H_EINVAL;
        
        if (signo == H_SIGKILL || signo == H_SIGSTOP)
            return -H_EINVAL;
        
        // Copy old action to memory
        if (oldact_loc && task.copy_to_memory(oldact_loc, task.get_sigaction(signo)) < 0)
            return cvt_error();
        
        if (act_loc)
        {
            sys_sigaction action;
            if (task.copy_from_memory(action, act_loc) < 0)
                return cvt_error();
            
            if (task.sigaction(signo, action) < 0)
                return cvt_error();
        }

        return 0;
    }

    int32_t sys_rt_sigreturn(Task &task)
    {
        task.sigreturn();
        return 0;
    }

    int32_t sys_pipe2(Task &task, uint32_t pipefd_loc, int32_t flags)
    {
        auto pipes = TaskPipe::make_pair(flags & OPEN_NONBLOCK);

        // Add both pipes
        int fd1 = task.set_fd(pipes.first);
        int fd2 = task.set_fd(pipes.second);

        if (fd1 < 0 || fd2 < 0)
        {
            if (fd1 >= 0) task.close_fd(fd1);
            if (fd2 >= 0) task.close_fd(fd2);
            return cvt_error();
        }

        if (flags & OPEN_CLOEXEC)
        {
            task.set_fd_flags(fd1, H_FD_CLOEXEC);
            task.set_fd_flags(fd2, H_FD_CLOEXEC);
        }

        int32_t pipefds[2] = {fd1, fd2};

        if (task.copy_to_memory(pipefd_loc, pipefds) < 0)
            return cvt_error();
        
        return 0;
    }

    int32_t sys_rt_sigprocmask(Task &task, int32_t how, uint32_t set_loc, uint32_t oldset_loc, uint32_t sigset_size)
    {
        if (sigset_size != sizeof(sys_sigset))
            return -H_EINVAL;

        sys_sigset old_set = task.get_signal_sigset();

        if (oldset_loc && task.copy_to_memory(oldset_loc, old_set) < 0)
            return cvt_error();
        
        if (set_loc)
        {
            sys_sigset set;

            if (task.copy_from_memory(set, set_loc) < 0)
                return cvt_error();
            
            switch (how)
            {
            case H_SIG_BLOCK:
                old_set.sig[0] |= set.sig[0];
                old_set.sig[1] |= set.sig[1];
                break;
            case H_SIG_UNBLOCK:
                old_set.sig[0] &= ~set.sig[0];
                old_set.sig[1] &= ~set.sig[1];
                break;
            case H_SIG_SETMASK:
                old_set.sig[0] = set.sig[0];
                old_set.sig[1] = set.sig[1];
                break;
            default:
                return -H_EINVAL;
            }

            task.set_signal_mask(old_set);
        }

        return 0;
    }

    int32_t sys_renameat2(Task &task, int32_t old_dfd, uint32_t oldpath_loc, int32_t new_dfd, uint32_t newpath_loc, uint32_t flags)
    {
        // TODO: support flags
        if (flags != 0)
            return -H_ENOTSUP;

        char *old_path = task.mem_get_string(oldpath_loc);
        char *new_path = task.mem_get_string(newpath_loc);

        if (!old_path || !new_path)
        {
            dealloc(old_path);
            dealloc(new_path);
            return cvt_error();
        }

        int old_rel_fd = task.open_rel_fd(old_dfd, old_path);
        int new_rel_fd = task.open_rel_fd(new_dfd, new_path);

        int res = vfs.renameat(old_rel_fd, old_path, new_rel_fd, new_path);
        vfs.close(old_rel_fd);
        vfs.close(new_rel_fd);
        dealloc(old_path);
        dealloc(new_path);

        if (res < 0)
            return cvt_error();
        return 0;
    }

    int32_t sys_truncate64(Task &task, uint32_t path_loc, uint32_t off_high, uint32_t off_low)
    {
        int64_t off = ((uint64_t)off_high << 32) | off_low;

        char *path = task.mem_get_string(path_loc);
        if (!path)
            return cvt_error();
        
        int fd = task.open_rel_file(H_AT_FDCWD, path, OPEN_WRONLY);
        dealloc(path);
        if (fd < 0)
            return cvt_error();
        
        int res = vfs.truncate(fd, off);
        vfs.close(fd);

        if (res < 0)
            return cvt_error();
        return 0;
    }

    int32_t sys_renameat(Task &task, int32_t old_dfd, uint32_t oldpath_loc, int32_t new_dfd, uint32_t newpath_loc)
    {
        // Call renameat2 with flags=0
        return sys_renameat2(task, old_dfd, oldpath_loc, new_dfd, newpath_loc, 0);
    }

    int32_t sys_tgkill(Task &task, int32_t tgid, int32_t tid, int32_t signal)
    {
        // tgid is ignored
        (void)tgid;

        Task *target = Task::get_task(tid);
        if (!target)
            return cvt_error();
        
        sys_siginfo siginfo = make_kill_siginfo(signal, sys_getuid(task), task.get_pid());

        if (task.check_can_signal(*target, siginfo) < 0)
            return cvt_error();
        
        if (target->send_signal(siginfo) < 0)
            return cvt_error();
        
        return 0;
    }

    int32_t sys_setuid(Task &task, uint32_t new_uid)
    {
        int uid, euid, suid;
        task.get_uid(&uid, &euid, &suid);

        if ((int32_t)new_uid != uid && (int32_t)new_uid != suid && euid != 0)
            return -H_EPERM;
        
        if (euid == 0)
        {
            uid = new_uid;
            suid = new_uid;
        }

        euid = new_uid;

        task.set_uid(uid, euid, suid);

        return 0;
    }

    int32_t sys_setgid(Task &task, uint32_t new_gid)
    {
        int gid, egid, sgid;
        task.get_gid(&gid, &egid, &sgid);

        if ((int32_t)new_gid != gid && (int32_t)new_gid != sgid && egid != 0)
            return -H_EPERM;
        
        if (egid == 0)
        {
            gid = new_gid;
            sgid = new_gid;
        }

        egid = new_gid;

        task.set_gid(gid, egid, sgid);

        return 0;
    }

    int32_t sys_setreuid(Task &task, uint32_t new_uid, uint32_t new_euid)
    {
        int uid, euid, suid;
        task.get_uid(&uid, &euid, &suid);

        if (new_uid != (uint32_t)-1)
        {
            // If the process isn't privileged, and new new real UID is not the same as either
            // the old real UID or effective UID, fail with H_EPERM
            if (uid != 0 && (int32_t)new_uid != uid && (int32_t)new_uid != euid)
                return -H_EPERM;

            uid = new_uid;
        }

        if (new_euid != (uint32_t)-1)
        {
            // If the process isn't privileged, and new effective UID is not the same as one of:
            // - The old real UID
            // - The effective UID
            // - The saved set-user ID
            // Then fail with H_EPERM
            if (uid != 0 && (int32_t)new_euid != uid && (int32_t)new_euid != euid && (int32_t)new_euid != suid)
                return -H_EPERM;
            euid = new_euid;
        }

        task.set_uid(uid, euid, suid);

        return 0;
    }

    int32_t sys_setregid(Task &task, uint32_t new_gid, uint32_t new_egid)
    {
        int gid, egid, sgid;
        task.get_gid(&gid, &egid, &sgid);

        if (new_gid != (uint32_t)-1)
        {
            if (gid != 0 && (int32_t)new_gid != gid && (int32_t)new_gid != egid)
                return -H_EPERM;

            gid = new_gid;
        }

        if (new_egid != (uint32_t)-1)
        {
            if (gid != 0 && (int32_t)new_egid != gid && (int32_t)new_egid != egid && (int32_t)new_egid != sgid)
                return -H_EPERM;
            egid = new_egid;
        }

        task.set_gid(gid, egid, sgid);

        return 0;
    }

    int32_t sys_setresuid(Task &task, uint32_t new_uid, uint32_t new_euid, uint32_t new_suid)
    {
        int uid, euid, suid;
        task.get_uid(&uid, &euid, &suid);

        if (new_uid != (uint32_t)-1)
        {
            if (uid != 0 && (int32_t)new_uid != uid && (int32_t)new_uid != euid && (int32_t)new_uid != suid)
                return -H_EPERM;
            uid = new_uid;
        }

        if (new_euid != (uint32_t)-1)
        {
            if (uid != 0 && (int32_t)new_euid != uid && (int32_t)new_euid != euid && (int32_t)new_euid != suid)
                return -H_EPERM;
            euid = new_euid;
        }

        if (new_suid != (uint32_t)-1)
        {
            if (uid != 0 && (int32_t)new_suid != uid && (int32_t)new_suid != euid && (int32_t)new_suid != suid)
                return -H_EPERM;
            suid = new_suid;
        }

        task.set_uid(uid, euid, suid);

        return 0;
    }

    int32_t sys_setresgid(Task &task, uint32_t new_gid, uint32_t new_egid, uint32_t new_sgid)
    {
        int gid, egid, sgid;
        task.get_gid(&gid, &egid, &sgid);

        if (new_gid != (uint32_t)-1)
        {
            if (gid != 0 && (int32_t)new_gid != gid && (int32_t)new_gid != egid && (int32_t)new_gid != sgid)
                return -H_EPERM;
            gid = new_gid;
        }

        if (new_egid != (uint32_t)-1)
        {
            if (gid != 0 && (int32_t)new_egid != gid && (int32_t)new_egid != egid && (int32_t)new_egid != sgid)
                return -H_EPERM;
            egid = new_egid;
        }

        if (new_sgid != (uint32_t)-1)
        {
            if (gid != 0 && (int32_t)new_sgid != gid && (int32_t)new_sgid != egid && (int32_t)new_sgid != sgid)
                return -H_EPERM;
            sgid = new_sgid;
        }

        task.set_gid(gid, egid, sgid);

        return 0;
    }

    int32_t sys_readv(Task &task, int32_t fd, uint32_t vec_loc, uint32_t vlen)
    {
        if (vlen == 0)
            return 0;
        
        sys_iovec vec;
        if (task.copy_from_memory(vec, vec_loc) < 0)
            return cvt_error();
        
        if (vec.size == 0)
            return sys_readv(task, fd, vec_loc + sizeof(sys_iovec), vlen - 1);
        
        return sys_read(task, fd, vec.data, vec.size);
    }
} // namespace Hamster

