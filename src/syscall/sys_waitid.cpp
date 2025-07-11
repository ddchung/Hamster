// Hamster waitid system call
// Not to be confused with the waitid wrapper function in the C library

#include <syscall/syscall.hpp>
#include <abi/syscall_id.hpp>
#include <abi/structs.hpp>
#include <process/process.hpp>
#include <errno/errno.h>
#include <process/scheduler.hpp>

namespace Hamster
{
    namespace
    {
        void waitid_callback(Thread &thread)
        {
            int32_t which = get_arg(thread, 0);
            int32_t id = get_arg(thread, 1);
            uint32_t infop_addr = get_arg(thread, 2);
            int32_t options = get_arg(thread, 3);
            uint32_t ru_addr = get_arg(thread, 4);

            // We only support P_PID and P_ALL for `which`
            if (which != 1 && which != 0)
            {
                // ENOTSUP if P_PIDFD or P_PGID is used
                // EINVAL if any other value is used
                error = which > 1 && which <= 3 ? ENOTSUP : EINVAL;
                transfer_error(thread);
                thread.resume();
                return;
            }

            // We also only support WEXITED
            if (options & 0x2 || options & 0x8)
            {
                // WSTOPPED and WCONTINUED aren't supported
                error = ENOTSUP;
                transfer_error(thread);
                thread.resume();
                return;
            }

            int op_ppid = thread.get_process()->pid;
            int op_pid = which == 1 ? id : -1; // P_PID or P_ALL
            int exit_status = 0;

            // WNOWAIT
            uint32_t res = scheduler.get_exit_status(op_ppid, op_pid, exit_status, !(options & 0x1000000));

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
                    if (infop_addr)
                        thread.get_process()->memory_space.memset(infop_addr, 0, sizeof(sys_siginfo));
                    set_return(thread, 0);
                    thread.resume();
                    return;
                }

                // Continue pausing...
                // If we don't unpause, it will keep calling this callback
                // so we do nothing to keep waiting.
                return;
            }

            // Process has exited
            sys_siginfo siginfo;
            siginfo.signo = 17; // SIGCHLD
            siginfo.fields.child.pid = res;
            

            // TODO: UID
            siginfo.fields.child.uid = 0; // Assuming UID 0 for now

            // Combine both signal and code parts of exit status, since only one is ever set at a time,
            // and the spec says that this represents both, interpreted depending on the context.
            siginfo.fields.child.status = (exit_status & (exit_status >> 8)) & 0xFF;
            siginfo.code = exit_status & 0xFF ? 2 : 1; // 1 for exit with code, 2 for signalled exit

            // Copy to userspace

            if (infop_addr)
                thread.get_process()->memory_space.memcpy(infop_addr, &siginfo, sizeof(sys_siginfo));

            if (ru_addr)
                thread.get_process()->memory_space.memset(ru_addr, 0, sizeof(sys_rusage));

            set_return(thread, 0);
            thread.resume();

            // done.
        }
    }

    int sys_waitid(Thread &thread)
    {
        thread.pause(waitid_callback);
        return 0;
    }
} // namespace Hamster

