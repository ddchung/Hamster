// Hamster clone system call

#include <syscall/syscall.hpp>
#include <process/scheduler.hpp>
#include <memory/allocator.hpp>
#include <cassert>

namespace Hamster
{
    int32_t sys_clone(uint32_t flags, uint32_t stack_loc, uint32_t ptid_loc,
                      uint32_t newtls, uint32_t ctid_loc)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr && "No current task");

        // Create a new task with the specified flags
        Task *new_task = current_task->clone(flags);

        if (!new_task)
        {
            return cvt_error();
        }

        // Set the new task's stack pointer

        if (stack_loc)
            new_task->emulator.x[2] = stack_loc; // sp

        // Set the return code for the new task
        new_task->emulator.x[10] = 0; // return 0

        // Set the thread pointer if provided
        if (flags & H_CLONE_SETTLS)
            new_task->emulator.x[3] = newtls; // tp

        // Add to the scheduler
        uint32_t tid = scheduler.add_task(new_task);
        if (tid == 0)
        {
            dealloc(new_task);
            return cvt_error();
        }

        return tid;
    }
} // namespace Hamster

