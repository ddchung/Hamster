// Hamster getcwd system call

#include <syscall/syscall.hpp>
#include <abi/syscall_id.hpp>
#include <process/process.hpp>
#include <memory/allocator.hpp>
#include <errno/errno.h>

namespace Hamster
{
    int sys_getcwd(Thread &thread)
    {
        uint32_t buf_addr = get_arg(thread, 0);
        uint32_t size = get_arg(thread, 1);
        if (buf_addr == 0 || size == 0)
        {
            error = EINVAL;
            return transfer_error(thread);
        }

        Process *proc = thread.get_process();

        if (proc->cwd.size() + 1 > size)
        {
            error = ERANGE;
            return transfer_error(thread);
        }

        // Copy the current working directory to the provided buffer

        if (proc->memory_space.memcpy(buf_addr, proc->cwd.c_str(), proc->cwd.size() + 1) < 0)
        {
            error = EFAULT;
            return transfer_error(thread);
        }

        return set_return(thread, buf_addr);
    }
} // namespace Hamster

