// Hamster close system call

#include <syscall/syscall.hpp>
#include <process/scheduler.hpp>
#include <filesystem/vfs.hpp>
#include <errno/errno.h>

namespace Hamster
{
    int32_t sys_close(int32_t fd)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        auto &fds = current_task->fd_table->obj.fds;
        if (fd < 0 || fd >= (int32_t)fds.size())
        {
            error = EBADF; // Bad file descriptor
            return -1;
        }

        UserFD &user_fd = fds[fd];

        if (user_fd.type == UserFDType::VFS)
        {
            // Close the VFS file descriptor
            int vfs_fd = user_fd.vfs_fd;

            user_fd.vfs_fd = -1; // Reset the VFS file descriptor
            
            fd_refcount[vfs_fd]--;
            if (fd_refcount[vfs_fd] == 0)
            {
                fd_refcount.erase(vfs_fd);

                int res = vfs.close(vfs_fd);
                if (res < 0)
                {
                    return cvt_error();
                }
                return 0;
            }
        }
        else if (user_fd.type == UserFDType::PID)
        {
            // Mark as closed

            user_fd.type = UserFDType::VFS;
            user_fd.vfs_fd = -1; // Reset the VFS file descriptor
        }

        return 0;
    }
} // namespace Hamster
