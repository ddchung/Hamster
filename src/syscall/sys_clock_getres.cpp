// Hamster clock_getres_time64 system call

#include <syscall/syscall.hpp>
#include <process/scheduler.hpp>
#include <abi/structs.hpp>
#include <abi/values.hpp>
#include <errno/errno.h>

namespace Hamster
{
    int32_t sys_clock_getres_time64(int32_t clock_id, uint32_t res_loc)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        sys_timespec res;

        if (!res_loc)
            return -H_EFAULT;

        switch (clock_id)
        {
        case H_CLOCK_REALTIME:
        case H_CLOCK_MONOTONIC:
            res.nsec = 1'000'000; // 1ms
            res.sec = 0;
            break;
        default:
            return -H_EINVAL;
        }

        if (current_task->copy_to_user(res, res_loc) < 0)
            return cvt_error();
        return 0;
    }
} // namespace Hamster

