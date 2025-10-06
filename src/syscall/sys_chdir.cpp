// Hamster chdir system call

#include <syscall/syscall.hpp>
#include <process/scheduler.hpp>
#include <errno/errno.h>
#include <filesystem>
#include <cassert>

namespace Hamster
{
    int32_t sys_chdir(uint32_t path_loc)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        // Get path
        const char *path = current_task->memory->obj.memory.get_string(path_loc);
        if (!path)
        {
            error = H_EFAULT; // Bad address
            return cvt_error();
        }

        // Normalize the CWD
        String path_str = path;
        dealloc(path);
        path_str = std::filesystem::path(path_str).lexically_normal().generic_string();

        // Set the current working directory

        auto &cwd = current_task->process->obj.fs_info->obj.cwd_path;
        auto &root = current_task->process->obj.fs_info->obj.root_path;

        if (path_str[0] == '/')
        {
            // Absolute path
            cwd = root + path_str;
        }
        else
        {
            // Relative path

            // Note: The VFS correctly handles multiple slashes
            cwd = cwd + "/" + path_str;
        }

        return 0;
    }
} // namespace Hamster

