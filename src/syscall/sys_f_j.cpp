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
} // namespace Hamster

