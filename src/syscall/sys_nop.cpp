// Hamster system calls that do nothing or almost nothing

#include <syscall/syscall.hpp>
#include <process/scheduler.hpp>
#include <errno/errno.h>

namespace Hamster
{
    int32_t sys_sched_yield()
    {
        // We won't implement this
        return 0;
    }

    int32_t sys_prctl(int32_t option, uint32_t arg2,
                          uint32_t arg3, uint32_t arg4,
                          uint32_t arg5)
    {
        // Do nothing for now
        return -EINVAL;
    }
} // namespace Hamster
