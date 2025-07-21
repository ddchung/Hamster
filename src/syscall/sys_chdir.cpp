// Hamster chdir system call implementation

#include <syscall/syscall.hpp>
#include <memory/allocator.hpp>
#include <process/process.hpp>
#include <errno/errno.h>

namespace Hamster
{
    int sys_chdir(Thread &thread)
    {
        // int chdir(const char *path);
        uint32_t path = get_arg(thread, 0);

        // Get the path from the thread's memory space
        Process *process = thread.get_process();
        char *path_str = process->memory_space.get_string(path);

        if (!path_str)
        {
            return transfer_error(thread);
        }

        if (path_str[0] == '\0')
        {
            error = ENOENT;
            dealloc(path_str);
            return transfer_error(thread);
        }

        // Set the current working directory
        process->cwd = path_str;

        dealloc(path_str);
        return set_return(thread, 0); // Success
    }
} // namespace Hamster

