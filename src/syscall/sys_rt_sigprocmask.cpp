// Hamster rt_sigprocmask system call

#include <syscall/syscall.hpp>
#include <process/scheduler.hpp>
#include <abi/structs.hpp>
#include <abi/values.hpp>
#include <errno/errno.h>

namespace Hamster
{
    int32_t sys_rt_sigprocmask(int32_t how, uint32_t set_loc, uint32_t oldset_loc, uint32_t sigsetsize)
    {
        Task *current_task = scheduler.get_current_task();
        if (!current_task)
        {
            error = ESRCH; // No current task
            return -1;
        }

        if (sigsetsize != sizeof(sys_sigset))
        {
            error = EINVAL; // Invalid sigset size
            return -1;
        }

        if (oldset_loc)
        {
            sys_sigset sigset;
            sigset.sig[0] = ~current_task->sig_mask & 0xFFFFFFFF;
            sigset.sig[1] = (~current_task->sig_mask >> 32) & 0xFFFFFFFF;
            if (current_task->copy_to_user(sigset, oldset_loc) < 0)
            {
                error = EFAULT; // Bad address
                return -1;
            }
        }

        if (set_loc)
        {
            uint64_t new_mask = 0;
            if (current_task->copy_from_user(new_mask, set_loc) < 0)
            {
                error = EFAULT; // Bad address
                return -1;
            }

            switch (how)
            {
            case H_SIG_BLOCK:
                current_task->sig_mask &= ~new_mask;
                break;
            case H_SIG_UNBLOCK:
                current_task->sig_mask |= new_mask;
                break;
            case H_SIG_SETMASK:
                current_task->sig_mask = ~new_mask;
                break;
            default:
                return -EINVAL;
            }
        }

        return 0;
    }
} // namespace Hamster

