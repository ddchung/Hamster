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

        error = 0;
        
        if ((arg % HAMSTER_PAGE_SIZE) > HAMSTER_PAGE_SIZE - HAMSTER_MAX_IOCTL_SIZE)
        {
            task.memcpy(IOCTL_BUF, arg, HAMSTER_MAX_IOCTL_SIZE);
            ioarg.p = IOCTL_BUF;
            res = fd->ioctl(op, ioarg);
            task.memcpy(arg, IOCTL_BUF, HAMSTER_MAX_IOCTL_SIZE);
        }
        else
        {
            ioarg.p = task.mem_make_iterator(arg);
            res = fd->ioctl(op, ioarg);
        }

        if (error)
            return cvt_error();
        return res;
    }
} // namespace Hamster

