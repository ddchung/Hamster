// Hamster llseek system call
// Note: Not to be confused with the lseek wrapper function in userspace

#include <syscall/syscall.hpp>
#include <abi/syscall_id.hpp>
#include <process/thread.hpp>
#include <filesystem/vfs.hpp>
#include <errno/errno.h>

namespace Hamster
{
    int sys_llseek(Thread &thread)
    {
        int vfs_fd = deref_fd(thread, get_arg(thread, 0));
        int64_t offset = ((int64_t)get_arg(thread, 1) << 32) | (int64_t)get_arg(thread, 2);
        uint32_t res_ptr = get_arg(thread, 3);
        int32_t whence = get_arg(thread, 4);

        if (vfs_fd < 0)
        {
            error = EBADF;
            return transfer_error(thread);
        }
        if (res_ptr == 0)
        {
            error = EINVAL;
            return transfer_error(thread);
        }

        // SEEK_SET = 0, SEEK_CUR = 1, SEEK_END = 2
        // These are the only ones we support for now
        if (whence < 0 || whence > 2)
        {
            // We don't support SEEK_HOLE or SEEK_DATA
            // Set error to ENOTSUP in those two cases
            // Otherwise, set it to EINVAL
            error = whence > 2 ? EINVAL : ENOTSUP;
            return transfer_error(thread);
        }

        int res = vfs.seek(vfs_fd, offset, whence);
        if (res < 0)
        {
            return transfer_error(thread);
        }

        // Get the new file offset
        int64_t new_offset = vfs.tell(vfs_fd);
        if (new_offset < 0)
        {
            error = EIO;
            return transfer_error(thread);
        }

        // Write the new offset to the user space
        if (thread.get_process()->memory_space.memcpy(res_ptr, &new_offset, sizeof(new_offset)) != 0)
        {
            error = EFAULT;
            return transfer_error(thread);
        }

        // syscall returns 0 on success, but userspace wrapper returns the new offset
        // So we set the return value to 0
        return set_return(thread, 0);
    }
} // namespace Hamster

