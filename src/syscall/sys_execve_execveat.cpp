// Hamster execve and execveat system calls

#include <syscall/syscall.hpp>
#include <process/scheduler.hpp>
#include <memory/allocator.hpp>
#include <process/task.hpp>
#include <abi/values.hpp>
#include <errno/errno.h>

namespace Hamster
{
    int32_t sys_execve(uint32_t path_loc, uint32_t argv_loc, uint32_t envp_loc)
    {
        return sys_execveat(H_AT_FDCWD, path_loc, argv_loc, envp_loc, 0);
    }

    int32_t sys_execveat(int32_t dfd, uint32_t path_loc, uint32_t argv_loc, uint32_t envp_loc, int32_t flags)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr && "No current task");

        // Get the path
        char *path = current_task->memory->obj.memory.get_string(path_loc);
        if (!path)
        {
            error = EFAULT;
            return cvt_error();
        }

        // Get the relative directory file descriptor

        int vfs_at_fd = current_task->get_relative_fd(path, dfd);

        if (vfs_at_fd < 0)
        {
            dealloc(path);
            return cvt_error();
        }

        // Get the arguments

        char **argv_strings;
        char **envp_strings;
        size_t count;
        if (!argv_loc)
            argv_strings = nullptr;
        else
        {
            count = 0;
            uint32_t i;
            for (uint32_t it = argv_loc; current_task->memory->obj.memory.memcpy(&i, it, 4) == 0 && i != 0; it += 4)
                ++count;

            argv_strings = alloc<char *>(count + 1);
            argv_strings[count] = nullptr;
            for (size_t it = 0; it < count; ++it)
            {
                current_task->memory->obj.memory.memcpy(&i, argv_loc + it * 4, 4);
                char *string = current_task->memory->obj.memory.get_string(i);
                if (!string)
                {
                    // if it failed, put empty one
                    string = alloc<char>(1);
                    string[0] = '\0';
                }
                argv_strings[it] = string;
            }
        }
        if (!envp_loc)
            envp_strings = nullptr;
        else
        {
            count = 0;
            uint32_t i;
            for (uint32_t it = envp_loc; current_task->memory->obj.memory.memcpy(&i, it, 4) == 0 && i != 0; it += 4)
                ++count;

            envp_strings = alloc<char *>(count + 1);
            envp_strings[count] = nullptr;
            for (size_t it = 0; it < count; ++it)
            {
                current_task->memory->obj.memory.memcpy(&i, envp_loc + it * 4, 4);
                char *string = current_task->memory->obj.memory.get_string(i);
                if (!string)
                {
                    // if it failed, put empty one
                    string = alloc<char>(1);
                    string[0] = '\0';
                }
                envp_strings[it] = string;
            }
        }

        // Do the exec
        int ret = current_task->process->obj.exec(path, argv_strings, envp_strings, vfs_at_fd);
        vfs.close(vfs_at_fd);

        // Clean up
        dealloc(path);

        if (argv_strings)
        {
            for (const char *const *it = argv_strings; *it; ++it)
            {
                dealloc(*it);
            }
            dealloc(argv_strings);
        }
        if (envp_strings)
        {
            for (const char *const *it = envp_strings; *it; ++it)
            {
                dealloc(*it);
            }
            dealloc(envp_strings);
        }

        if (ret < 0)
        {
            return cvt_error();
        }

        return 0;
    }
} // namespace Hamster

