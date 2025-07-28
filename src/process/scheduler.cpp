// Hamster scheduler

#include <process/scheduler.hpp>
#include <process/task.hpp>
#include <errno/errno.h>
#include <memory/allocator.hpp>
#include <syscall/syscall.hpp>
#include <cstring>

namespace Hamster
{
    uint32_t Scheduler::add_task(Task *task)
    {
        if (!task)
        {
            error = EINVAL;
            return 0;
        }

        tasks[next_tid] = task;
        task->init_tid(next_tid);

        return next_tid++;
    }

    Task *Scheduler::get_task(uint32_t tid)
    {
        auto it = tasks.find(tid);
        if (it != tasks.end())
        {
            return it->second;
        }
        error = ESRCH;
        return nullptr;
    }

    int Scheduler::tick()
    {
        // Tick loop
        for (auto &[tid, task] : tasks)
        {
            current_task = task;

            if (task->is_paused || task->is_dead)
                continue;

            int result = do_tick(*task);
            if (result < 0)
            {
                return -1;
            }
        }

        current_task = nullptr;

        // Remove loop
        for (auto it = tasks.begin(); it != tasks.end();)
        {
            if (it->second->is_dead && it->first != it->second->get_pid())
            {
                // Remove dead non-leader tasks
                dealloc(it->second);
                it = tasks.erase(it);
            }
            else if (it->second->is_dead && it->second->process->obj.tasks.size() == 1)
            {
                // Remove dead leader tasks, but only if they are the last task in the process
                dealloc(it->second);
                it = tasks.erase(it);
            }
            else
            {
                ++it;
            }
        }

        return 0;
    }

    static const char *empty_strings[] = {nullptr};

    int Scheduler::spawn(const char *path, const char *const *argv, const char *const *envp, int dirfd)
    {
        if (!path)
        {
            error = EBADF;
            return -1;
        }

        if (!argv) argv = empty_strings;
        if (!envp) envp = empty_strings;

        Task *task = alloc<Task>();

        task->memory = make_task_member<EmulatorMemory>();
        task->program_brk = make_task_member<uint32_t>();
        task->emulator.memory = &task->memory->obj;
        task->fd_table = make_task_member<FDTable>();
        task->process = make_task_member<Process>();

        memset(task->emulator.x, 0, sizeof(task->emulator.x));

        task->process->obj.pg = make_task_member<ProcessGroup>();
        task->process->obj.signal_handlers = make_task_member<SignalHandlers>();
        task->process->obj.set_default_signal_handlers();
        task->process->obj.fs_info = make_task_member<FSInfo>();

        task->process->obj.pg->obj.session = make_task_member<Session>();
        task->process->obj.pg->obj.pgid = next_tid;
        task->process->obj.pg->obj.session->obj.sid = next_tid;
        task->process->obj.pid = next_tid;
        task->process->obj.ppid = 0;

        task->process->obj.uid = 0;
        task->process->obj.euid = 0;
        task->process->obj.gid = 0;
        task->process->obj.egid = 0;

        task->process->obj.tasks.push_back(task);

        uint32_t tid = add_task(task);

        if (tid == 0)
        {
            dealloc(task);
            return -1;
        }

        int ret = task->process->obj.exec(path, argv, envp, dirfd);

        if (ret < 0)
        {
            dealloc(task);
            return -1;
        }

        return tid;
    }

    int Scheduler::adopt_children(uint32_t pid)
    {
        for (auto &[_, task] : tasks)
        {
            if (task->process->obj.ppid == pid)
            {
                task->process->obj.ppid = 1; // Adopt by init
            }
        }
        return 0;
    }

    Process *Scheduler::get_process(uint32_t pid)
    {
        // First check TID for quick access
        auto it = tasks.find(pid);
        if (it != tasks.end())
        {
            if (it->second->process->obj.pid == pid)
            {
                return &it->second->process->obj;
            }
        }

        // If not found, check all tasks
        for (auto &[_, task] : tasks)
        {
            if (task->process->obj.pid == pid)
            {
                return &task->process->obj;
            }
        }

        // Still not found, no process with that PID
        error = ESRCH;
        return nullptr;
    }

    int Scheduler::do_tick(Task &task)
    {
        if (task.is_dead)
            return 0; // Skip dead tasks

        // Handle any pending signals

        if (!task.sig_queue.empty())
        {
            sys_siginfo siginfo = task.sig_queue.front();
            task.sig_queue.pop_front();

            // Check if the signal is valid
            if (siginfo.signo < 1 || siginfo.signo > 31)
            {
                return -1;
            }

            // Run if not blocked
            if (task.sig_mask & (1 << (siginfo.signo - 1)))
            {
                auto &handler = task.process->obj.signal_handlers->obj.sig_handlers[siginfo.signo];
                if (handler.fn)
                    handler.fn(&task, &siginfo, handler.data);
            }

            return 0;
        }

        if (task.blocking_operation == BlockingOperation::IO_READ)
        {
            return task.poll_read();
        }
        else if (task.blocking_operation == BlockingOperation::IO_WRITE)
        {
            return task.poll_write();
        }
        else if (task.blocking_operation == BlockingOperation::WAIT)
        {
            return task.poll_wait();
        }

        // Execute the task's instruction
        auto result = task.emulator.execute();
        if (result.status == RiscVEmulator::ExecuteResult::Status::Success)
        {
            // Successful execution, continue
            return 0;
        }
        else if (result.status == RiscVEmulator::ExecuteResult::Status::ECALL)
        {
            // Handle system call
            int32_t syscall_id = task.emulator.x[17]; // a7 is syscall ID
            int32_t syscall_result = Hamster::syscall(syscall_id);
            task.emulator.x[10] = syscall_result; // a0 is syscall return value
            return 0;
        }
        else if (result.status == RiscVEmulator::ExecuteResult::Status::EBREAK)
        {
            // TODO: Handle EBREAK
            return 0;
        }

        sys_siginfo siginfo = {};

        if (result.status == RiscVEmulator::ExecuteResult::Status::IllegalInstruction)
        {
            siginfo.signo = H_SIGILL;
            siginfo.errno_value = 0;
            siginfo.code = H_ILL_ILLOPC;

            // - 4, because the PC was incremented
            siginfo.fields.fault.addr = task.emulator.pc - 4;

            task.send_signal(siginfo);
            return 0;
        }
        else if (result.status == RiscVEmulator::ExecuteResult::Status::IllegalLoad)
        {
            siginfo.signo = H_SIGSEGV;
            siginfo.errno_value = 0;
            siginfo.code = H_SEGV_MAPERR;

            // - 4, because the PC was incremented
            siginfo.fields.fault.addr = task.emulator.pc - 4;

            task.send_signal(siginfo);
            return 0;
        }
        else if (result.status == RiscVEmulator::ExecuteResult::Status::IllegalStore)
        {
            siginfo.signo = H_SIGSEGV;
            siginfo.errno_value = 0;
            siginfo.code = H_SEGV_ACCERR;

            // - 4, because the PC was incremented
            siginfo.fields.fault.addr = task.emulator.pc - 4;

            task.send_signal(siginfo);
            return 0;
        }
        else if (result.status == RiscVEmulator::ExecuteResult::Status::Error)
        {
            // Error means that there was an internal error in the emulator
            // so, try again next time
            return -1;
        }

        return -1;
    }
} // namespace Hamster

