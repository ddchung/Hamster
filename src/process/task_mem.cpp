// Hamster task memory management and utility functions

#include <process/task.hpp>

namespace Hamster
{
    char *Task::mem_get_string(uint32_t addr)
    {
        return memory->ms.get_string(addr);
    }

    const void *Task::mem_make_iterator_read(uint32_t addr)
    {
        return memory->ms.make_iterator_read(addr);
    }

    void *Task::mem_make_iterator(uint32_t addr)
    {
        return memory->ms.make_iterator(addr);
    }

    int8_t Task::mem_get_permissions(uint32_t addr, uint32_t size)
    {
        return memory->ms.get_permissions(addr, size);
    }

    uint32_t Task::mbrk(uint32_t brk)
    {
        if (brk == 0)
            return memory->brk;

        if (brk < memory->brk)
        {
            // Shrink brk

            // Note: align `brk` up to page size. `unmap` rounds down automatically
            if (memory->ms.unmap(brk + (HAMSTER_PAGE_SIZE - 1),
                                 memory->brk - brk) < 0)
            {
                return memory->brk;
            }
        }
        else if (brk > memory->brk)
        {
            // Expand brk

            // same thing for `memory->brk`
            if (memory->ms.map_anonymous(memory->brk + (HAMSTER_PAGE_SIZE - 1),
                                        brk - memory->brk, PERM_READ | PERM_WRITE) < 0)
            {
                return memory->brk;
            }
        }

        memory->brk = brk;
        return memory->brk;
    }

    int Task::sigaltstack(const sys_sigaltstack *new_stack, sys_sigaltstack *old_stack)
    {
        if (old_stack)
            *old_stack = alt_signal_stack;
        if (new_stack)
            alt_signal_stack = *new_stack;
        return 0;
    }

    void Task::set_tid_address(uint32_t tid_addr)
    {
        clear_child_tid = tid_addr;
    }

    void Task::set_robust_list(uint32_t head)
    {
        robust_list = head;
    }

    int Task::memcpy(uint32_t dest, const void *src, uint32_t n)
    {
        return memory->ms.memcpy(dest, src, n);
    }

    int Task::memcpy(void *dest, uint32_t src, uint32_t n)
    {
        return memory->ms.memcpy(dest, src, n);
    }

    int Task::memset(uint32_t addr, uint8_t value, uint32_t n)
    {
        return memory->ms.memset(addr, value, n);
    }

    int Task::mem_is_mapped(uint32_t addr, uint32_t size) const
    {
        return memory->ms.is_mapped(addr, size);
    }

    int Task::munmap(uint32_t addr, uint32_t size)
    {
        emulator.flush_caches();
        return memory->ms.unmap(addr, size);
    }

    int Task::munmap_all()
    {
        emulator.flush_caches();
        if (is_vfork && parent)
            parent->interrupt_block();
        return memory->ms.unmap_all();
    }

    int Task::mprotect(uint32_t addr, uint32_t size, uint8_t permissions)
    {
        return memory->ms.mprotect(addr, size, permissions);
    }

    int Task::futex_wait(uint32_t addr, void (*callback)())
    {
        return memory->ms.futex_wait(addr, callback);
    }

    int Task::futex_wake(uint32_t addr, uint32_t count)
    {
        return memory->ms.futex_wake(addr, count);
    }

    int Task::futex_requeue(uint32_t wake_addr, uint32_t wake_count, uint32_t requeue_addr, uint32_t requeue_count)
    {
        return memory->ms.futex_requeue(wake_addr, wake_count, requeue_addr, requeue_count);
    }
} // namespace Hamster

