// Hamster exit syscall

#include <syscall/syscall.hpp>
#include <process/scheduler.hpp>
#include <abi/values.hpp>

namespace Hamster
{
    int32_t sys_exit(int32_t status)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        current_task->exit(make_wait_exited(status));

        return 0;
    }

    int32_t sys_exit_group(int32_t status)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        for (Task *t : current_task->process->obj.tasks)
        {
            t->exit(make_wait_exited(status));
        }

        return 0;
    }
} // namespace Hamster

