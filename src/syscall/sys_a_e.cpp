// Hamster A-E system calls

#include <syscall/syscall.hpp>
#include <process/task.hpp>
#include <memory/stl_sequential.hpp>
#include <abi/values.hpp>
#include <abi/structs.hpp>

namespace Hamster
{
    namespace
    {
        char **read_strings(Task &task, uint32_t strings_loc)
        {
            uint32_t *pointers = task.mem_read_until_zero<uint32_t>(strings_loc);
            if (!pointers)
                return nullptr;
            
            Vector<char *> strings;
            strings.reserve(5); // Ensure data() is not nullptr

            for (uint32_t *it = pointers; *it; ++it)
            {
                char *string = task.mem_get_string(*it);
                if (!string)
                {
                    dealloc(pointers);
                    for (char *string : strings)
                        dealloc(string);
                    return nullptr;
                }
                strings.push_back(string);
            }

            char **out = alloc<char*>(strings.size() + 1);
            out[strings.size()] = nullptr;
            memcpy(out, strings.data(), strings.size() * sizeof(char *));
            
            dealloc(pointers);
            return out;
        }

        void destroy_strings(char **strings)
        {
            if (!strings)
                return;
            for (char **it = strings; *it; ++it)
                dealloc(*it);
            dealloc(strings);
        }
    } // namespace
    

    int32_t sys_exit(Task &task, int32_t status)
    {
        int res = task.exit(make_wait_exited(status));
        if (res < 0)
            return cvt_error();
        return 0;
    }

    int32_t sys_exit_group(Task &task, int32_t status)
    {
        int res = task.exit_group(make_wait_exited(status));
        if (res < 0)
            return cvt_error();
        return 0;
    }

    int32_t sys_close(Task &task, int32_t fd)
    {
        int res = task.close_fd(fd);
        if (res < 0)
            return cvt_error();
        return 0;
    }

    int32_t sys_clone(Task &task, uint32_t flags, uint32_t stack_loc, uint32_t ptid_loc, uint32_t tls, uint32_t ctid_loc)
    {
        Task *new_task = task.clone(flags, stack_loc, ptid_loc, tls, ctid_loc);
        if (!new_task)
            return cvt_error();
        return new_task->get_tid();
    }

    int32_t sys_execve(Task &task, uint32_t path_loc, uint32_t argv_loc, uint32_t envp_loc)
    {
        return sys_execveat(task, H_AT_FDCWD, path_loc, argv_loc, envp_loc, 0);
    }

    int32_t sys_execveat(Task &task, int32_t thread_dfd, uint32_t path_loc, uint32_t argv_loc, uint32_t envp_loc, int32_t flags)
    {
        char *path = task.mem_get_string(path_loc);
    
        // note 0x03 specifies checking for executability
        //
        // also note that path is intended to possibly be null, and open_rel_file
        // accounts for this
        int fd = task.open_rel_file(thread_dfd, path, (flags & H_AT_EMPTY_PATH) | OPEN_RDONLY | 0x03);
        dealloc(path);

        if (fd < 0)
            return cvt_error();

        char **argv, **envp;
        argv = read_strings(task, argv_loc);
        envp = read_strings(task, envp_loc);

        if (!argv || !envp)
        {
            destroy_strings(argv);
            destroy_strings(envp);
            vfs.close(fd);
            return cvt_error();
        }

        int res = task.exec(fd, argv, envp);
        vfs.close(fd);
        destroy_strings(argv);
        destroy_strings(envp);

        if (res < 0)
            return cvt_error();
        return 0;
    }

    int32_t sys_chdir(Task &task, uint32_t path_loc)
    {
        char *path = task.mem_get_string(path_loc);
        if (!path)
            return cvt_error();
        int res = task.chdir(path);
        dealloc(path);

        if (res < 0)
            return cvt_error();
        return 0;
    }

    int32_t sys_brk(Task &task, uint32_t new_brk)
    {
        return task.mbrk(new_brk);
    }
} // namespace Hamster

