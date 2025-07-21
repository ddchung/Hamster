// Hamster readlinkat system call implementation

#include <syscall/syscall.hpp>
#include <abi/syscall_id.hpp>
#include <process/process.hpp>
#include <filesystem/vfs.hpp>
#include <errno/errno.h>
#include <memory/allocator.hpp>
#include <cstring>

namespace Hamster
{
    int sys_readlinkat(Thread &thread)
    {
        // ssize_t readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz);
        
        int32_t thread_dfd = get_arg(thread, 0);
        uint32_t path = get_arg(thread, 1);
        uint32_t buf = get_arg(thread, 2);
        uint32_t bufsiz = get_arg(thread, 3);

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

        char *link_target = nullptr;

        if (is_ref_root)
        {
            link_target = vfs.get_target(path_str);
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

            link_target = vfs.get_targetat(dfd, path_str);
        }

        dealloc(path_str);

        if (!link_target)
        {
            return transfer_error(thread);
        }

        // readlinkat does not append a null terminator to the buffer
        size_t target_length = std::min<size_t>(strlen(link_target), bufsiz);

        if (process->memory_space.memcpy(buf, link_target, target_length) < 0)
        {
            dealloc(link_target);
            return transfer_error(thread);
        }

        dealloc(link_target);

        // Return the length of the link target
        return set_return(thread, target_length);
    }
} // namespace Hamster

