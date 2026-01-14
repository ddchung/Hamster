// Hamster F-J system calls

#include <syscall/syscall.hpp>
#include <process/task.hpp>
#include <process/task_vfs_fd.hpp>
#include <abi/values.hpp>
#include <abi/structs.hpp>

namespace Hamster
{
    int32_t sys_getpid(Task &task)
    {
        return task.get_pid();
    }

    int32_t sys_gettid(Task &task)
    {
        return task.get_tid();
    }

    int32_t sys_getpgid(Task &task, int32_t pid)
    {
        if (pid == 0)
            pid = task.get_pid();
        
        Task *t = Task::get_task_pid(pid);
        if (!t)
            return cvt_error();
        
        return t->get_pgid();
    }

    int32_t sys_getsid(Task &task, int32_t pid)
    {
        if (pid == 0)
            pid = task.get_pid();
        
        Task *t = Task::get_task_pid(pid);
        if (!t)
            return cvt_error();
        
        return t->get_sid();
    }

    int32_t sys_getppid(Task &task)
    {
        return task.get_ppid();
    }

    int32_t sys_ioctl(Task &task, int32_t task_fd, int32_t op, uint32_t arg)
    {
        static uint8_t IOCTL_BUF[HAMSTER_MAX_IOCTL_SIZE];

        BaseTaskFD *fd = task.get_fd(task_fd);
        if (!fd)
            return cvt_error();
        
        IoctlArg ioarg;
        int res;
        ioarg.i = arg;

        int error = 0;
        
        if ((arg % HAMSTER_PAGE_SIZE) > HAMSTER_PAGE_SIZE - HAMSTER_MAX_IOCTL_SIZE)
        {
            task.memcpy(IOCTL_BUF, arg, HAMSTER_MAX_IOCTL_SIZE);
            ioarg.p = IOCTL_BUF;
            Hamster::error = 0;
            res = fd->ioctl(op, ioarg);
            error = Hamster::error;
            task.memcpy(arg, IOCTL_BUF, HAMSTER_MAX_IOCTL_SIZE);
        }
        else
        {
            ioarg.p = task.mem_make_iterator(arg);
            Hamster::error = 0;
            res = fd->ioctl(op, ioarg);
            error = Hamster::error;
        }

        if (error)
            return cvt_error();
        return res;
    }

    int32_t sys_getuid(Task &task)
    {
        int uid;
        task.get_uid(&uid);
        return uid;
    }

    int32_t sys_geteuid(Task &task)
    {
        int euid;
        task.get_uid(nullptr, &euid);
        return euid;
    }

    int32_t sys_getresuid(Task &task, uint32_t ruid_loc, uint32_t euid_loc, uint32_t suid_loc)
    {
        int uid, euid, suid;
        task.get_uid(&uid, &euid, &suid);
        if ((ruid_loc && task.copy_to_memory(ruid_loc, uid) < 0)
         || (euid_loc && task.copy_to_memory(euid_loc, euid) < 0)
         || (suid_loc && task.copy_to_memory(suid_loc, suid) < 0))
        {
            return cvt_error();
        }
        return 0;
    }

    int32_t sys_getgid(Task &task)
    {
        int gid;
        task.get_gid(&gid);
        return gid;
    }

    int32_t sys_getegid(Task &task)
    {
        int egid;
        task.get_gid(nullptr, &egid);
        return egid;
    }

    int32_t sys_getresgid(Task &task, uint32_t rgid_loc, uint32_t egid_loc, uint32_t sgid_loc)
    {
        int gid, egid, sgid;
        task.get_gid(&gid, &egid, &sgid);
        if ((rgid_loc && task.copy_to_memory(rgid_loc, gid) < 0)
         || (egid_loc && task.copy_to_memory(egid_loc, egid) < 0)
         || (sgid_loc && task.copy_to_memory(sgid_loc, sgid) < 0))
        {
            return cvt_error();
        }
        return 0;
    }

    int32_t sys_faccessat(Task &task, int32_t dirfd, uint32_t pathname_loc, int32_t mode)
    {
        return sys_faccessat2(task, dirfd, pathname_loc, mode, 0);
    }

    int32_t sys_faccessat2(Task &task, int32_t dirfd, uint32_t pathname_loc, int32_t mode, int32_t flags)
    {
        char *path = task.mem_get_string(pathname_loc);
        if (!path)
            return cvt_error();
        
        int rel_fd = task.open_rel_fd(dirfd, path);
        if (rel_fd < 0)
            return cvt_error();
        
        int res = task.accessat(rel_fd, path, mode, flags);
        dealloc(path);
        vfs.close(rel_fd);

        if (res < 0)
            return cvt_error();
        return 0;
    }

    int32_t sys_fcntl64(Task &task, int32_t task_fd, int32_t op, uint32_t arg)
    {
        switch (op)
        {
        case FILE_DUPFD:
        case FILE_DUPFD_CLOEXEC:
        {
            int new_slot = task.allocate_fd(arg);
            if (new_slot < 0)
                return cvt_error();
            if (task.dup_fd(task_fd, new_slot) < 0)
                return cvt_error();
            task.set_fd_flags(new_slot, op == FILE_DUPFD_CLOEXEC ? H_FD_CLOEXEC : 0);
            return new_slot;
        }
        case FILE_GETFD:
        {
            int res = task.get_fd_flags(task_fd);
            if (res < 0)
                return cvt_error();
            return res;
        }
        case FILE_SETFD:
            if (task.set_fd_flags(task_fd, arg & H_FD_CLOEXEC) < 0)
                return cvt_error();
            return 0;
        case FILE_GETFL:
        {
            BaseTaskFD *fd = task.get_fd(task_fd);
            if (!fd)
                return cvt_error();
            int res = fd->get_flags();
            if (res < 0)
                return cvt_error();
            return res;
        }
        case FILE_SETFL:
        {
            constexpr int CHANGEABLE_FLAGS = OPEN_APPEND | OPEN_NONBLOCK;
            BaseTaskFD *fd = task.get_fd(task_fd);
            if (!fd)
                return cvt_error();
            int old_flags = fd->get_flags();
            if (old_flags < 0)
                return cvt_error();
            if (fd->set_flags((old_flags & ~CHANGEABLE_FLAGS) | (arg & CHANGEABLE_FLAGS)) < 0)
                return cvt_error();
            return 0;
        }
        default:
            return -H_EINVAL;
        }
    }

    int32_t sys_getdents64(Task &task, int32_t fd, uint32_t dirent_loc, uint32_t count)
    {
        BaseTaskFD *file = task.get_fd(fd);
        if (!file)
            return cvt_error();

        int vfs_fd = file->get_vfs_fd();
        if (vfs_fd < 0)
            return cvt_error();

        if (!dirent_loc)
        {
            error = H_EFAULT; // Bad address
            return cvt_error();
        }

        vfs.seek(vfs_fd, 0, H_SEEK_SET);

        char *const *list = file->list();
        if (!list)
            return -H_EINVAL;

        bool ok = true;
        uint32_t bytes_read = 0;
        int64_t to_skip = file->tell();
        int64_t off = to_skip;

        for (const char * const *entry = list; *entry != nullptr; ++entry)
        {
            const char *name = *entry;
            size_t name_len = strlen(name);

            if (to_skip > 0)
            {
                to_skip -= sizeof(sys_dirent) + name_len;
                continue;
            }

            if (bytes_read + sizeof(sys_dirent) + name_len > count)
            {
                // End of buffer
                break;
            }

            sys_stat st;
            if (vfs.lstatat(vfs_fd, name, &st) < 0)
            {
                // If stat fails, we can skip this entry
                continue;
            }

            // sys_dirent's name field is an array of size 1, so we need to ensure enough space
            // here, we use the our _malloc and _free instead of alloc<T>/dealloc<T> because
            // we need more space than sizeof(sys_dirent)

            // sys_dirent's name[1] accounts for the null terminator, so we only add name_len
            sys_dirent *dirent = (sys_dirent *)_malloc(sizeof(sys_dirent) + name_len);
            assert(dirent != nullptr);

            dirent->ino = st.ino;
            dirent->offset = off + bytes_read;
            dirent->reclen = sizeof(sys_dirent) + name_len;

            switch (st.mode & STAT_IFMT)
            {
            case STAT_IFREG:
                dirent->type = H_DT_REG;
                break;
            case STAT_IFDIR:
                dirent->type = H_DT_DIR;
                break;
            case STAT_IFBLK:
                dirent->type = H_DT_BLK;
                break;
            case STAT_IFCHR:
                dirent->type = H_DT_CHR;
                break;
            case STAT_IFIFO:
                dirent->type = H_DT_FIFO;
                break;
            case STAT_IFLNK:
                dirent->type = H_DT_LNK;
                break;
            case STAT_IFSOCK:
                dirent->type = H_DT_SOCK;
                break;
            }
            
            strcpy(dirent->name, name); // Copy the name into the dirent

            // Write the dirent to the user space buffer
            if (task.memcpy(dirent_loc + bytes_read, dirent, sizeof(sys_dirent) + name_len) < 0)
            {
                _free(dirent);
                error = H_EFAULT;
                ok = false;
                break;
            }

            _free(dirent);
            bytes_read += sizeof(sys_dirent) + name_len;
        }

        // Free the list of entries
        for (const char * const *entry = list; *entry != nullptr; ++entry)
        {
            dealloc(*entry);
        }
        dealloc(list);

        if (!ok)
        {
            return cvt_error();
        }

        // Update file position
        file->seek(bytes_read, H_SEEK_CUR);

        return bytes_read; // Return the number of bytes read
    }

    int32_t sys_getcwd(Task &task, uint32_t buf_loc, uint32_t size)
    {
        char *cwd = task.getcwd();

        size_t cwd_len = strlen(cwd);

        if (cwd_len + 1 > size)
        {
            dealloc(cwd);
            return -H_ERANGE;
        }
        
        if (task.memcpy(buf_loc, cwd, cwd_len + 1) < 0)
        {
            dealloc(cwd);
            return cvt_error();
        }

        dealloc(cwd);
        
        return buf_loc;
    }

    int32_t sys_ftruncate64(Task &task, int32_t fd, uint32_t off_high, uint32_t off_low)
    {
        int64_t off = ((uint64_t)off_high << 32) | off_low;

        BaseTaskFD *file = task.get_fd(fd);
        if (!file)
            return cvt_error();
        
        if (file->truncate(off) < 0)
            return cvt_error();
        return 0;
    }

    int32_t sys_fchmod(Task &task, int32_t task_fd, uint32_t mode)
    {
        int fd = task.get_vfs_fd(task_fd);
        if (fd < 0)
            return cvt_error();
        if (vfs.chmod(fd, mode) < 0)
            return cvt_error();
        return 0;
    }

    int32_t sys_fchmodat(Task &task, int32_t dirfd, uint32_t path_loc, uint32_t mode, int32_t flags)
    {
        // We don't support AT_SYMLINK_NOFOLLOW
        if (flags != 0)
            return -H_ENOTSUP;
        
        char *path = task.mem_get_string(path_loc);
        if (!path)
            return cvt_error();
        
        int file = task.open_rel_file(dirfd, path, OPEN_WRONLY);
        dealloc(path);
        if (file < 0)
            return cvt_error();
        
        int res = vfs.chmod(file, mode);
        vfs.close(file);

        if (res < 0)
            return cvt_error();
        return 0;
    }

    int32_t sys_fchownat(Task &task, int32_t dirfd, uint32_t path_loc, uint32_t uid, uint32_t gid, int32_t flags)
    {
        char *path = task.mem_get_string(path_loc);
        if (!path)
            return cvt_error();
        
        int rel_fd = task.open_rel_fd(dirfd, path);
        
        int res = flags & H_AT_SYMLINK_NOFOLLOW ? vfs.lchownat(rel_fd, path, uid, gid) : vfs.chownat(rel_fd, path, uid, gid);
        dealloc(path);
        vfs.close(rel_fd);

        if (res < 0)
            return cvt_error();
        return 0;
    }

    int32_t sys_fchown(Task &task, int32_t task_fd, uint32_t uid, uint32_t gid)
    {
        int fd = task.get_vfs_fd(task_fd);
        if (fd < 0)
            return cvt_error();
        
        if (vfs.chown(fd, uid, gid) < 0)
            return cvt_error();
        return 0;
    }

    int32_t sys_getrandom(Task &task, uint32_t buf_loc, uint32_t size, uint32_t flags)
    {
        if (task.mem_is_mapped(buf_loc, size) != 1)
            return -H_EFAULT;
        // Flags aren't used
        (void)flags;
        for (uint32_t it = buf_loc; it < buf_loc + size; ++it)
            if (task.copy_to_memory(it, (uint8_t)rand()) < 0)
                return cvt_error();
        return size;
    }

    int32_t sys_futex_time64(Task &task, uint32_t uaddr_loc, int32_t futex_op, uint32_t val, uint32_t val2_timeoutloc, uint32_t uaddr2_loc, uint32_t val3)
    {
        if (uaddr_loc % 4 != 0)
            return -H_EINVAL;
        
        futex_op &= H_FUTEX_CMD_MASK;

        uint32_t value;
        if (task.copy_from_memory(value, uaddr_loc) < 0)
            return cvt_error();

        switch (futex_op)
        {
        case H_FUTEX_WAIT:
            val3 = UINT32_MAX;
            [[fallthrough]];
        case H_FUTEX_WAIT_BITSET:
            if (val3 == 0) // no bits set
                return -H_EINVAL;
            if (val2_timeoutloc)
                return -H_ENOTSUP; // TODO: futex wait timeout
            if (value != val)
                return -H_EAGAIN;

            // TODO: better way
            static_assert(sizeof(void *) >= sizeof(uint32_t));

            // Block until woken up
            if (task.futex_wait(uaddr_loc, [](void *arg){
                uint32_t tid = (uint32_t)(uintptr_t)arg;
                Task *task = Task::get_task(tid);
                if (task)
                    task->end_block();
            }, (void *)(uintptr_t)task.get_tid(), val3) < 0) // pass TID instead of task to avoid dangling pointer when task dies
                return cvt_error();
            task.block([](Task &, uint64_t){}, 0);
            return 0;
        case H_FUTEX_WAKE:
            val3 = UINT32_MAX;
            [[fallthrough]];
        case H_FUTEX_WAKE_BITSET:
            if (val3 == 0) // no bits set
                return -H_EINVAL;
            return cvt_error(task.futex_wake(uaddr_loc, val, val3));
        case H_FUTEX_CMP_REQUEUE:
            if (value != val3)
                return -H_EAGAIN;
            [[fallthrough]];
        case H_FUTEX_REQUEUE:
            return cvt_error(task.futex_requeue(uaddr_loc, val, uaddr2_loc, val2_timeoutloc));
        case H_FUTEX_WAKE_OP:
        {
            uint32_t oldval;
            if (task.copy_from_memory(oldval, uaddr2_loc) < 0)
                return cvt_error();
            
            uint32_t op, oparg, cmp, cmparg;
            op = (val3 >> 28) & 0xF;
            cmp = (val3 >> 24) & 0xF;
            oparg = (val3 >> 12) & 0xFFF;
            cmparg = val3 & 0xFFF;

            if (op & H_FUTEX_OP_OPARG_SHIFT)
            {
                if (oparg >= 32)
                    return -H_EINVAL;
                op &= ~H_FUTEX_OP_OPARG_SHIFT;
                oparg = 1 << oparg;
            }

            uint32_t newval = oldval;
            switch (op)
            {
            case H_FUTEX_OP_SET: newval = oparg; break;
            case H_FUTEX_OP_ADD: newval += oparg; break;
            case H_FUTEX_OP_OR: newval |= oparg; break;
            case H_FUTEX_OP_ANDN: newval &= ~oparg; break;
            case H_FUTEX_OP_XOR: newval ^= oparg; break;
            default: return -H_EINVAL;
            }
            if (task.copy_to_memory(uaddr2_loc, newval) < 0)
                return cvt_error();
            
            int woken1 = 0, woken2 = 0;
            woken1 = task.futex_wake(uaddr_loc, val);
            if (woken1 < 0)
                return cvt_error();

            bool cmp_ok = false;

            switch (cmp)
            {
            case H_FUTEX_OP_CMP_EQ: cmp_ok = (oldval == cmparg); break;
            case H_FUTEX_OP_CMP_NE: cmp_ok = (oldval != cmparg); break;
            case H_FUTEX_OP_CMP_LT: cmp_ok = (oldval < cmparg); break;
            case H_FUTEX_OP_CMP_LE: cmp_ok = (oldval <= cmparg); break;
            case H_FUTEX_OP_CMP_GT: cmp_ok = (oldval > cmparg); break;
            case H_FUTEX_OP_CMP_GE: cmp_ok = (oldval >= cmparg); break;
            default: return -H_EINVAL;
            }

            if (cmp_ok)
            {
                woken2 = task.futex_wake(uaddr2_loc, val2_timeoutloc);
                if (woken2 < 0)
                    return cvt_error();
            }

            return woken1 + woken2;
        }
        default:
            return -H_EINVAL;
        }
    }
} // namespace Hamster

