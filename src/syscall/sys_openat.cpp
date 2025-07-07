// Hamster openat syscall

#include <syscall/syscall.hpp>
#include <filesystem/vfs.hpp>
#include <memory/allocator.hpp>
#include <cstring>

namespace Hamster
{
    int sys_openat(Thread &thread)
    {
        int32_t thread_dfd = get_arg(thread, 0);
        uint32_t path = get_arg(thread, 1);
        int32_t flags = get_arg(thread, 2);
        int32_t mode = get_arg(thread, 3);

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
            return set_return(thread, -EBADF);
        }

        int fd;

        if (is_ref_root)
        {
            // Open the file relative to the root directory
            fd = vfs.open(path_str, flags, mode);
        }
        else
        {
            // Open the file relative to the directory file descriptor
            int dfd = deref_fd(thread, thread_dfd);
            if (dfd < 0)
            {
                dealloc(path_str);
                return set_return(thread, -EBADF);
            }

            fd = vfs.openat(dfd, path_str, flags, mode);
        }

        dealloc(path_str);

        if (fd < 0)
        {
            return transfer_error(thread);
        }

        // Set the reference count to 1, because we just opened it

        fd_refcount[fd] = 1;

        // Add to the process's file descriptor table

        for (size_t i = 0; i < process->fds.size(); ++i)
        {
            if (process->fds[i].fd < 0)
            {
                // Reuse a closed file descriptor slot
                process->fds[i].fd = fd;
                process->fds[i].fd_flags = 0; // No flags set
                return set_return(thread, i);
            }
        }

        // If we reach here, we need to allocate a new file descriptor
        ProcessFd new_fd;
        new_fd.fd = fd;
        new_fd.fd_flags = 0; // No flags set
        process->fds.push_back(new_fd);
        return set_return(thread, process->fds.size() - 1);
    }
} // namespace Hamster

