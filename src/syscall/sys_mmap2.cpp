// Hamster mmap2 system call

#include <syscall/syscall.hpp>
#include <process/scheduler.hpp>
#include <filesystem/vfs.hpp>
#include <abi/values.hpp>
#include <errno/errno.h>

namespace Hamster
{
    int32_t sys_mmap2(uint32_t addr, uint32_t length, uint32_t prot, uint32_t flags, int32_t fd, uint32_t offset)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        MemorySpace &memory = current_task->get_memory();

        // "the final argument specifies the offset into the file in 4096-byte units (instead of bytes, as is done by mmap(2))"
        int64_t offset64 = (int64_t)offset * 4096;  

        // Do some checking
        
        if (flags & (H_MAP_FIXED | H_MAP_FIXED_NOREPLACE) && addr & (HAMSTER_PAGE_SIZE - 1))
            return -H_EINVAL;

        int vfs_fd = 0;
        if (!(flags & H_MAP_ANONYMOUS))
        {
            // file mapping
            vfs_fd = current_task->get_vfs_fd(fd);
            if (vfs_fd < 0 || !vfs.is_valid_fd(vfs_fd))
                return -H_EBADF;
            int64_t file_size = vfs.size(vfs_fd);
            if (file_size < 0)
                return -H_EIO;
        }

        uint8_t internal_perms = 0
            | (prot & H_PROT_READ ? PERM_READ : 0)
            | (prot & H_PROT_WRITE ? PERM_WRITE : 0)
            | (prot & H_PROT_EXEC ? PERM_EXEC : 0);

        // Round down to the nearest page boundary
        uint32_t internal_addr = addr & ~(HAMSTER_PAGE_SIZE - 1);

        if (memory.how_many_mapped(internal_addr, length) > 0)
        {
            // Already mapped
            if (flags & H_MAP_FIXED_NOREPLACE)
                return -H_EEXIST;
            else if (flags & H_MAP_FIXED)
            {
                if (memory.unmap(internal_addr, length) < 0)
                    return -H_EPERM; // Failed to unmap existing mapping
            }
            else
            {
                // Allocate new region by triggering code below
                internal_addr = 0;
            }
        }

        // Allocate new region if needed
        if (internal_addr == 0)
            internal_addr = memory.allocate(length);
        assert(internal_addr != 0);

        // Do the mapping

        if (flags & H_MAP_SHARED)
        {
            if (flags & H_MAP_ANONYMOUS)
                memory.map_shared_anonymous(internal_addr, length, internal_perms);
            else
                memory.map_shared_file(internal_addr, vfs_fd, offset64, length, internal_perms);
        }
        else
        {
            if (flags & H_MAP_ANONYMOUS)
                memory.map_anonymous(internal_addr, length, internal_perms);
            else
                memory.map_private_file(internal_addr, vfs_fd, offset64, length, internal_perms);
        }

        return internal_addr;
    }
} // namespace Hamster

