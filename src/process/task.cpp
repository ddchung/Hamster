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

    Task *Task::get_task_pgid(uint32_t pgid)
    {
        // first, check task with same TID
        auto it = tasks.find(pgid);
        if (it != tasks.end() && it->second->get_pgid() == pgid)
            return it->second;

        // next, search all tasks
        for (const auto &[tid, task] : tasks)
            if (task->get_pgid() == pgid)
                return task;

        // not found
        error = H_ESRCH;
        return nullptr;
    }

    Process *Task::get_init_process()
    {
        auto it = tasks.find(1);
        if (it == tasks.end())
            return nullptr;
        return &it->second->process.get();
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
        // TODO: clear_child_tid
        
        if (is_vfork && parent)
            parent->interrupt_block();

        process->remove_task(this);

        if (process->num_tasks() == 0)
            process->exit(code);

        return 0;
    }

    Task *Task::clone(uint32_t flags, uint32_t stack, uint32_t ptid_loc, uint32_t tls, uint32_t ctid_loc)
    {
        // validate arguments

        if (((flags & H_CLONE_SIGHAND) && !(flags & H_CLONE_VM))
         || ((flags & H_CLONE_THREAD) != (flags & H_CLONE_SIGHAND)) // Note: in Hamster, if you have one you must have both.
         || ((flags & H_CLONE_FS) && (flags & H_CLONE_NEWNS))
         || ((flags & H_CLONE_NEWUSER) && (flags & H_CLONE_FS))
         || ((flags & H_CLONE_NEWIPC) && (flags & H_CLONE_SYSVSEM))
         || ((flags & H_CLONE_NEWPID) && (flags & (H_CLONE_THREAD | H_CLONE_PARENT)))
         || ((flags & H_CLONE_NEWUSER) && (flags & H_CLONE_THREAD))
         || ((flags & H_CLONE_PARENT) && (getpid() == 1))
         || ((stack % 16))
         || ((flags & H_CLONE_PIDFD) && (flags & H_CLONE_DETACHED))
         || ((flags & H_CLONE_PIDFD) && (flags & H_CLONE_THREAD))
         || ((flags & H_CLONE_PIDFD) && (flags & H_CLONE_PARENT_SETTID)))
        {
            error = H_EINVAL;
            return nullptr;
        }

        // check for unsupported flags

        if (flags & ~(
            H_SIGCHLD
          | H_CLONE_CHILD_CLEARTID
          | H_CLONE_CHILD_SETTID
          | H_CLONE_FILES
          | H_CLONE_FS
          | H_CLONE_PARENT
          | H_CLONE_PARENT_SETTID
          | H_CLONE_SETTLS
          | H_CLONE_SIGHAND
          | H_CLONE_THREAD
          | H_CLONE_VFORK
          | H_CLONE_VM))
        {
            error = H_EINVAL;
            return nullptr;
        }

        // allocate TID

        uint32_t tid = next_tid++;

        Task *new_task = alloc<Task>();

        using enum SharedPtrCopyType;

        new_task->memory.assign(memory, flags & H_CLONE_VM ? SHALLOW : DEEP);
        new_task->fd_table.assign(fd_table, flags & H_CLONE_FILES ? SHALLOW : DEEP);
        new_task->signal_mask = signal_mask;
        new_task->emulator = emulator;
        new_task->emulator.memory = &new_task->memory->ms;
        new_task->emulator.flush_caches();
        new_task->tid = tid;
        new_task->parent = this;
        new_task->signal_saved_state = signal_saved_state;
        if (flags & H_CLONE_CHILD_CLEARTID)
            new_task->set_tid_address(ctid_loc);
        if (flags & H_CLONE_VFORK)
        {
            // Block "forever", until interrupted by child
            block([](Task &, uint64_t){}, 0, [](Task &, uint64_t){});
            new_task->is_vfork = true;
        }
        new_task->is_handling_signal = is_handling_signal;

        if ((flags & H_CLONE_VM) && !(flags & H_CLONE_VFORK))
            new_task->alt_signal_stack = {};
        else
            new_task->alt_signal_stack = alt_signal_stack;
        
        // initialize new task's process
        // Note that this also initializes the signal handlers,
        // since CLONE_THREAD requires CLONE_SIGHAND and vice versa.
        if (flags & H_CLONE_THREAD)
            new_task->process.assign(process, SHALLOW);
        else
        {
            SharedPtr<TaskFSInfo, size_t> fs_info;
            int uid, euid, suid;
            int gid, egid, sgid;

            fs_info.assign(process->get_fs_info(), flags & H_CLONE_FS ? SHALLOW : DEEP);
            process->get_uid(&uid, &euid, &suid);
            process->get_gid(&gid, &egid, &sgid);

            new_task->process.construct(tid, new_task, process->get_process_group(), process->get_signal_handlers(),
                                        fs_info, flags & H_CLONE_PARENT ? process->get_parent() : &this->process.get(),
                                        uid, euid, suid, gid, egid, sgid, process->get_groups());
        }

        // Store the TID if needed
        if (((flags & H_CLONE_CHILD_SETTID) && new_task->copy_to_memory(ctid_loc, tid) != 0)
         || ((flags & H_CLONE_PARENT_SETTID) && copy_to_memory(ptid_loc, tid) != 0))
        {
            dealloc(new_task);
            error = H_EFAULT;
            return nullptr;
        }

        // Set thread pointer (tp == x4)
        if (flags & H_CLONE_SETTLS)
            new_task->get_emulator().x[4] = tls;
        
        // Set stack if specified
        if (stack != 0)
            new_task->get_emulator().x[2] = stack;
        
        // Set return value to 0 in new task
        new_task->get_emulator().x[10] = 0;

        assert(tasks.find(tid) == tasks.end());
        tasks[tid] = new_task;
        new_task->add_to_scheduler();

        return new_task;
    }

    int Task::waitid(int idtype, uint32_t id, sys_siginfo *siginfo, int options)
    {
        auto &state_changes = process->get_state_changes();

        for (auto it = state_changes.begin(); it != state_changes.end(); ++it)
        {
            bool match = false;
            switch (idtype)
            {
            case H_P_PID:
                match = (it->pid == id);
                break;
            case H_P_PGID:
                match = (it->pgid == id);
                break;
            case H_P_ALL:
                match = true;
                break;
            default:
                error = H_EINVAL;
                return -1;
            }

            if (match)
            {
                // Found matching state change, check if it applies
                switch (it->type)
                {
                case ProcessStateChange::EXIT:
                    if ((options & H_WEXITED) == 0)
                        continue;
                    siginfo->fields.child.status = it->exit_code;
                    if (is_wait_exited(it->exit_code))
                        siginfo->code = H_CLD_EXITED;
                    else if (is_wait_terminated(it->exit_code))
                        siginfo->code = H_CLD_KILLED;
                    else
                        siginfo->code = H_CLD_DUMPED;
                    break;
                case ProcessStateChange::STOP:
                    if ((options & H_WSTOPPED) == 0)
                        continue;
                    siginfo->code = H_CLD_STOPPED;
                    siginfo->fields.child.status = it->signo;
                    break;
                case ProcessStateChange::CONT:
                    if ((options & H_WCONTINUED) == 0)
                        continue;
                    siginfo->code = H_CLD_CONTINUED;
                    siginfo->fields.child.status = it->signo;
                    break;
                }

                // OK, full match, return
                siginfo->fields.kill.pid = it->pid;
                siginfo->fields.kill.uid = it->uid;
                siginfo->signo = H_SIGCHLD;

                if ((options & H_WNOWAIT) == 0)
                {
                    // delete this state change
                    state_changes.erase(it);
                }

                return 0;
            }
        }

        // not found
        error = H_EAGAIN;
        return -1;
    }

    int Task::exit_group(uint16_t code)
    {
        return process->exit(code);
    }

    int Task::block(BlockingCallback callback, uint64_t saved, BlockingCallback interrupt_callback)
    {
        if (!callback)
        {
            error = H_EINVAL;
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

    int Task::block(int (*callback)(Task &, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t))
    {
        if (!callback)
        {
            error = H_EINVAL;
            return -1;
        }

        if (is_blocking())
        {
            error = H_EAGAIN;
            return -1;
        }

        blocking_operation_alt = callback;
        
        return block([](Task &task, uint64_t saved){
            uint32_t *x = task.emulator.x;
            int res = task.blocking_operation_alt(task, saved, x[11], x[12], x[13], x[14], x[15]);
            if (res == -1)
            {
                if (error == H_EAGAIN)
                    return;
                res = cvt_error();
            }

            x[10] = res;
            task.end_block();
            task.blocking_operation_alt = nullptr;
        }, emulator.x[10]); // save a0 because if this function is called from a 
        //                     system call, a0 will be overwritten by the call's return value
    }

    int Task::interrupt_block()
    {
        if (!is_blocking())
        {
            error = H_EPERM;
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

    void Task::run()
    {
        current_task = this;

        if (pending_signals.size() > 0 || process->get_pending_signals().size() > 0)
        {
            TaskSignalQueue *sigqueue = &pending_signals;
            const sys_siginfo *siginfo;

            // First, check task's signal queue
            siginfo = sigqueue->peek(signal_mask);

            // Check shared signal queue if not found
            if (!siginfo)
            {
                sigqueue = &process->get_pending_signals();
                siginfo = sigqueue->peek(signal_mask);
            }

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
                sigqueue->pop(signal_mask);
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
            last_instruction_tick = _get_sys_time();

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

        current_task = nullptr;
    }

    Task *Task::get_current_task()
    {
        return current_task;
    }

    uint64_t Task::get_last_tick()
    {
        return last_instruction_tick;
    }

    void Task::add_to_scheduler()
    {
        // Unique ID
        this->BaseKTask::id = (uint32_t)((uintptr_t)(this) >> 2);
        this->BaseKTask::next_tick = _get_sys_time();
        kscheduler.add_task(this);
    }

    void Task::pause(uint8_t signo)
    {
        for (Task *task : process->get_tasks())
        {
            task->is_paused = true;
        }
        process->notify_pause(signo);
    }

    void Task::unpause()
    {
        for (Task *task : process->get_tasks())
            task->is_paused = false;
        process->notify_continue();
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
        if (parent)
            parent->children.insert(this);

        // also have the leader task in `tasks`
        tasks.emplace(leader);

        this->pgroup->add_process(this);
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
                state_change.pid = pid;
                state_change.uid = uid;
                state_change.pgid = pgroup->get_pgid();

                if (is_wait_exited(code) || is_wait_terminated(code) || is_wait_terminated_coredump(code))
                {
                    state_change.type = ProcessStateChange::EXIT;
                    state_change.exit_code = code;
                }
                else if (is_wait_stopped(code))
                {
                    state_change.type = ProcessStateChange::STOP;
                    // retrieve signal number
                    // see make_wait_stopped in abi/values.hpp
                    state_change.signo = code >> 8;
                }
                else
                {
                    state_change.type = ProcessStateChange::CONT;
                    state_change.signo = H_SIGCONT;
                }

                parent->children.erase(this);
            }

            pgroup->remove_process(this);
            
            // make init adopt children
            Process *init = Task::get_init_process();
            if (init)
                for (Process *child : children)
                {
                    child->parent = init;
                    init->children.insert(child);
                }
        }

        return 0;
    }

    void Process::notify_pause(uint8_t signo)
    {
        if (parent)
        {
            parent->state_changes.emplace_back();
            auto &state_change = parent->state_changes.back();
            state_change.pid = pid;
            state_change.uid = uid;
            state_change.pgid = pgroup->get_pgid();
            state_change.type = ProcessStateChange::STOP;
            state_change.signo = signo;
        }
    }

    void Process::notify_continue()
    {
        if (parent)
        {
            parent->state_changes.emplace_back();
            auto &state_change = parent->state_changes.back();
            state_change.pid = pid;
            state_change.uid = uid;
            state_change.pgid = pgroup->get_pgid();
            state_change.type = ProcessStateChange::CONT;
            state_change.signo = H_SIGCONT;
        }
    }
} // namespace Hamster

