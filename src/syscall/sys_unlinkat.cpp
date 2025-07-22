// Hamster unlinkat system call

#include <syscall/syscall.hpp>
#include <filesystem/vfs.hpp>
#include <process/process.hpp>
#include <memory/allocator.hpp>
#include <errno/errno.h>
#include <cstring>

namespace Hamster
{
    int sys_unlinkat(Thread &thread)
    {
        int32_t thread_dfd = get_arg(thread, 0);
        uint32_t path = get_arg(thread, 1);
        // For now, don't support flags

        bool is_ref_root = false;

        // Get the path from the thread's memory space

        Process *process = thread.get_process();

        char *path_str = process->memory_space.get_string(path);

        if (!path_str)
        {
            return transfer_error(thread);
        }

        if (path_str[0] == '/')
        {
            // Absolute path, use the root directory
            is_ref_root = true;
        }
        else if (thread_dfd == -100)
        {
            // AT_FDCWD, use the current working directory
            is_ref_root = true;

            // +1 for '/' and +1 for '\0'
            char *new_buffer = alloc<char>(process->cwd.length() + 1 + strlen(path_str) + 1);

            strcpy(new_buffer, process->cwd.c_str());
            strcat(new_buffer, "/");
            strcat(new_buffer, path_str);

            dealloc(path_str);
            path_str = new_buffer;
        }
        else if (thread_dfd < 0)
        {
            // Invalid directory file descriptor
            dealloc(path_str);
            error = EBADF;
            return transfer_error(thread);
        }

        int res = 0;

        if (is_ref_root)
        {
            res = vfs.remove(path_str);
        }
        else
        {
            // Open the file relative to the directory file descriptor
            int dfd = deref_fd(thread, thread_dfd);
            if (dfd < 0)
            {
                dealloc(path_str);
                error = EBADF;
                return transfer_error(thread);
            }

            res = vfs.removeat(dfd, path_str);
        }

        dealloc(path_str);

        if (res < 0)
        {
            return transfer_error(thread);
        }

        return set_return(thread, 0);
    }
} // namespace Hamster

