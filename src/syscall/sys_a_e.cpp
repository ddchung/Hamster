// Hamster A-E system calls

#include <syscall/syscall.hpp>
#include <process/task.hpp>
#include <abi/values.hpp>
#include <abi/structs.hpp>

namespace Hamster
{
    int32_t sys_exit(Task &task, int32_t status)
    {
        // Exit only this thread if multithreaded, or clean up whole process if singlethreaded
        if (task.get_process()->num_tasks() > 1)
            task.exit();
        else
            task.get_process()->exit(make_wait_exited(status));
        return 0;
    }

    int32_t sys_exit_group(Task &task, int32_t status)
    {
        task.get_process()->exit(make_wait_exited(status));
        return 0;
    }
} // namespace Hamster

