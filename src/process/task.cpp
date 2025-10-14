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
        task->emulator.memory = &task->get_memory();

        tasks[tid] = task;
        task->add_to_scheduler();

        int res = task->process->exec(fd, argv, envp);
        if (res < 0)
        {
            task->exit();
            dealloc(task);
            return nullptr;
        }

        return task;
    }
    
    int Task::exit()
    {
        if (flags & (KSCHED_REMOVE_NOW | KSCHED_REMOVE_ALL))
        {
            // already exited
            error = EINVAL;
            return -1;
        }

        // If PID 1 (init) exits, bring down whole system
        if (tid == 1)
            flags |= KSCHED_REMOVE_ALL;
        else
            flags |= KSCHED_REMOVE_NOW;

        assert(tasks.find(tid)->second == this);
        tasks.erase(tid);

        process->remove_task(this);

        return 0;
    }

    int Task::open_rel_fd(int thread_dfd, const char *path)
    {
        BaseTaskFD *fd;
        if (thread_dfd >= 0)
            fd = fd_table->get_fd(thread_dfd);
        else if (thread_dfd == H_AT_FDCWD)
            fd = nullptr;
        else
        {
            error = EBADF;
            return -1;
        }

        return process->get_fs_info()->open_rel_fd(path, fd);
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
                    assert(interrupt_blocking);
                    interrupt_blocking(*this);

                    blocking_operation = nullptr;
                    interrupt_blocking = nullptr;
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
            blocking_operation(*this);

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
                _trace("TID %" PRIu32 ": ERROR!\n", tid);
                this->exit();
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
        for (Task *task : tasks)
        {
            task->exit();
        }

        tasks.clear();
        leader = nullptr;

        if (parent)
        {
            parent->state_changes.emplace_back();
            auto &state_change = parent->state_changes.back();
            state_change.type = ProcessStateChange::EXIT;
            state_change.exit_code = code;
            state_change.pid = pid;
        }

        return 0;
    }
} // namespace Hamster

