// Hamster readlinkat system call

#include <syscall/syscall.hpp>
#include <process/scheduler.hpp>
#include <filesystem/vfs.hpp>
#include <errno/errno.h>
#include <cstring>

namespace Hamster
{
    int32_t sys_readlinkat(int32_t thread_dfd, uint32_t pathname_loc, uint32_t buf_loc, uint32_t buf_size)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        // Get the path
        char *pathname = current_task->memory->obj.memory.get_string(pathname_loc);
        if (!pathname)
        {
            error = EFAULT; // Bad address
            return cvt_error();
        }

        int vfs_rel_fd = current_task->get_relative_fd(pathname, thread_dfd);
        if (vfs_rel_fd < 0)
        {
            return cvt_error();
        }

        char *target = vfs.get_targetat(vfs_rel_fd, pathname);
        dealloc(pathname);
        if (!target)
            return cvt_error();

        size_t to_copy = strlen(target);
        if (to_copy >= buf_size)
            to_copy = buf_size;
        
        // Copy to userspace
        // Note, from readlinkat(2): "readlink() does not append a terminating null byte to buf."
        int res = current_task->memory->obj.memory.memcpy(buf_loc, target, to_copy);

        dealloc(target);
        if (res < 0)
        {
            error = EFAULT; // Bad address
            return cvt_error();
        }

        return to_copy;
    }
} // namespace Hamster

