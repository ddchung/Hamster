// Hamster clock_settime_time64 system call

#include <syscall/syscall.hpp>
#include <process/scheduler.hpp>
#include <platform/clock_realtime.hpp>
#include <abi/values.hpp>
#include <abi/structs.hpp>
#include <errno/errno.h>

namespace Hamster
{
    int32_t sys_clock_settime64(int32_t clock_id, uint32_t ts_loc)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        sys_timespec ts;

        if (!ts_loc)
            return -H_EFAULT;

        if (current_task->copy_from_user(ts, ts_loc) < 0)
            return cvt_error();

        uint64_t ts_ms = ts.sec * 1000 + (ts.nsec + 500'000) / 1'000'000;
        uint64_t now = _get_sys_time();

        switch (clock_id)
        {
        case H_CLOCK_REALTIME:
            clock_rt_offset = now - ts_ms;
            break;
        case H_CLOCK_MONOTONIC:
            return -H_EPERM;
        default:
            return -H_EINVAL;
        }

        return 0;
    }
} // namespace Hamster

