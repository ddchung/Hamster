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
} // namespace Hamster

