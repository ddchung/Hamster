// Hamster system calls for Thread, Process, Process group and session IDs

#include <syscall/syscall.hpp>
#include <process/scheduler.hpp>
#include <errno/errno.h>
#include <algorithm>

namespace Hamster
{
    int32_t sys_gettid()
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        return current_task->tid;
    }

    int32_t sys_getpid()
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        return current_task->get_pid();
    }

    int32_t sys_getppid()
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        return current_task->process->obj.ppid;
    }

    int32_t sys_getpgid(int32_t pid)
    {
        if (pid == 0)
        {
            Task *current_task = scheduler.get_current_task();
            assert(current_task != nullptr);

            return current_task->get_pgid();
        }
        else
        {
            Process *process = scheduler.get_process(pid);
            if (!process)
            {
                error = H_ESRCH;
                return cvt_error();
            }

            return process->get_pgid();
        }
    }

    int32_t sys_getsid()
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        return current_task->get_sid();
    }

    int32_t sys_setpgid(int32_t spid, int32_t spgid)
    {
        if (spid < 0 || spgid < 0)
        {
            error = H_EINVAL;
            return cvt_error();
        }

        uint32_t pid = spid;
        uint32_t pgid = spgid;
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        Process *process;

        if (pid == 0)
        {
            process = &current_task->process->obj;
        }
        else
        {
            process = scheduler.get_process(pid);
            if (process == nullptr)
            {
                error = H_ESRCH;
                return cvt_error();
            }

            // Check if it is the calling process or one of its children
            if (process->pid != current_task->get_pid() && process->ppid != current_task->get_pid())
            {
                error = H_ESRCH;
                return cvt_error();
            }
        }
        if (pgid != 0 && pgid != process->pid)
        {
            ProcessGroup *pg = scheduler.get_process_group(pgid);
            if (pg == nullptr)
            {
                error = H_ESRCH;
                return cvt_error();
            }

            // Check if the old and new process groups belong to the same session
            if (process->get_sid() != pg->session->obj.sid)
            {
                error = H_EPERM;
                return cvt_error();
            }

            // FIXME: pointer cast
            process->join_process_group((TaskMember<ProcessGroup>*)pg);
        }
        else
        {
            // Set the process group ID to the process ID
            auto old_session = process->pg ? process->pg->obj.session : nullptr;
            process->join_process_group(nullptr);
            process->pg = make_task_member<ProcessGroup>();
            process->pg->obj.pgid = process->pid;
            process->pg->obj.processes.push_back(process);
            process->pg->obj.session = old_session ? ref_task_member(old_session) : make_task_member<Session>();
            process->pg->obj.session->obj.pgroups.push_back(&process->pg->obj);
        }
        return 0;
    }

    int32_t sys_getsid(int32_t pid)
    {
        if (pid == 0)
        {
            Task *current_task = scheduler.get_current_task();
            assert(current_task != nullptr);

            return current_task->get_sid();
        }
        else
        {
            Process *process = scheduler.get_process(pid);
            if (!process)
            {
                error = H_ESRCH;
                return cvt_error();
            }

            return process->get_sid();
        }
    }

    int32_t sys_setsid()
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        Process &process = current_task->process->obj;

        if (process.get_pgid() == process.pid)
        {
            // Already a process group leader
            error = H_EPERM;
            return cvt_error();
        }

        process.join_process_group(nullptr);
        process.pg = make_task_member<ProcessGroup>();
        process.pg->obj.pgid = process.pid;
        process.pg->obj.processes.push_back(&process);
        process.pg->obj.session = make_task_member<Session>();
        process.pg->obj.session->obj.sid = process.pid;

        return process.get_sid();
    }

} // namespace Hamster

