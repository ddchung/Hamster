// Hamster brk system call

#include <syscall/syscall.hpp>
#include <process/scheduler.hpp>
#include <abi/structs.hpp>
#include <errno/errno.h>

namespace Hamster
{
    int32_t sys_brk(uint32_t new_brk)
    {
        _trace("sys_brk(%x)\n", new_brk);

        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        auto &brk = current_task->program_brk->obj;

        if (new_brk)
            brk = new_brk;
        return brk;
    }
} // namespace Hamster

