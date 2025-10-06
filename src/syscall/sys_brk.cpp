// Hamster brk system call

#include <syscall/syscall.hpp>
#include <process/scheduler.hpp>
#include <abi/structs.hpp>
#include <errno/errno.h>

namespace Hamster
{
    int32_t sys_brk(uint32_t new_brk)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        auto &brk = current_task->program_brk->obj;

        if (new_brk)
        {
            if (new_brk < brk)
            {
                // Unmap some pages
                if (current_task->get_memory().unmap(new_brk + (HAMSTER_PAGE_SIZE - 1), brk - new_brk) < 0)
                {
                    error = H_EFAULT;
                    return -1;
                }
            }
            else
            {
                // Map more pages
                if (current_task->get_memory().map_anonymous(brk, new_brk - brk, PERM_READ | PERM_WRITE) < 0)
                {
                    error = H_EFAULT;
                    return -1;
                }
            }

            brk = new_brk;
        }
        return brk;
    }
} // namespace Hamster

