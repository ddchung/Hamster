// Hamster waitid and wait4 system calls

#include <syscall/syscall.hpp>
#include <abi/structs.hpp>
#include <abi/syscall_id.hpp>
#include <process/scheduler.hpp>
#include <platform/platform.hpp>
#include <errno/errno.h>

namespace Hamster
{
    namespace
    {
        void poll_wait(Task &)
        {
            Task *current_task = scheduler.get_current_task();
            assert(current_task != nullptr && "No current task");

            // Call either waitid or wait4 syscall
            int32_t result;

            int32_t syscall_id = current_task->emulator.x[17]; // a7 register contains syscall ID

            // Restore argument 0 that is in io_block_fd
            current_task->emulator.x[10] = current_task->blocking_operation_saved[0];

            switch (syscall_id)
            {
            case SyscallID::WAITID:
                result = syscall(sys_waitid);
                break;
            case SyscallID::WAIT4:
                result = syscall(sys_wait4);
                break;
            default:
                result = -ENOSYS;
                break;
            }

            if (result < 0 && result == -EAGAIN)
            {
                // Still blocking, do nothing and check again next time
                return;
            }
            _trace("sys_wait: completed wait operation, result %d\n", result);

            // Completed successfully
            // Copy to a0 register (return value)
            current_task->emulator.x[10] = result;
            current_task->blocking_operation = nullptr;
        }
    }

    int32_t sys_waitid(int32_t idtype, int32_t id, uint32_t infop_loc,
                       int32_t options, uint32_t ru_loc)
    {
        uint32_t pid = 0;

        Task *current_task = scheduler.get_current_task();
        assert(current_task);

        Process &current_process = current_task->process->obj;

        switch (idtype)
        {
        case H_P_PID:
            pid = id; // Wait for specific process
            break;
        case H_P_PGID:
            pid = id == 0 ? current_process.get_pgid() : id; // Wait for process group
            break;
        case H_P_PIDFD:
        {
            if (id < 0 || (size_t)id >= current_task->fd_table->obj.fds.size())
            {
                return -EINVAL; // Invalid PIDFD
            }
            UserFD &fd = current_task->fd_table->obj.fds[id];
            if (fd.type != UserFDType::PID)
            {
                return -EINVAL; // Not a PIDFD
            }
            pid = fd.pid; // Get PID from PIDFD
        }
        break;
        case H_P_ALL:
            pid = 0; // Wait for any child process
            break;
        default:
            return -EINVAL; // Invalid idtype
        }

        Map<uint32_t, ProcessStateChange> &state_changes = current_process.children_state_changes;
        Map<uint32_t, ProcessStateChange>::iterator it;

        auto is_valid = [options](const ProcessStateChange &change)
        {
            if (options & H_WEXITED && change.type == ProcessStateChangeType::EXIT)
                return true;
            if (options & H_WEXITED && change.type == ProcessStateChangeType::TERMINATE)
                return true;
            if (options & H_WSTOPPED && change.type == ProcessStateChangeType::STOP)
                return true;
            if (options & H_WCONTINUED && change.type == ProcessStateChangeType::CONTINUE)
                return true;
            return false;
        };

        switch (idtype)
        {
        case H_P_ALL:
            // Wait for any child process
            it = state_changes.begin();
            while (it != state_changes.end() && !is_valid(it->second))
                ++it;
            break;
        case H_P_PID:
        case H_P_PIDFD:
            // Wait for specific process
            it = state_changes.find(pid);
            break;
        case H_P_PGID:
            // Wait for any process in the group
            it = state_changes.begin();
            while (it != state_changes.end() && it->second.gid != current_process.get_pgid() && !is_valid(it->second))
                ++it;
            break;
        default:
            __builtin_unreachable(); // Should not reach here
        };
        if (it == state_changes.end())
        {
            // Check if there are even any waitable processes
            bool found = false;
            switch (idtype)
            {
            case H_P_ALL:
                for (auto &[pid, task] : scheduler.get_tasks())
                {
                    if (task->process->obj.ppid == current_process.pid)
                    {
                        found = true;
                        break;
                    }
                }
                break;
            case H_P_PID:
            case H_P_PIDFD:
                for (auto &[pid, task] : scheduler.get_tasks())
                {
                    if (task->get_pid() == (uint32_t)id && task->process->obj.ppid == current_process.pid)
                    {
                        found = true;
                        break;
                    }
                }
                break;
            case H_P_PGID:
                for (auto &[pid, task] : scheduler.get_tasks())
                {
                    if (task->get_pgid() == current_process.get_pgid() && task->process->obj.ppid == current_process.pid)
                    {
                        found = true;
                        break;
                    }
                }
                break;
            default:
                __builtin_unreachable(); // Should not reach here
            }

            if (!found)
            {
                // No child processes to wait for
                return -ECHILD;
            }

            if (options & H_WNOHANG)
            {
                // No matching process found, and H_WNOHANG is set
                return 0;
            }
            
            if (!current_task->blocking_operation)
                _trace("sys_waitid: blocking on Thread PID %d, idtype %d, id %d, options %d\n",
                       current_task->get_pid(), idtype, id, options);

            // Block until a matching process state change occurs
            current_task->blocking_operation = poll_wait;

            // Save argument 0, as it will be overwritten by the return handler
            current_task->blocking_operation_saved[0] = idtype;


            return -EAGAIN;
        }

        // We have a matching process state change

        ProcessStateChange &change = it->second;
        if (infop_loc != 0)
        {
            sys_siginfo info;
            info.signo = H_SIGCHLD;

            switch (change.type)
            {
            case ProcessStateChangeType::EXIT:
                info.code = H_CLD_EXITED;
                break;
            case ProcessStateChangeType::STOP:
                info.code = H_CLD_STOPPED;
                break;
            case ProcessStateChangeType::CONTINUE:
                info.code = H_CLD_CONTINUED;
                break;
            case ProcessStateChangeType::TERMINATE:
                info.code = H_CLD_KILLED;
                break;
            default:
                info.code = H_CLD_TRAPPED; // Default case, should not happen
                break;
            }
            info.fields.child.pid = it->first;
            info.fields.child.uid = change.uid;
            info.fields.child.status = change.exit_code;
            info.fields.child.utime = 0; // Not implemented, set to 0
            info.fields.child.stime = 0; // Not implemented, set to 0

            if (current_task->memory->obj.memory.memcpy(infop_loc, &info, sizeof(sys_siginfo)) < 0)
            {
                error = EFAULT;
                return cvt_error();
            }
        }

        if (ru_loc != 0)
        {
            // Not implemented, set to 0
            if (current_task->memory->obj.memory.memset(ru_loc, 0, sizeof(sys_rusage)) < 0)
            {
                error = EFAULT;
                return cvt_error();
            }
        }

        // Remove if needed
        if (!(options & H_WNOWAIT))
        {
            state_changes.erase(it);
        }

        return 0;
    }
}
