// Hamster clock_gettime64 system call

#include <syscall/syscall.hpp>
#include <process/scheduler.hpp>
#include <platform/clock_realtime.hpp>
#include <platform/platform.hpp>
#include <abi/structs.hpp>
#include <abi/values.hpp>
#include <errno/errno.h>

namespace Hamster
{
    int32_t sys_clock_gettime64(int32_t clock_id, uint32_t tp_loc)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        sys_timespec ts = {};

        if (!tp_loc)
            return -EFAULT;

        uint64_t now = _get_sys_time();

        switch (clock_id)
        {
        case H_CLOCK_REALTIME:
            ts.nsec = (now + clock_rt_offset) % 1000 * 1'000'000; // 1 million ms in ns
            ts.sec = (now + clock_rt_offset) / 1000;
            break;
        case H_CLOCK_MONOTONIC:
            ts.nsec += now % 1000 * 1'000'000;
            ts.sec += now / 1000;
            break;
        default:
            return -EINVAL;
        }

        if (current_task->copy_to_user(ts, tp_loc) < 0)
            return cvt_error();
        return 0;
    }
} // namespace Hamster
