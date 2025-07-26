// Hamster exit syscall

#include <syscall/syscall.hpp>
#include <process/scheduler.hpp>
#include <abi/values.hpp>

namespace Hamster
{
    int32_t sys_exit(int32_t status)
    {
        Task *current_task = scheduler.get_current_task();
        if (!current_task)
        {
            return cvt_error(); // No current task
        }

        current_task->exit(make_wait_exited(status));

        return 0;
    }
} // namespace Hamster

