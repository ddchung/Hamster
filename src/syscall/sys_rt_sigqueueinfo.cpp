// Hamster rt_sigqueueinfo system call

#include <syscall/syscall.hpp>
#include <process/scheduler.hpp>
#include <abi/structs.hpp>
#include <abi/values.hpp>
#include <errno/errno.h>
#include <cassert>

namespace Hamster
{
    int32_t sys_rt_sigqueueinfo(int32_t tgid, int32_t sig, uint32_t info_loc)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task);

        if (sig < 1 || sig >= H_SIGRTMAX)
        {
            error = EINVAL;
            return cvt_error();
        }

        sys_siginfo siginfo = {};

        if (current_task->copy_from_user(siginfo, info_loc) < 0)
        {
            error = EFAULT;
            return cvt_error();
        }

        siginfo.signo = sig;
        siginfo.errno_value = 0;

        if (siginfo.code >= 0 || siginfo.code == H_SI_TKILL)
        {
            // Invalid code
            return -EPERM;
        }

        // Send to process

        Process *process = scheduler.get_process(tgid);
        if (!process)
        {
            error = ESRCH;
            return cvt_error();
        }

        return process->send_signal(siginfo);
    }

    int32_t sys_rt_tgsigqueueinfo(int32_t /* tgid - not used */, int32_t tid, int32_t sig, uint32_t info_loc)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task);

        if (sig < 1 || sig >= H_SIGRTMAX)
        {
            error = EINVAL;
            return cvt_error();
        }

        sys_siginfo siginfo = {};

        if (current_task->copy_from_user(siginfo, info_loc) < 0)
        {
            error = EFAULT;
            return cvt_error();
        }

        siginfo.signo = sig;
        siginfo.errno_value = 0;

        if (siginfo.code >= 0 || siginfo.code == H_SI_TKILL)
        {
            // Invalid code
            return -EPERM;
        }

        // Send to thread

        Task *task = scheduler.get_task(tid);
        if (!task)
        {
            error = ESRCH;
            return cvt_error();
        }

        return task->send_signal(siginfo);
    }
}
