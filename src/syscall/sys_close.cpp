// Hamster close syscall

#include <syscall/syscall.hpp>
#include <abi/syscall_id.hpp>
#include <errno/errno.h>
#include <filesystem/vfs.hpp>
#include <process/process.hpp>
#include <cassert>

namespace Hamster
{
    int sys_close(Thread &thread)
    {
        int32_t fd = get_arg(thread, 0);

        int vfs_fd = deref_fd(thread, fd);
        if (vfs_fd < 0)
        {
            error = EBADF;
            return transfer_error(thread);
        }

        Process *process = thread.get_process();
        if (vfs_fd < 0)
        {
            error = EBADF;
            return transfer_error(thread);
        }

        fd_refcount[vfs_fd]--;
        process->fds[fd].fd = -1; // Mark as closed

        int ret = 0;

        if (fd_refcount[vfs_fd] <= 0)
        {
            ret = vfs.close(vfs_fd);
            fd_refcount.erase(vfs_fd);
        }

        if (ret < 0)
        {
            // error set in `vfs.close`
            return transfer_error(thread);
        }

        return 0;
    }
} // namespace Hamster

