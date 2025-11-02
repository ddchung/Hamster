// Hamster A-E system calls

#include <syscall/syscall.hpp>
#include <process/task.hpp>
#include <abi/values.hpp>
#include <abi/structs.hpp>

namespace Hamster
{
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
} // namespace Hamster

