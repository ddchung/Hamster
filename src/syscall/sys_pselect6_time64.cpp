// Hamster pselect6_time64 system call

#include <syscall/syscall.hpp>
#include <process/scheduler.hpp>
#include <filesystem/vfs.hpp>
#include <errno/errno.h>
#include <abi/structs.hpp>
#include <cstring>

namespace Hamster
{
    namespace
    {
        bool is_fd_set(uint32_t *fds, int fd)
        {
            return (fds[fd / 32] & (1u << (fd % 32))) != 0;
        }

        void poll_pselect6(Task &current_task)
        {
            // Call the system call
            current_task.emulator.x[10] = current_task.blocking_operation_saved[0]; // Restore nfds
            int32_t result = syscall(sys_pselect6_time64);

            if (result == -EAGAIN)
                // Still blocking, do nothing and check again next time
                return;
            _trace("sys_pselect6: completed pselect6 operation, result %d\n", result);
            // Completed 
            // Copy to a0 register (return value)
            current_task.emulator.x[10] = result;
            // Clear the blocking operation
            current_task.blocking_operation = nullptr;
        }
    } // namespace
    
    int32_t sys_pselect6_time64(int32_t nfds, uint32_t readfds_loc, uint32_t writefds_loc,
                                    uint32_t exceptfds_loc, uint32_t timeout_loc,
                                    uint32_t sigmask_loc)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr && "No current task");

        // Prepare file descriptor sets
        // Note that we will not support exceptfds for now
        uint32_t fdset_size = (nfds + 31) / 32; // Number of 32-bit words needed
        uint32_t *read_fds = (uint32_t*)alloca(fdset_size * sizeof(uint32_t));
        uint32_t *write_fds = (uint32_t*)alloca(fdset_size * sizeof(uint32_t));

        if (readfds_loc != 0)
        {
            if (current_task->copy_from_user(*read_fds, readfds_loc, fdset_size * sizeof(uint32_t)) < 0)
            {
                error = EFAULT;
                return cvt_error();
            }
        }
        if (writefds_loc != 0)
        {
            if (current_task->copy_from_user(*write_fds, writefds_loc, fdset_size * sizeof(uint32_t)) < 0)
            {
                error = EFAULT;
                return cvt_error();
            }
        }
        if (exceptfds_loc != 0)
        {
            // Mark all of them as not ready
            if (current_task->get_memory().memset(exceptfds_loc, 0, fdset_size * sizeof(uint32_t)) < 0)
            {
                error = EFAULT;
                return cvt_error();
            }
        }

        // check timeout
        if (timeout_loc != 0)
        {
            sys_timespec timeout = {};
            if (current_task->copy_from_user(timeout, timeout_loc) < 0)
            {
                error = EFAULT;
                return cvt_error();
            }

            // Check if we ran out of time
            uint64_t now = _get_sys_time();
            if (current_task->last_tick + timespec_to_systick(timeout) <= now)
            {
                // Timeout reached, mark all as not ready
                if ((readfds_loc != 0 &&
                    current_task->get_memory().memset(readfds_loc, 0, fdset_size * sizeof(uint32_t)) < 0)
                    || (writefds_loc != 0 &&
                    current_task->get_memory().memset(writefds_loc, 0, fdset_size * sizeof(uint32_t)) < 0))
                {
                    error = EFAULT;
                    return cvt_error();
                }
                return 0; // No file descriptors ready, return 0
            }
        }

        // Note: signal mask isn't implemented yet

        for (int i = 0; i < nfds; ++i)
        {
            if (!is_fd_set(read_fds, i) && !is_fd_set(write_fds, i))
                continue; // Skip if not in either set

            constexpr int READY_READ = 1, READY_WRITE = 2;
            int ready = 0;

            int vfs_fd = current_task->get_vfs_fd(i);
            if (vfs_fd < 0)
                return cvt_error();
            
            if (is_fd_set(read_fds, i))
            {
                if (vfs.poll(vfs_fd, 0x1) > 0)
                {
                    ready |= READY_READ; // File descriptor is ready for reading
                }
            }
            if (is_fd_set(write_fds, i))
            {
                if (vfs.poll(vfs_fd, 0x2) > 0)
                {
                    ready |= READY_WRITE; // File descriptor is ready for writing
                }
            }
            if (ready == 0)
                continue; // Not ready, skip

            memset(read_fds, 0, fdset_size * sizeof(uint32_t));
            memset(write_fds, 0, fdset_size * sizeof(uint32_t));
            
            // File descriptor is ready, update the sets
            if (ready & READY_READ)
            {
                read_fds[i / 32] |= (1u << (i % 32));
            }
            if (ready & READY_WRITE)
            {
                write_fds[i / 32] |= (1u << (i % 32));
            }

            // copy to user memory
            if (readfds_loc != 0 &&
                current_task->copy_to_user(*read_fds, readfds_loc, fdset_size * sizeof(uint32_t)) < 0)
            {
                error = EFAULT;
                return cvt_error();
            }
            if (writefds_loc != 0 &&
                current_task->copy_to_user(*write_fds, writefds_loc, fdset_size * sizeof(uint32_t)) < 0)
            {
                error = EFAULT;
                return cvt_error();
            }

            // Return the number of ready file descriptors
            if (ready == (READY_READ | READY_WRITE))
                return 2; // both are ready
            return 1; // only one is ready
        }

        // Block until a file descriptor is ready
        if (!current_task->blocking_operation)
            _trace("sys_pselect6: blocking on NFDS %d\n", nfds);
        current_task->blocking_operation = poll_pselect6;
        current_task->blocking_operation_saved[0] = nfds; // Save nfds for later

        // mark as blocked
        // This is intercepted by the blocking operation handler (poll_pselect6)
        return -EAGAIN;
    }
} // namespace Hamster

