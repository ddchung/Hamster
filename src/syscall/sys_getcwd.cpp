// Hamster getcwd system call

#include <syscall/syscall.hpp>
#include <process/scheduler.hpp>
#include <errno/errno.h>
#include <cstring>
#include <cstddef>

namespace Hamster
{
    int32_t sys_getcwd(uint32_t buf_loc, uint32_t size)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        if (buf_loc == 0)
        {
            error = H_EFAULT; // Bad address
            return cvt_error();
        }

        // Get the current working directory
        auto &cwd = current_task->process->obj.fs_info->obj.cwd_path;
        auto &root = current_task->process->obj.fs_info->obj.root_path;

        // Subtract the root path from the CWD to get what the CWD is relative to the root
        const char *cwd_str = cwd.c_str() + root.length();

        // Check if the buffer is large enough
        size_t cwd_length = strlen(cwd_str);
        if (size < cwd_length + 1)
        {
            error = H_ERANGE; // Buffer too small
            return cvt_error();
        }

        // Copy the CWD to the buffer
        if (current_task->memory->obj.memory.memcpy_alloc(buf_loc, cwd_str, cwd_length + 1) < 0)
        {
            error = H_EFAULT; // Bad address
            return cvt_error();
        }

        return buf_loc;
    }
} // namespace Hamster

