// Hamster P-T system calls

#include <syscall/syscall.hpp>
#include <process/task.hpp>
#include <process/task_vfs_fd.hpp>
#include <abi/values.hpp>
#include <abi/structs.hpp>

namespace Hamster
{
    int32_t sys_read(Task &task, int32_t fd, uint32_t buf_loc, uint32_t count)
    {
        task.block([](Task &task, uint32_t task_fd, uint32_t buf_loc, uint32_t count, uint32_t, uint32_t, uint32_t) -> int {
            BaseTaskFD *fd = task.get_fd(task_fd);
            if (!fd)
                return -1;

            // Check if it is readable
            switch (fd->poll(0x1)) // 0x1 READ
            {
            case 0:
                error = H_EAGAIN;
                return -1;
            case 1:
                break;
            default:
                return -1;
            }

            // Read up to end of VM page

            uint32_t to_read = std::min(count, HAMSTER_PAGE_SIZE - (buf_loc % HAMSTER_PAGE_SIZE));

            void *it = task.mem_make_iterator(buf_loc);
            if (!it)
                return -1;
            
            return fd->read(it, to_read);
        });

        return 0;
    }

    int32_t sys_setpgid(Task &task, int32_t pid, int32_t pgid)
    {
        if (pid == 0)
            pid = task.get_pid();
        
        if ((uint32_t)pid == task.get_pid())
        {
            int res = task.set_pgid(pgid);
            if (res < 0)
                return cvt_error();
            return 0;
        }
        else
        {
            // Find the child process with the given PID
            const Set<Process *> &children = task.get_children_processes();
            for (Process *child : children)
            {
                if (child->get_pid() == (uint32_t)pid)
                {
                    int res = child->set_pgid(pgid);
                    if (res < 0)
                        return cvt_error();
                    return 0;
                }
            }

            // not found
            return -H_ESRCH;
        }
    }

    int32_t sys_setsid(Task &task)
    {
        int res = task.setsid();
        if (res < 0)
            return cvt_error();
        return task.get_sid();
    }

    int32_t sys_sched_yield(Task &task)
    {
        // no-op for now
        return 0;
    }
} // namespace Hamster

