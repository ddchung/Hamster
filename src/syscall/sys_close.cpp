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

        int res = current_task->close(fd);

        if (res < 0)
            return cvt_error();
        return 0;
    }
} // namespace Hamster
