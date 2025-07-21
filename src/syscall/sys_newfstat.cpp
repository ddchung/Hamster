// Hamster newfstat system call

#include <syscall/syscall.hpp>
#include <abi/syscall_id.hpp>
#include <filesystem/vfs.hpp>
#include <abi/structs.hpp>
#include <process/process.hpp>
#include <errno/errno.h>

namespace Hamster
{
    int sys_newfstat(Thread &thread)
    {
        int vfs_fd = deref_fd(thread, get_arg(thread, 0));
        uint32_t statbuf_addr = get_arg(thread, 1);

        if (vfs_fd < 0)
        {
            error = EBADF;
            return transfer_error(thread);
        }

        if (statbuf_addr == 0)
        {
            error = EFAULT;
            return transfer_error(thread);
        }

        sys_stat st;
        if (vfs.stat(vfs_fd, &st) < 0)
        {
            return transfer_error(thread);
        }

        // Copy the stat structure to user space
        if (thread.get_process()->memory_space.memcpy(statbuf_addr, &st, sizeof(sys_stat)) < 0)
        {
            error = EFAULT;
            return transfer_error(thread);
        }

        return set_return(thread, 0);
    }
} // namespace Hamster

