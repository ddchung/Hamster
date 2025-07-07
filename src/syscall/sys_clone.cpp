// Hamster clone system call implementation

#include <syscall/syscall.hpp>
#include <abi/syscall_id.hpp>
#include <process/process.hpp>
#include <process/thread.hpp>
#include <process/scheduler.hpp>
#include <memory/allocator.hpp>
#include <errno/errno.h>
#include <signal.h>

namespace Hamster
{
    int sys_clone(Thread &thread)
    {
        uint32_t flags = get_arg(thread, 0);

        // For now, we only support single-threaded processes, so we will not handle any sharing
        if (flags != 0 && flags != SIGCHLD)
        {
            error = ENOSYS; // Not implemented
            return transfer_error(thread);
        }

        // Fork

        Process *process = thread.get_process();
        Process *new_process = alloc<Process>(1, *process);
        new_process->ppid = process->pid; // Set parent PID

        // Add to scheduler
        scheduler.add_process(new_process);

        // Return to both processes

        set_return(thread, new_process->pid);

        set_return(*new_process->threads.begin(), 0);

        return 0; // Success
    }
}
