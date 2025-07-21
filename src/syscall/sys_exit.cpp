// Hamster exit syscall

#include <syscall/syscall.hpp>
#include <abi/syscall_id.hpp>
#include <process/scheduler.hpp>
#include <cassert>

namespace Hamster
{
    int sys_exit(Thread &thread)
    {
        // Get the exit status from the a0 register
        int status = get_arg(thread, 0);

        Process *process = thread.get_process();

        assert(process);

        // Make init adopt all child processes
        scheduler.adopt_processes(process->pid);

        for (auto &t : process->threads)
        {
            t.set_state(ThreadState::ENDED);
        }

        // Set the exit status for the process

        // Encode into the correct format, used by wait
        process->exit_status = (status & 0xFF) << 8;

        // No return to thread, as it is ended

        return 0;
    }
}
