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

    Process *Task::get_process(uint32_t pid)
    {
        // first, check task with same TID
        auto it = tasks.find(pid);
        if (it != tasks.end() && it->second->get_pid() == pid)
        {
            return &it->second->process.get();
        }

        // next, search all tasks
        for (const auto &[tid, task] : tasks)
        {
            if (task->get_pid() == pid)
            {
                return &task->process.get();
            }
        }

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

    int Task::accessat(int dirfd, const char *pathname, int mode, int flags)
    {
        int uid, gid, *groups;

        if (flags & H_AT_EACCESS)
        {
            process->get_uid(nullptr, &uid, nullptr);
            process->get_gid(nullptr, &gid, nullptr);
        }
        else
        {
            process->get_uid(&uid, nullptr, nullptr);
            process->get_gid(&gid, nullptr, nullptr);
        }

        // Put `gid` into groups
        groups = alloc<int>(process->get_groups().size() + 1);
        ::memcpy(groups, process->get_groups().data(), process->get_groups().size() * sizeof(int));
        groups[process->get_groups().size()] = gid;

        // Only forward AT_SYMLINK_NOFOLLOW
        int res = vfs.accessat(dirfd, pathname, uid, groups, process->get_groups().size() + 1, mode, flags & H_AT_SYMLINK_NOFOLLOW);
        dealloc(groups);
        return res;
    }

    int Task::exit_group(uint16_t code)
    {
        return process->exit(code);
    }

    int Task::open_rel_fd(int thread_dfd, const char *path)
    {
        BaseTaskFD *fd;
        if (thread_dfd >= 0)
            fd = get_fd(thread_dfd);
        else if (thread_dfd == H_AT_FDCWD)
            fd = nullptr;
        else
        {
            error = H_EBADF;
            return -1;
        }

        return process->get_fs_info()->open_rel_fd(path, fd);
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

    BaseTaskFD *Task::get_fd(int fd) const
    {
        return fd_table->get_fd(fd);
    }

    int Task::close_fd(int fd)
    {
        return fd_table->close(fd);
    }

    void Task::get_uid(int *uid, int *euid, int *suid) const
    {
        process->get_uid(uid, euid, suid);
    }

    void Task::get_gid(int *gid, int *egid, int *sgid) const
    {
        process->get_gid(gid, egid, sgid);
    }

    void Task::set_uid(int uid, int euid, int suid)
    {
        process->set_uid(uid, euid, suid);
    }

    void Task::set_gid(int gid, int egid, int sgid)
    {
        process->set_gid(gid, egid, sgid);
    }

    char *Task::mem_get_string(uint32_t addr)
    {
        return memory->ms.get_string(addr);
    }

    int Task::mask_mode(int mode) const
    {
        return process->get_fs_info()->mask_mode(mode);
    }

    int Task::allocate_fd(int start)
    {
        return fd_table->allocate_fd(start);
    }

    int Task::set_fd(BaseTaskFD *task_fd, int fd)
    {
        return fd_table->set_fd(task_fd, fd);
    }

    const void *Task::mem_make_iterator_read(uint32_t addr)
    {
        return memory->ms.make_iterator_read(addr);
    }

    void *Task::mem_make_iterator(uint32_t addr)
    {
        return memory->ms.make_iterator(addr);
    }

    int8_t Task::mem_get_permissions(uint32_t addr, uint32_t size)
    {
        return memory->ms.get_permissions(addr, size);
    }

    void Task::close_cloexec_fds()
    {
        fd_table->close_cloexec();
    }

    uint32_t Task::mbrk(uint32_t brk)
    {
        if (brk == 0)
            return memory->brk;

        if (brk < memory->brk)
        {
            // Shrink brk

            // Note: align `brk` up to page size. `unmap` rounds down automatically
            if (memory->ms.unmap(brk + (HAMSTER_PAGE_SIZE - 1),
                                 memory->brk - brk) < 0)
            {
                return memory->brk;
            }
        }
        else if (brk > memory->brk)
        {
            // Expand brk

            // same thing for `memory->brk`
            if (memory->ms.map_anonymous(memory->brk + (HAMSTER_PAGE_SIZE - 1),
                                        brk - memory->brk, PERM_READ | PERM_WRITE) < 0)
            {
                return memory->brk;
            }
        }

        memory->brk = brk;
        return memory->brk;
    }

    int Task::sigaltstack(const sys_sigaltstack *new_stack, sys_sigaltstack *old_stack)
    {
        if (old_stack)
            *old_stack = alt_signal_stack;
        if (new_stack)
            alt_signal_stack = *new_stack;
        return 0;
    }

    void Task::set_tid_address(uint32_t tid_addr)
    {
        clear_child_tid = tid_addr;
    }

    void Task::set_robust_list(uint32_t head)
    {
        robust_list = head;
    }

    int Task::set_pgid(uint32_t pgid)
    {
        return process->set_pgid(pgid);
    }
    int Task::memcpy(uint32_t dest, const void *src, uint32_t n)
    {
        return memory->ms.memcpy(dest, src, n);
    }

    int Task::memcpy(void *dest, uint32_t src, uint32_t n)
    {
        return memory->ms.memcpy(dest, src, n);
    }

    int Task::memset(uint32_t addr, uint8_t value, uint32_t n)
    {
        return memory->ms.memset(addr, value, n);
    }

    int Task::mem_is_mapped(uint32_t addr, uint32_t size) const
    {
        return memory->ms.is_mapped(addr, size);
    }

    int Task::munmap(uint32_t addr, uint32_t size)
    {
        emulator.flush_caches();
        return memory->ms.unmap(addr, size);
    }

    int Task::munmap_all()
    {
        emulator.flush_caches();
        return memory->ms.unmap_all();
    }

    int Task::mprotect(uint32_t addr, uint32_t size, uint8_t permissions)
    {
        return memory->ms.mprotect(addr, size, permissions);
    }

    int Task::futex_wait(uint32_t addr, void (*callback)())
    {
        return memory->ms.futex_wait(addr, callback);
    }

    int Task::futex_wake(uint32_t addr, uint32_t count)
    {
        return memory->ms.futex_wake(addr, count);
    }

    int Task::futex_requeue(uint32_t wake_addr, uint32_t wake_count, uint32_t requeue_addr, uint32_t requeue_count)
    {
        return memory->ms.futex_requeue(wake_addr, wake_count, requeue_addr, requeue_count);
    }

    int Task::dup_fd(int fd, int new_fd)
    {
        return fd_table->dup(fd, new_fd);
    }

    void Task::clear_fds()
    {
        fd_table->clear();
    }

    int Task::send_signal(const sys_siginfo &siginfo)
    {
        return pending_signals.push(siginfo);
    }

    int Task::send_signal_process(const sys_siginfo &siginfo)
    {
        return process->get_pending_signals().push(siginfo);
    }

    size_t Task::pending_signals_size() const
    {
        return pending_signals.size();
    }

    size_t Task::pending_signals_size_process() const
    {
        return process->get_pending_signals().size();
    }

    void Task::block_signal(uint8_t signo)
    {
        signal_mask.block(signo);
    }

    void Task::unblock_signal(uint8_t signo)
    {
        signal_mask.unblock(signo);
    }

    void Task::set_signal_blocked(uint8_t signo, bool blocked)
    {
        signal_mask.set_blocked(signo, blocked);
    }

    int Task::is_signal_blocked(uint8_t signo) const
    {
        return signal_mask.check(signo);
    }

    uint64_t Task::get_signal_mask(bool invert) const
    {
        return signal_mask.convert(invert);
    }

    sys_sigset Task::get_signal_sigset() const
    {
        return signal_mask.to_sigset();
    }

    int Task::is_signal_handler(uint8_t signo)
    {
        return process->get_signal_handlers()->is_handler(signo);
    }

    int Task::is_signal_ignored(uint8_t signo)
    {
        return process->get_signal_handlers()->is_ignored(signo);
    }

    int Task::is_signal_default(uint8_t signo)
    {
        return process->get_signal_handlers()->is_default(signo);
    }

    int Task::chroot(const char *path)
    {
        return process->get_fs_info()->chroot(path);
    }

    int Task::chdir(const char *path)
    {
        return process->get_fs_info()->chdir(path);
    }

    char *Task::getcwd()
    {
        return process->get_fs_info()->getcwd();
    }

    char *Task::get_abs_cwd()
    {
        return process->get_fs_info()->get_abs_cwd();
    }

    int Task::get_umask() const
    {
        return process->get_fs_info()->get_umask();
    }

    void Task::set_umask(int new_umask)
    {
        process->get_fs_info()->set_umask(new_umask);
    }

    bool Task::is_leader() const
    {
        return process->get_leader() == this;
    }

    uint32_t Task::get_pid() const
    {
        return process->get_pid();
    }

    uint32_t Task::get_ppid() const
    {
        return process->get_ppid();
    }

    uint32_t Task::get_pgid() const
    {
        return process->get_process_group()->get_pgid();
    }

    uint32_t Task::get_sid() const
    {
        return process->get_process_group()->get_session()->get_sid();
    }

    int Task::set_groups(const Vector<int> &groups)
    {
        return process->set_groups(groups);
    }

    const Set<Task *> &Task::get_process_tasks() const
    {
        return process->get_tasks();
    }

    const Set<Process *> &Task::get_children_processes() const
    {
        return process->get_children();
    }

    int Task::setsid()
    {
        return process->setsid();
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

    int Process::set_pgid(uint32_t pgid)
    {
        if (pgid == 0)
            pgid = pid;

        const auto &pgroups = pgroup->get_session()->get_process_groups();

        for (ProcessGroup *pg : pgroups)
        {
            if (pg->get_pgid() == pgid)
            {
                // Found existing process group
                pgroup->remove_process(this);
                pgroup.assign(pg->get_shared_ptr(), SharedPtrCopyType::SHALLOW);
                pgroup->add_process(this);
                return 0;
            }
        }

        // not found
        error = H_EPERM;
        return -1;
    }

    int Process::setsid()
    {
        if (pgroup->get_pgid() == pid)
        {
            // already a pgroup leader
            error = H_EPERM;
            return -1;
        }

        // make new process group with new session
        pgroup->remove_process(this);
        pgroup.construct(pid);
        pgroup->add_process(this);

        return 0;
    }

    void Process::get_uid(int *uid, int *euid, int *suid) const
    {
        if (uid)
            *uid = this->uid;
        if (euid)
            *euid = this->euid;
        if (suid)
            *suid = this->suid;
    }

    void Process::get_gid(int *gid, int *egid, int *sgid) const
    {
        if (gid)
            *gid = this->gid;
        if (egid)
            *egid = this->egid;
        if (sgid)
            *sgid = this->sgid;
    }

    void Process::set_uid(int uid, int euid, int suid)
    {
        if (uid != -1)
            this->uid = uid;
        if (euid != -1)
            this->euid = euid;
        if (suid != -1)
            this->suid = suid;
    }

    void Process::set_gid(int gid, int egid, int sgid)
    {
        if (gid != -1)
            this->gid = gid;
        if (egid != -1)
            this->egid = egid;
        if (sgid != -1)
            this->sgid = sgid;
    }

    int Process::set_groups(const Vector<int> &groups)
    {
        this->groups = groups;
        return 0;
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

