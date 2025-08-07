// Hamster ftruncate64 and truncate64 system calls

#include <syscall/syscall.hpp>
#include <process/scheduler.hpp>
#include <filesystem/vfs.hpp>
#include <errno/errno.h>
#include <cassert>

namespace Hamster
{
    int32_t sys_ftruncate64(int32_t fd, uint32_t length_high, uint32_t length_low)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr && "No current task");

        int64_t length = ((int64_t)length_high << 32) | (int64_t) length_low;
        if (length < 0)
            return -EINVAL;

        int vfs_fd = current_task->get_vfs_fd(fd);
        if (vfs_fd < 0)
            return cvt_error();

        int res = vfs.truncate(vfs_fd, length);
        if (res < 0)
            return cvt_error();

        return 0;
    }

    int32_t sys_truncate64(uint32_t path_loc, uint32_t length_high, uint32_t length_low)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr && "No current task");

        int64_t length = ((int64_t)length_high << 32) | (int64_t) length_low;
        if (length < 0)
            return -EINVAL;

        char *path = current_task->get_memory().get_string(path_loc);
        if (!path)
            return cvt_error();
        
        path = current_task->process_user_path(path);
        if (!path)
            return cvt_error();
        
        int fd = vfs.open(path, OPEN_WRONLY);
        dealloc(path);

        if (fd < 0)
            return cvt_error();
        
        int res = vfs.truncate(fd, length);
        vfs.close(fd);

        if (res < 0)
            return cvt_error();
        return 0;
    }
} // namespace Hamster
