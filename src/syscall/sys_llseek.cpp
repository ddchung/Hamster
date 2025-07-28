// Hamster llseek system call
// Note: not to be confused with the lseek wrapper in libc

#include <syscall/syscall.hpp>
#include <filesystem/vfs.hpp>
#include <process/scheduler.hpp>
#include <abi/structs.hpp>
#include <errno/errno.h>

namespace Hamster
{
    int32_t sys_llseek(int32_t fd, uint32_t off_high, uint32_t off_low, uint32_t result_loc, int32_t whence)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        int vfs_fd = current_task->get_vfs_fd(fd);
        if (vfs_fd < 0)
            return cvt_error();

        // Check if the result location is valid
        if (result_loc == 0)
        {
            error = EFAULT; // Bad address
            return -1;
        }

        int64_t offset = ((int64_t)off_high << 32) | (int64_t)off_low;
        int64_t new_offset = vfs.seek(vfs_fd, offset, whence);
        if (new_offset < 0)
        {
            return cvt_error();
        }

        // Copy the new offset back to userspace
        if (current_task->memory->obj.memory.memcpy(result_loc, &new_offset, sizeof(new_offset)) < 0)
        {
            error = EFAULT; // Bad address
            return -1;
        }

        return 0;
    }
} // namespace Hamster

