// Hamster uname system call

#include <syscall/syscall.hpp>
#include <process/scheduler.hpp>
#include <errno/errno.h>

namespace Hamster
{
    constexpr sys_utsname default_uname = {
        .sysname = "Hamster",
        .nodename = "localhost",
        .release = "dev",
        .version = "0.0",
        .machine = "riscv32",
        .domainname = "localdomain",
    };

    int32_t sys_uname(uint32_t buf_loc)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        if (!buf_loc)
        {
            return -H_EFAULT;
        }

        if (current_task->copy_to_user(default_uname, buf_loc) < 0)
        {
            error = H_EFAULT;
            return cvt_error();
        }

        return 0;
    }
} // namespace Hamster

