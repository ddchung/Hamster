// Hamster execve syscall
// A wrapper around execveat

#include <syscall/syscall.hpp>
#include <filesystem/vfs.hpp>
#include <memory/allocator.hpp>
#include <errno/errno.h>
#include <cstring>

namespace Hamster
{
    int sys_execve(Thread &thread)
    {
        // Store the original values in register a4, which we set flags
        uint32_t old_a4 = thread.get_regs()[14];

        // and register a3, whose data would be lost
        uint32_t old_a3 = thread.get_regs()[13];

        // configure the arguments for execveat instead of execve

        // flags
        thread.get_regs()[14] = 0;

        // envp
        thread.get_regs()[13] = thread.get_regs()[12];

        // argv
        thread.get_regs()[12] = thread.get_regs()[11];

        // filename
        thread.get_regs()[11] = thread.get_regs()[10];

        // dirfd = AT_FDCWD
        thread.get_regs()[10] = -100;

        // run the execveat syscall with the modified at_fd
        int ret = sys_execveat(thread);

        // Revert the registers, except a0 (return value)
        thread.get_regs()[11] = thread.get_regs()[12];
        thread.get_regs()[12] = thread.get_regs()[13];
        thread.get_regs()[13] = old_a3;
        thread.get_regs()[14] = old_a4;

        return ret;
    }

    int sys_execveat(Thread &thread)
    {
        // int execveat(int dirfd, const char *filename, char *const argv[], char *const envp[], int flags);
        int32_t thread_dirfd = get_arg(thread, 0);
        uint32_t filename = get_arg(thread, 1);
        uint32_t argv = get_arg(thread, 2);
        uint32_t envp = get_arg(thread, 3);
        int32_t flags = get_arg(thread, 4);

        Process *process = thread.get_process();
        
        char *path = process->memory_space.get_string(filename);
        if (!path)
        {
            error = EIO;
            return transfer_error(thread);
        }

        bool is_relative = (path[0] != '/');

        if (is_relative && thread_dirfd == -100)
        {
            if (path[0] == '\0')
            {
                // Empty path
                error = flags & 0x1000 ? ENOTDIR : ENOENT;
                dealloc(path);
                return transfer_error(thread);
            }
            // AT_FDCWD
            char *new_buffer = alloc<char>(process->cwd.length() + 1 + strlen(path) + 1);
            strcpy(new_buffer, process->cwd.c_str());
            strcat(new_buffer, "/");
            strcat(new_buffer, path);

            dealloc(path);
            path = new_buffer;

            // use the code for the absolute path, because we made it absolute
            is_relative = false;
        }
        
        if (is_relative && thread_dirfd < 0)
        {
            // Invalid directory file descriptor
            dealloc(path);
            error = EBADF;
            return transfer_error(thread);
        }

        char **argv_strings;
        char **envp_strings;
        size_t count;
        if (!argv)
            argv_strings = nullptr;
        else
        {
            count = 0;
            uint32_t i;
            for (uint32_t it = argv; process->memory_space.memcpy(&i, it, 4) == 0 && i != 0; it += 4)
                ++count;

            argv_strings = alloc<char *>(count + 1);
            argv_strings[count] = nullptr;
            for (size_t it = 0; it < count; ++it)
            {
                process->memory_space.memcpy(&i, argv + it * 4, 4);
                char *string = process->memory_space.get_string(i);
                if (!string)
                {
                    // if it failed, put empty one
                    string = alloc<char>(1);
                    string[0] = '\0';
                }
                argv_strings[it] = string;
            }
        }
        if (!envp)
            envp_strings = nullptr;
        else
        {
            count = 0;
            uint32_t i;
            for (uint32_t it = envp; process->memory_space.memcpy(&i, it, 4) == 0 && i != 0; it += 4)
                ++count;

            envp_strings = alloc<char *>(count + 1);
            envp_strings[count] = nullptr;
            for (size_t it = 0; it < count; ++it)
            {
                process->memory_space.memcpy(&i, envp + it * 4, 4);
                char *string = process->memory_space.get_string(i);
                if (!string)
                {
                    // if it failed, put empty one
                    string = alloc<char>(1);
                    string[0] = '\0';
                }
                envp_strings[it] = string;
            }
        }

        int res;

        if (is_relative)
        {
            int vfs_dfd = deref_fd(thread, thread_dirfd);
            sys_stat st;
            if (vfs_dfd < 0 || vfs.stat(vfs_dfd, &st) < 0 || (path[0] != '\0' && !is_directory(st.mode)))
            {
                for (char **it = argv_strings; it && *it; ++it)
                    dealloc(*it);
                dealloc(argv_strings);

                for (char **it = envp_strings; it && *it; ++it)
                    dealloc(*it);
                dealloc(envp_strings);

                dealloc(path);

                error = EBADF;
                return transfer_error(thread);
            }

            res = process->load_elf(path, argv_strings, envp_strings, vfs_dfd);
        }
        else
        {
            res = process->load_elf(path, argv_strings, envp_strings);
        }

        for (char **it = argv_strings; it && *it; ++it)
            dealloc(*it);
        dealloc(argv_strings);

        for (char **it = envp_strings; it && *it; ++it)
            dealloc(*it);
        dealloc(envp_strings);

        dealloc(path);

        if (res < 0)
            return transfer_error(thread);
        
        return set_return(thread, res);
    }
} // namespace Hamster

