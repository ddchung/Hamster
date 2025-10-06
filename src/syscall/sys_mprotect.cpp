// Hamster mprotect system call

#include <syscall/syscall.hpp>
#include <process/scheduler.hpp>
#include <errno/errno.h>

namespace Hamster
{
    int32_t sys_mprotect(uint32_t addr, uint32_t size, int32_t prot)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        MemorySpace &memory = current_task->get_memory();

        if (!memory.is_mapped(addr, size))
            return -H_ENOMEM;
        
        if (prot & (H_PROT_GROWSUP | H_PROT_GROWSDOWN))
        {
            // We don't support these
            return -H_ENOTSUP;
        }

        return memory.mprotect(addr, size, prot & 07);
    }
} // namespace Hamster

