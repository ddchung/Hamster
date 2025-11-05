// Hamster K-O system calls

#include <syscall/syscall.hpp>
#include <process/task.hpp>
#include <process/task_vfs_fd.hpp>
#include <abi/values.hpp>
#include <abi/structs.hpp>

namespace Hamster
{
    int32_t sys_openat(Task &task, int32_t thread_dfd, uint32_t path_loc, int32_t flags, uint32_t mode)
    {
        char *path = task.mem_get_string(path_loc);
        if (!path)
            return cvt_error();
        
        int rel_fd = task.open_rel_fd(thread_dfd, path);
        if (rel_fd < 0)
            return cvt_error();
        
        // Process mode with umask
        mode = task.mask_mode(mode);

        int fd = vfs.openat(rel_fd, path, flags, mode);
        dealloc(path);
        vfs.close(rel_fd);

        if (fd < 0)
            return cvt_error();

        int task_fd = task.allocate_fd();
        if (task_fd < 0)
        {
            vfs.close(fd);
            return cvt_error();
        }

        // Make a new TaskVFSFD with the opened file descriptor, and put it in the
        // allocated slot
        if (task.set_fd(alloc<TaskVFSFD>(1, fd), task_fd) < 0)
            return cvt_error();
        return task_fd;
    }

    int32_t sys_mmap2(Task &task, uint32_t addr, uint32_t length, uint32_t prot, uint32_t flags, int32_t fd, uint32_t offset)
    {
        uint32_t res = task.mmap(addr, length, prot, flags, fd, offset);
        if (res == UINT32_MAX)
            return cvt_error();
        return res;
    }
} // namespace Hamster

