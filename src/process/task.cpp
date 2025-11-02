// Hamster tasks

#include <process/task.hpp>
#include <memory/allocator.hpp>
#include <platform/platform.hpp>
#include <syscall/syscall.hpp>
#include <errno/errno.h>
#include <cassert>
#include <cinttypes>

namespace Hamster
{
    Task *Task::create_task(int fd, const char *const *argv, const char *const *envp)
    {
        Task *task = alloc<Task>();

        uint32_t tid = next_tid++;

        task->tid = tid;

        // Note: we don't add the task to the process yet, since the Process constructor
        //       will handler that since we passed `task` as the leader
        task->process.construct(tid, task);
        task->memory.construct();
        task->fd_table.construct();
        // Note: task->emulator.memory is a weak pointer, which
        //       is destroyed before task->memory
        task->emulator.memory = &task->memory->ms;

        tasks[tid] = task;
        task->add_to_scheduler();

        int res = task->exec(fd, argv, envp);
        if (res < 0)
        {
            task->exit(make_wait_terminated_coredump(H_SIGKILL));
            dealloc(task);
            return nullptr;
        }

        return task;
    }

    Task *Task::get_task(uint32_t tid)
    {
        auto it = tasks.find(tid);
        if (it == tasks.end())
        {
            error = H_ESRCH;
            return nullptr;
        }

        assert(it->second != nullptr);
        assert(it->second->get_tid() == tid);

        return it->second;
    }

    Task *Task::get_task_pid(uint32_t pid)
    {
        // first, check task with same TID
        auto it = tasks.find(pid);
        if (it != tasks.end() && it->second->get_pid() == pid)
            return it->second;

        // next, search all tasks
        for (const auto &[tid, task] : tasks)
            if (task->get_pid() == pid)
                return task;

        // not found
        error = H_ESRCH;
        return nullptr;
    }
    
    int Task::exit(uint16_t code)
    {
        if (flags & (KSCHED_REMOVE_NOW | KSCHED_REMOVE_ALL))
        {
            // already exited
            error = H_EINVAL;
            return -1;
        }

        // If PID 1 (init) exits, bring down whole system
        if (tid == 1)
            flags |= KSCHED_REMOVE_ALL;
        else
            flags |= KSCHED_REMOVE_NOW;

        assert(tasks.find(tid)->second == this);
        tasks.erase(tid);

        // TODO: traverse robust futex list

        process->remove_task(this);

        if (process->num_tasks() == 0)
            process->exit(code);

        return 0;
    }

    int Task::exit_group(uint16_t code)
    {
        return process->exit(code);
    }

    int Task::block(BlockingCallback callback, uint64_t saved, BlockingCallback interrupt_callback)
    {
        if (!callback)
        {
            error = EINVAL;
            return -1;
        }

        if (is_blocking())
        {
            error = H_EAGAIN;
            return -1;
        }

        if (!interrupt_callback)
            interrupt_callback = [](Task &task, uint64_t saved) {
                task.get_emulator().x[10] = -H_EINTR;
                task.end_block();
            };
    
        blocking_operation = callback;
        blocking_operation_saved = saved;
        interrupt_blocking = interrupt_callback;

        return 0;
    }

    int Task::interrupt_block()
    {
        if (!is_blocking())
        {
            error = EPERM;
            return -1;
        }

        interrupt_blocking(*this, blocking_operation_saved);
        return 0;
    }

    void Task::end_block()
    {
        blocking_operation = nullptr;
        interrupt_blocking = nullptr;
    }

    bool Task::is_leader() const
    {
        return process->get_leader() == this;
    }

    const Set<Task *> &Task::get_process_tasks() const
    {
        return process->get_tasks();
    }

    const Set<Process *> &Task::get_children_processes() const
    {
        return process->get_children();
    }

    Task::~Task()
    {
        assert(flags & (KSCHED_REMOVE_NOW | KSCHED_REMOVE_ALL));
    }

    void Task::run()
    {
        if (pending_signals.size() > 0)
        {
            const sys_siginfo *siginfo = pending_signals.peek(signal_mask);
            if (siginfo)
            {
                assert(siginfo->signo >= 1 && siginfo->signo <= 64);

                // stop any blocking operation in progress
                if (blocking_operation)
                {
                    interrupt_block();
                    end_block();
                }

                // handle the signal
                int res = process->get_signal_handlers()->handle_signal(siginfo->signo, *this, *siginfo);
                assert(res == 0);
                pending_signals.pop(signal_mask);
                return;
            }

            // Signals present, but masked, continue normally
        }

        if (blocking_operation)
        {
            blocking_operation(*this, blocking_operation_saved);

            // Limit blocking checks to once a millisecond
            this->BaseKTask::next_tick = _get_sys_time() + 1;
        }
        else
        {
            auto status = emulator.run();

            using Status = RiscVEmulator::ExecuteResult::Status;

            switch (status.status)
            {
            case Status::Success:
                break;
            case Status::ECALL:
                emulator.x[10] = syscall(*this, emulator.x[17]);
                break;
            case Status::EBREAK:
                // TODO: EBREAK
                _trace("TID %" PRIu32 ": EBREAK\n", tid);
                break;
            case Status::IllegalInstruction:
            case Status::IllegalLoad:
            case Status::IllegalStore:
            case Status::Error:
                // TODO: Send SIGILL, SIGBUS, SIGSEGV
                _trace("TID %" PRIu32 ": ERROR!\n", tid);
                this->exit(make_wait_terminated_coredump(H_SIGKILL));
                break;
            }
        }

        // Update next tick
        this->BaseKTask::next_tick = _get_sys_time();
    }

    void Task::add_to_scheduler()
    {
        // Unique ID
        this->BaseKTask::id = (uint32_t)((uintptr_t)(this) >> 2);
        this->BaseKTask::next_tick = _get_sys_time();
        kscheduler.add_task(this);
    }
    
    Session::Session(uint32_t sid)
        : sid(sid)
    {
    }

    ProcessGroup::ProcessGroup(uint32_t pgid, const SharedPtr<Session> &session)
        : session(session, SharedPtrCopyType::SHALLOW), pgid(pgid)
    {
        if (!session)
            this->session.construct(pgid);
        this->session->add_process_group(this);
    }

    const SharedPtr<ProcessGroup> &ProcessGroup::get_shared_ptr() const
    {
        return (*processes.begin())->get_process_group();
    }

    ProcessGroup::~ProcessGroup()
    {
        session->remove_process_group(this);
    }

    Process::Process(uint32_t pid, Task *leader, const SharedPtr<ProcessGroup> &pgroup,
                const SharedPtr<TaskSignalHandlers> &signal_handlers,
                const SharedPtr<TaskFSInfo> &fs_info, Process *parent, int uid, int euid, int suid,
                int gid, int egid, int sgid, const Vector<int> &groups)
        : pgroup(pgroup, SharedPtrCopyType::SHALLOW), signal_handlers(signal_handlers, SharedPtrCopyType::SHALLOW),
          fs_info(fs_info, SharedPtrCopyType::SHALLOW), leader(leader), parent(parent), pid(pid), uid(uid), euid(euid), suid(suid),
          gid(gid), egid(egid), sgid(sgid), groups(groups)
    {
        // Construct new members if ones weren't provided
        if (!pgroup)
            this->pgroup.construct(pid);
        if (!signal_handlers)
            this->signal_handlers.construct();
        if (!fs_info)
            this->fs_info.construct();

        // also have the leader task in `tasks`
        tasks.emplace(leader);

        this->pgroup->add_process(this);
    }

    Process::~Process()
    {
        pgroup->remove_process(this);
    }

    void Process::add_task(Task *task)
    {
        tasks.emplace(task);
    }

    void Process::remove_task(Task *task)
    {
        if (leader == task)
            leader = nullptr;
        tasks.erase(task);
    }

    int Process::exit(uint16_t code)
    {
        if (num_tasks() > 0)
        {
            // Make all tasks exit
            // Note that `Task::exit` will remove the task from `this->tasks`
            for (auto it = tasks.begin(); it != tasks.end();)
                (*it++)->exit(code);
            return 0;
        }
        else
        {
            // Notify parent of exit
            if (parent)
            {
                parent->state_changes.emplace_back();
                auto &state_change = parent->state_changes.back();
                state_change.type = ProcessStateChange::EXIT;
                state_change.exit_code = code;
                state_change.pid = pid;
            }
        }

        return 0;
    }
} // namespace Hamster

