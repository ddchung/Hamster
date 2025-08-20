// Hamster munmap system call

#include <syscall/syscall.hpp>
#include <process/scheduler.hpp>
#include <errno/errno.h>

namespace Hamster
{
    int32_t sys_munmap(uint32_t addr, uint32_t length)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        MemorySpace &memory = current_task->get_memory();

        // Round down to the nearest page boundary
        uint32_t internal_addr = addr & ~(HAMSTER_PAGE_SIZE - 1);
        uint32_t internal_length = (length + HAMSTER_PAGE_SIZE - 1) & ~(HAMSTER_PAGE_SIZE - 1);

        // Check if the region is mapped
        if (memory.is_mapped(internal_addr, internal_length) <= 0)
            return -EINVAL;

        // Unmap the region
        if (memory.unmap(internal_addr, internal_length) < 0)
            return -EIO;

        return 0;
    }
} // namespace Hamster

