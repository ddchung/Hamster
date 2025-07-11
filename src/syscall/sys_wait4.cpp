// Hamster wait4 system call

#include <syscall/syscall.hpp>
#include <abi/syscall_id.hpp>
#include <process/process.hpp>
#include <process/scheduler.hpp>
#include <errno/errno.h>
#include <abi/structs.hpp>

namespace Hamster
{
    namespace
    {
        void wait4_callback(Thread &thread)
        {
            int32_t pid = get_arg(thread, 0);
            uint32_t stat_loc = get_arg(thread, 1);
            int32_t options = get_arg(thread, 2);
            uint32_t ru_loc = get_arg(thread, 3);

            if (pid < -1 || pid == 0)
            {
                // We don't support PGID-based waits yet
                error = ENOTSUP;
                transfer_error(thread);
                thread.resume();
                return;
            }

            int exit_status = 0;
            uint32_t res = scheduler.get_exit_status(thread.get_process()->pid, pid, exit_status, !(options & 0x1000000));

            if (res == 0)
            {
                if (error != EBUSY)
                {
                    // Error occured, transfer it
                    transfer_error(thread);
                    thread.resume();
                    return;
                }

                // Still waiting for the process to exit

                // clear error
                error = 0;

                if (options & 0x1) // WNOHANG
                {
                    // If WNOHANG is set, return immediately with 0
                    set_return(thread, 0);
                    thread.resume();
                    return;
                }

                // Continue pausing...
                // If we don't unpause, it will keep calling this callback
                // so we do nothing to keep waiting.
                return;
            }

            int32_t status32 = (int32_t)exit_status;

            if (stat_loc)
                thread.get_process()->memory_space.memcpy(stat_loc, &status32, sizeof(int32_t));
            if (ru_loc)
                thread.get_process()->memory_space.memset(ru_loc, 0, sizeof(sys_rusage));

            set_return(thread, 0);
            thread.resume();
        }
    }

    int sys_wait4(Thread &thread)
    {
        thread.pause(wait4_callback);
        return 0;
    }
} // namespace Hamster

