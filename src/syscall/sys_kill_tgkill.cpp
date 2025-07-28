// Hamster kill and tgkill system calls

#include <syscall/syscall.hpp>
#include <process/scheduler.hpp>
#include <abi/structs.hpp>
#include <abi/values.hpp>
#include <errno/errno.h>
#include <cassert>

namespace Hamster
{
    int32_t sys_kill(int32_t pid, int32_t sig)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task);

        int uid = current_task->process->obj.uid;
        int euid = current_task->process->obj.euid;

        sys_siginfo siginfo;
        siginfo.signo = sig;
        siginfo.errno_value = 0;
        siginfo.code = H_SI_USER;
        siginfo.fields.kill.pid = current_task->get_pid();
        siginfo.fields.kill.uid = euid;

        if (pid > 0)
        {
            // Send to process
            Process *process = scheduler.get_process(pid);
            if (!process)
            {
                error = ESRCH;
                return cvt_error();
            }

            if (uid != 0 && uid != process->uid && euid != 0 && euid != process->euid)
            {
                error = EPERM;
                return cvt_error();
            }

            if (sig && (process->send_signal(siginfo) < 0))
            {
                return cvt_error();
            }
            return 0;
        }
        else if (pid == 0)
        {
            // Send to processes in the same process group
            bool at_least_one_sent = false;
            ProcessGroup &pg = current_task->process->obj.pg->obj;
            for (auto &proc : pg.processes)
            {
                if (uid != 0 && uid != proc->uid && euid != 0 && euid != proc->euid)
                    continue; // Skip processes that the user cannot signal
                at_least_one_sent = true;
                if (sig && (proc->send_signal(siginfo) < 0))
                {
                    return cvt_error();
                }
            }
            return 0;
        }
        else if (pid == -1)
        {
            bool at_least_one_sent = false;
            // Send to all processes
            for (auto &task : scheduler.get_tasks())
            {
                auto &proc = task.second->process->obj;
                if (proc.pid == 1) // Skip init process
                    continue;
                if (uid != 0 && uid != proc.uid && euid != 0 && euid != proc.euid)
                    continue; // Skip processes that the user cannot signal
                at_least_one_sent = true;
                if (sig && (proc.send_signal(siginfo) < 0))
                {
                    return cvt_error();
                }
            }

            if (!at_least_one_sent)
            {
                error = EPERM;
                return cvt_error();
            }

            return 0;
        }
        else
        {
            // Send to all processes in the specified process group
            bool at_least_one_sent = false;
            for (auto &task : scheduler.get_tasks())
            {
                auto &proc = task.second->process->obj;
                if (proc.get_pgid() != -pid)
                    continue; // Not in the specified process group
                if (uid != 0 && uid != proc.uid && euid != 0 && euid != proc.euid)
                    continue; // Skip processes that the user cannot signal
                at_least_one_sent = true;
                if (sig && (proc.send_signal(siginfo) < 0))
                {
                    return cvt_error();
                }
            }

            if (!at_least_one_sent)
            {
                error = EPERM;
                return cvt_error();
            }

            return 0;
        }
    }

    int32_t sys_tgkill(int32_t /* tgid - not used */, int32_t tid, int32_t sig)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task);

        int uid = current_task->process->obj.uid;
        int euid = current_task->process->obj.euid;

        sys_siginfo siginfo;
        siginfo.signo = sig;
        siginfo.errno_value = 0;
        siginfo.code = H_SI_USER;
        siginfo.fields.kill.pid = current_task->get_pid();
        siginfo.fields.kill.uid = euid;

        if (tid > 0)
        {
            // Send to thread
            Task *task = scheduler.get_task(tid);
            if (!task)
            {
                error = ESRCH;
                return cvt_error();
            }
            if (uid != 0 && uid != task->process->obj.uid && euid != 0 && euid != task->process->obj.euid)
            {
                error = EPERM;
                return cvt_error();
            }
            if (sig && (task->send_signal(siginfo) < 0))
            {
                return cvt_error();
            }
            return 0;
        }
        else
        {
            error = EINVAL;
            return cvt_error();
        }
    }
}
