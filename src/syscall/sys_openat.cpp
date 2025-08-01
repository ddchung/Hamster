// Hamster openat system call

#include <syscall/syscall.hpp>
#include <process/scheduler.hpp>
#include <filesystem/vfs.hpp>
#include <errno/errno.h>
#include <cassert>

namespace Hamster
{
    int32_t sys_openat(int32_t thread_dfd, uint32_t pathname_loc, int32_t flags, uint32_t mode)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr && "No current task");

        char *pathname = current_task->memory->obj.memory.get_string(pathname_loc);
        if (!pathname)
        {
            error = EFAULT;
            return cvt_error();
        }
        
        int vfs_at_fd = current_task->get_relative_fd(pathname, thread_dfd);

        if (vfs_at_fd < 0)
        {
            dealloc(pathname);
            return cvt_error();
        }

        int new_fd = vfs.openat(vfs_at_fd, pathname, flags, mode);
        vfs.close(vfs_at_fd);
        dealloc(pathname);

        if (new_fd < 0)
        {
            return cvt_error();
        }

        // Make it the controlling TTY if it is a terminal, and we don't have one
        if ((flags & OPEN_NOCTTY) == 0)
        {
            auto &session = current_task->process->obj.pg->obj.session->obj;
            if (session.controlling_tty == DeviceID{0, 0})
            {
                if (vfs.is_tty(new_fd) == 1)
                {
                    DeviceID dev_id = vfs.get_device_id(new_fd);
                    if (dev_id.major != 0 || dev_id.minor != 0)
                    {
                        session.controlling_tty = dev_id;
                        IoctlArg arg;
                        arg.i = 0;
                        vfs.ioctl(new_fd, H_TIOCSCTTY, arg);
                    }
                }
            }
        }

        // Set its reference count to 1, since we just opened it
        fd_refcount[new_fd] = 1;

        int new_thread_fd = current_task->get_unused_fd_index();
        if (new_thread_fd < 0)
        {
            error = EMFILE;
            vfs.close(new_fd);
            return cvt_error();
        }

        auto &fd = current_task->fd_table->obj.fds[new_thread_fd];
        fd.type = UserFDType::VFS;
        fd.vfs_fd = new_fd;

        // Set appropriate CLOEXEC flag
        if (flags & OPEN_CLOEXEC)
            fd.flags |= H_FD_CLOEXEC;
        else
            fd.flags &= ~H_FD_CLOEXEC; // Clear CLOEXEC if not set

        return new_thread_fd;
    }
} // namespace Hamster

