// Hamster A-E system calls

#include <syscall/syscall.hpp>
#include <process/task.hpp>
#include <process/task_socket.hpp>
#include <memory/stl_sequential.hpp>
#include <abi/values.hpp>
#include <abi/structs.hpp>
#include <platform/clock_realtime.hpp>

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
        // FIXME
        flags &= ~(H_CLONE_DETACHED | H_CLONE_SYSVSEM);

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
        if (fd < 0)
        {
            dealloc(path);
            return cvt_error();
        }

        char **argv, **envp;
        argv = read_strings(task, argv_loc);
        envp = read_strings(task, envp_loc);

        if (!argv || !envp)
        {
            destroy_strings(argv);
            destroy_strings(envp);
            vfs.close(fd);
            dealloc(path);
            return cvt_error();
        }

        int res = task.exec(fd, path, argv, envp);
        dealloc(path);
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

    int32_t sys_dup(Task &task, int32_t task_fd)
    {
        return sys_fcntl64(task, task_fd, FILE_DUPFD, 0);
    }

    int32_t sys_dup3(Task &task, int32_t old_fd, int32_t new_fd, int32_t flags)
    {
        if (old_fd == new_fd)
            return -H_EINVAL;
        task.close_fd(new_fd);
        return sys_fcntl64(task, old_fd, flags & OPEN_CLOEXEC ? FILE_DUPFD_CLOEXEC : FILE_DUPFD, new_fd);
    }

    int32_t sys_clock_getres_time64(Task &task, int32_t clock_id, uint32_t res_loc)
    {
        sys_timespec res;

        if (!res_loc)
            return -H_EFAULT;

        switch (clock_id)
        {
        case H_CLOCK_REALTIME:
        case H_CLOCK_MONOTONIC:
            res.nsec = 1'000'000; // 1ms
            res.sec = 0;
            break;
        default:
            return -H_EINVAL;
        }

        if (task.copy_to_memory(res_loc, res) < 0)
            return cvt_error();
        return 0;
    }

    int32_t sys_clock_gettime64(Task &task, int32_t clock_id, uint32_t tp_loc)
    {
        sys_timespec ts = {};
        uint64_t now = _get_sys_time();

        switch (clock_id)
        {
        case H_CLOCK_REALTIME:
            ts.nsec = (now + clock_rt_offset) % 1000 * 1'000'000; // 1 million ms in ns
            ts.sec = (now + clock_rt_offset) / 1000;
            break;
        case H_CLOCK_MONOTONIC:
            ts.nsec += now % 1000 * 1'000'000;
            ts.sec += now / 1000;
            break;
        default:
            return -H_EINVAL;
        }

        if (task.copy_to_memory(tp_loc, ts) < 0)
            return cvt_error();
        return 0;
    }

    int32_t sys_clock_settime64(Task &task, int32_t clock_id, uint32_t ts_loc)
    {
        sys_timespec ts;

        if (task.copy_from_memory(ts, ts_loc) < 0)
            return cvt_error();

        uint64_t ts_ms = ts.sec * 1000 + (ts.nsec + 500'000) / 1'000'000;
        uint64_t now = _get_sys_time();

        switch (clock_id)
        {
        case H_CLOCK_REALTIME:
            clock_rt_offset = ts_ms - now;
            break;
        case H_CLOCK_MONOTONIC:
            return -H_EPERM;
        default:
            return -H_EINVAL;
        }

        return 0;
    }

    int32_t sys_clock_nanosleep_time64(Task &task, int32_t clock_id, int32_t flags, uint32_t req_loc, uint32_t rem_loc)
    {
        task.block([](Task &task, uint32_t clock_id, uint32_t flags, uint32_t req_loc, uint32_t rem_loc, uint32_t, uint32_t) {
            sys_timespec req;

            if (!req_loc)
            {
                error = H_EINVAL;
                return -1;
            }

            if (task.copy_from_memory(req, req_loc) < 0)
                return -1;

            uint64_t req_ms = req.sec * 1000 + req.nsec / 1000000;
            uint64_t now = _get_sys_time();

            if (flags & H_TIMER_ABSTIME)
            {
                switch (clock_id)
                {
                case H_CLOCK_REALTIME:
                    now += clock_rt_offset;
                    [[fallthrough]];
                case H_CLOCK_MONOTONIC:
                    if (now >= req_ms)
                        return 0; // passed requested timepoint
                    break;
                default:
                    error = H_EINVAL;
                    return -1;
                }
            }
            else if (task.get_last_tick() + req_ms <= now)
                return 0; // Done sleeping
            
            // See comment on Task::block
            error = H_EAGAIN;
            return -1;
        });

        return 0;
    }

    int32_t sys_connect(Task &task, int32_t sockfd, uint32_t addr_loc, uint32_t addrlen)
    {
        BaseTaskFD *fd = task.get_fd(sockfd);
        if (!fd)
            return cvt_error();
        if (fd->type() != TaskFDType::Socket)
            return -H_ENOTSOCK;
        TaskSocket *socket_fd = (TaskSocket *)fd;

        void *buf = alloca(addrlen);
        if (task.memcpy(buf, addr_loc, addrlen) < 0)
            return cvt_error();
        return cvt_error(socket_fd->get_socket()->connect((sys_sockaddr *)buf, addrlen));
    }

    int32_t sys_bind(Task &task, int32_t sockfd, uint32_t addr_loc, uint32_t addrlen)
    {
        BaseTaskFD *fd = task.get_fd(sockfd);
        if (!fd)
            return cvt_error();
        if (fd->type() != TaskFDType::Socket)
            return -H_ENOTSOCK;
        TaskSocket *socket_fd = (TaskSocket *)fd;

        void *buf = alloca(addrlen);
        if (task.memcpy(buf, addr_loc, addrlen) < 0)
            return cvt_error();
        return cvt_error(socket_fd->get_socket()->bind((sys_sockaddr *)buf, addrlen));
    }
} // namespace Hamster

