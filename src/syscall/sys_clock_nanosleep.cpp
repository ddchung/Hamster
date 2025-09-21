// Hamster clock_nanosleep_time64 system call

#include <syscall/syscall.hpp>
#include <process/scheduler.hpp>
#include <platform/clock_realtime.hpp>
#include <abi/values.hpp>
#include <abi/structs.hpp>
#include <errno/errno.h>
#include <cinttypes>

namespace Hamster
{
    namespace
    {
        void poll_sleep(Task &task)
        {
            // Restore the `clock_id` argument
            int32_t clock_id = task.blocking_operation_saved[0];
            task.emulator.x[10] = clock_id;

            int32_t res = syscall(sys_clock_nanosleep_time64);
            if (res == -H_EAGAIN)
                // Continue sleeping
                return;
            
            // return to userspace
            task.emulator.x[10] = res;
            task.blocking_operation = nullptr;
        }
    } // namespace
    

    int32_t sys_clock_nanosleep_time64(int32_t clock_id, int32_t flags, uint32_t req_loc, uint32_t rem_loc)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        sys_timespec req;

        if (!req_loc)
            return -H_EINVAL;

        if (current_task->copy_from_user(req, req_loc) < 0)
            return cvt_error();

        uint64_t req_ms = req.sec * 1000 + req.nsec / 1000000;
        uint64_t now = _get_sys_time();

        if (flags & H_TIMER_ABSTIME)
        {
            switch (clock_id)
            {
            case H_CLOCK_REALTIME:
                now += clock_rt_offset;
                [[fallthrough]];
            case H_CLOCK_MONOTONIC:
                if (now >= req_ms)
                    return 0; // passed requested timepoint
                break;
            default:
                return -H_EINVAL;
            }
        }
        else if (current_task->last_tick + req_ms <= now)
            return 0; // Done sleeping
        
        // Not done sleeping
        current_task->blocking_operation = poll_sleep;
        current_task->blocking_operation_saved[0] = clock_id;

        // note: poll_sleep intercepts this H_EAGAIN, so it will never reach userspace
        return -H_EAGAIN;
    }
} // namespace Hamster

