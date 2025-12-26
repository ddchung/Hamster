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
        return memory->ms.unmap_all();
    }

    void Task::release_mem()
    {
        emulator.flush_caches();

        if (is_vfork && parent)
            parent->interrupt_block();

        if (memory.refcount() > 1)
            memory.construct();
        else
            munmap_all();
    }

    int Task::mprotect(uint32_t addr, uint32_t size, uint8_t permissions)
    {
        return memory->ms.mprotect(addr, size, permissions);
    }

    int Task::futex_wait(uint32_t addr, void (*callback)(), uint32_t bitset)
    {
        return memory->ms.futex_wait(addr, callback, bitset);
    }

    int Task::futex_wait(uint32_t addr, void (*callback)(void *), void *arg, uint32_t bitset)
    {
        return memory->ms.futex_wait(addr, callback, arg, bitset);
    }

    int Task::futex_wake(uint32_t addr, uint32_t count, uint32_t bitset)
    {
        return memory->ms.futex_wake(addr, count, bitset);
    }

    int Task::futex_requeue(uint32_t wake_addr, uint32_t wake_count, uint32_t requeue_addr, uint32_t requeue_count)
    {
        return memory->ms.futex_requeue(wake_addr, wake_count, requeue_addr, requeue_count);
    }

    uint32_t Task::mmap(uint32_t addr, uint32_t length, uint8_t prot, int flags, int fd, uint32_t offset)
    {
        int64_t offset64 = (int64_t)offset * 4096;

        if (flags & (H_MAP_FIXED | H_MAP_FIXED_NOREPLACE) && addr & (HAMSTER_PAGE_SIZE - 1))
        {
            error = H_EINVAL;
            return UINT32_MAX;
        }

        int vfs_fd = 0;
        if (!(flags & H_MAP_ANONYMOUS))
        {
            // file mapping
            vfs_fd = get_vfs_fd(fd);
            if (vfs_fd < 0 || !vfs.is_valid_fd(vfs_fd))
            {
                error = H_EBADF;
                return UINT32_MAX;
            }
            int64_t file_size = vfs.size(vfs_fd);
            if (file_size < 0)
            {
                error = H_EIO;
                return UINT32_MAX;
            }
        }

        // Round down to the nearest page boundary
        uint32_t internal_addr = addr & ~(HAMSTER_PAGE_SIZE - 1);

        if (memory->ms.how_many_mapped(internal_addr, length) > 0)
        {
            // Already mapped
            if (flags & H_MAP_FIXED_NOREPLACE)
            {
                error = H_EEXIST;
                return UINT32_MAX;
            }
            else if (flags & H_MAP_FIXED)
            {
                if (memory->ms.unmap(internal_addr, length) < 0)
                {
                    error = H_EPERM;
                    return UINT32_MAX;
                }
            }
            else
            {
                // Allocate new region by triggering code below
                internal_addr = 0;
            }
        }

        // Allocate new region if needed
        if (internal_addr == 0)
            internal_addr = memory->ms.allocate(length);
        assert(internal_addr != 0);

        // Do the mapping

        if (flags & H_MAP_SHARED)
        {
            if (flags & H_MAP_ANONYMOUS)
                memory->ms.map_shared_anonymous(internal_addr, length, prot);
            else
                memory->ms.map_shared_file(internal_addr, vfs_fd, offset64, length, prot);
        }
        else
        {
            if (flags & H_MAP_ANONYMOUS)
                memory->ms.map_anonymous(internal_addr, length, prot);
            else
                memory->ms.map_private_file(internal_addr, vfs_fd, offset64, length, prot);
        }

        return internal_addr;
    }
} // namespace Hamster

