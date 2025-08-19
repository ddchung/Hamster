
#include <process/task.hpp>
#include <process/scheduler.hpp>
#include <errno/errno.h>
#include <memory/allocator.hpp>
#include <abi/values.hpp>
#include <cassert>
#include <utility>
#include <cstring>
#include <algorithm>

#include <platform/platform.hpp>

namespace Hamster
{
    namespace
    {
        // Default signal handlers

        void sighand_nop(Task *, sys_siginfo *, sys_sigaction *)
        {
            Task *current_task = scheduler.get_current_task();
            if (current_task)
                _trace("TID %d received signal %d, doing nothing\n", current_task->tid, current_task->emulator.x[17]);
        }

        void sighand_term(Task *task, sys_siginfo *siginfo, sys_sigaction *)
        {
            assert(task);
            assert(siginfo);
            task->exit(make_wait_terminated(siginfo->signo));

            Task *current_task = scheduler.get_current_task();
            if (current_task)
                _trace("TID %d received signal %d, terminating\n", current_task->tid, siginfo->signo);
        }

        void sighand_dump(Task *task, sys_siginfo *siginfo, sys_sigaction *)
        {
            assert(task);
            assert(siginfo);
            task->exit(make_wait_terminated_coredump(siginfo->signo));

            Task *current_task = scheduler.get_current_task();
            if (current_task)
                _trace("TID %d received signal %d, terminating with core dump at PC 0x%08x\n", current_task->tid, siginfo->signo, current_task->emulator.pc);
        }

        void sighand_stop(Task *task, sys_siginfo * siginfo, sys_sigaction *)
        {
            assert(task);
            assert(siginfo);
            task->is_paused = true;

            Process *parent_process = scheduler.get_process(task->process->obj.ppid);

            if (parent_process)
            {
                ProcessStateChange state_change;
                state_change.type = ProcessStateChangeType::STOP;
                state_change.signal = siginfo->signo;
                parent_process->children_state_changes[task->get_pid()] = state_change;
            }

            Task *current_task = scheduler.get_current_task();
            if (current_task)
                _trace("TID %d received signal %d, stopping\n", current_task->tid, siginfo->signo);
        }

        void sighand_cont(Task *task, sys_siginfo *siginfo, sys_sigaction *)
        {
            assert(task);
            assert(siginfo);
            task->is_paused = false;

            Process *parent_process = scheduler.get_process(task->process->obj.ppid);

            if (parent_process)
            {
                ProcessStateChange state_change;
                state_change.type = ProcessStateChangeType::CONTINUE;
                state_change.signal = siginfo->signo;
                parent_process->children_state_changes[task->get_pid()] = state_change;
            }
        }

        constexpr SignalHandler sighand_default_nop = {sighand_nop, {.handler = H_SIG_DFL, .flags = 0, .restorer = 0, .mask = {}}};
        constexpr SignalHandler sighand_default_term = {sighand_term, {.handler = H_SIG_DFL, .flags = 0, .restorer = 0, .mask = {}}};
        constexpr SignalHandler sighand_default_dump = {sighand_dump, {.handler = H_SIG_DFL, .flags = 0, .restorer = 0, .mask = {}}};
        constexpr SignalHandler sighand_default_stop = {sighand_stop, {.handler = H_SIG_DFL, .flags = 0, .restorer = 0, .mask = {}}};
        constexpr SignalHandler sighand_default_cont = {sighand_cont, {.handler = H_SIG_DFL, .flags = 0, .restorer = 0, .mask = {}}};

        SignalHandler default_signal_handlers[64] = {
            {nullptr, {}},                // 0 - not used
            sighand_default_term,         // 1 - SIGHUP (terminate)
            sighand_default_term,         // 2 - SIGINT (terminate)
            sighand_default_term,         // 3 - SIGQUIT (terminate + core)
            sighand_default_dump,         // 4 - SIGILL (terminate + core)
            sighand_default_dump,         // 5 - SIGTRAP (terminate + core)
            sighand_default_dump,         // 6 - SIGABRT/SIGIOT (terminate + core)
            sighand_default_dump,         // 7 - SIGBUS (terminate + core)
            sighand_default_dump,         // 8 - SIGFPE (terminate + core)
            sighand_default_term,         // 9 - SIGKILL (terminate, cannot catch/ignore)
            sighand_default_term,         // 10 - SIGUSR1 (terminate)sighand_default_cont
            sighand_default_dump,         // 11 - SIGSEGV (terminate + core)
            sighand_default_term,         // 12 - SIGUSR2 (terminate)
            sighand_default_term,         // 13 - SIGPIPE (terminate)
            sighand_default_term,         // 14 - SIGALRM (terminate)
            sighand_default_term,         // 15 - SIGTERM (terminate)
            sighand_default_term,         // 16 - SIGSTKFLT (terminate)
            sighand_default_nop,          // 17 - SIGCHLD (ignore)
            sighand_default_cont,         // 18 - SIGCONT (continue, if stopped)
            sighand_default_stop,         // 19 - SIGSTOP (stop, cannot catch/ignore)
            sighand_default_stop,         // 20 - SIGTSTP (stop)
            sighand_default_stop,         // 21 - SIGTTIN (stop)
            sighand_default_stop,         // 22 - SIGTTOU (stop)
            sighand_default_nop,          // 23 - SIGURG (ignore)
            sighand_default_dump,         // 24 - SIGXCPU (terminate + core)
            sighand_default_dump,         // 25 - SIGXFSZ (terminate + core)
            sighand_default_term,         // 26 - SIGVTALRM (terminate)
            sighand_default_term,         // 27 - SIGPROF (terminate)
            sighand_default_nop,          // 28 - SIGWINCH (ignore)
            sighand_default_nop,          // 29 - SIGIO/SIGPOLL (ignore)
            sighand_default_term,         // 30 - SIGPWR (terminate)
            sighand_default_dump,         // 31 - SIGSYS (terminate + core)

            // 32 - 63 Realtime signals, NOP by default
            sighand_default_nop,          // 32 - SIGRTMIN
            sighand_default_nop,          // 33 - SIGRTMIN+1
            sighand_default_nop,          // 34 - SIGRTMIN+2
            sighand_default_nop,          // 35 - etc.
            sighand_default_nop,          // 36
            sighand_default_nop,          // 37
            sighand_default_nop,          // 38
            sighand_default_nop,          // 39
            sighand_default_nop,          // 40
            sighand_default_nop,          // 41
            sighand_default_nop,          // 42
            sighand_default_nop,          // 43
            sighand_default_nop,          // 44
            sighand_default_nop,          // 45
            sighand_default_nop,          // 46
            sighand_default_nop,          // 47
            sighand_default_nop,          // 48
            sighand_default_nop,          // 49
            sighand_default_nop,          // 50
            sighand_default_nop,          // 51
            sighand_default_nop,          // 52
            sighand_default_nop,          // 53
            sighand_default_nop,          // 54
            sighand_default_nop,          // 55
            sighand_default_nop,          // 56
            sighand_default_nop,          // 57
            sighand_default_nop,          // 58
            sighand_default_nop,          // 59
            sighand_default_nop,          // 60
            sighand_default_nop,          // 61
            sighand_default_nop,          // 62
            sighand_default_nop,          // 63 - (SIGRTMAX - 1)
        };
    } // namespace

    Task::~Task()
    {
        // Remove from process tasks
        if (process)
        {
            auto it = std::find(process->obj.tasks.begin(), process->obj.tasks.end(), this);
            if (it != process->obj.tasks.end())
            {
                process->obj.tasks.erase(it);
            }
        }

        // Cleanup resources, and send exit signal if necessary
        if (process->refcount <= 1)
        {
            scheduler.adopt_children(process->obj.pid);

            Process *parent = scheduler.get_process(process->obj.ppid);
            if (parent)
            {
                if (exit_signal & 0xFF)
                {
                    sys_siginfo siginfo;

                    siginfo.signo = exit_signal & 0xFF;
                    siginfo.errno_value = 0;
                    if ((exit_signal & 0xFF) == H_SIGCHLD)
                    {
                        if (is_wait_exited(exit_code))
                            siginfo.code = H_CLD_EXITED;
                        else if (is_wait_terminated(exit_code))
                            siginfo.code = H_CLD_KILLED;
                        else if (is_wait_terminated_coredump(exit_code))
                            siginfo.code = H_CLD_DUMPED;
                        else
                            siginfo.code = H_CLD_STOPPED;

                        siginfo.fields.child.pid = process->obj.pid;
                        siginfo.fields.child.uid = process->obj.euid;
                        siginfo.fields.child.status = exit_code;
                        siginfo.fields.child.utime = 0;
                        siginfo.fields.child.stime = 0;
                    }
                    else
                    {
                        siginfo.code = H_SI_USER;
                        siginfo.fields.kill.pid = process->obj.pid;
                        siginfo.fields.kill.uid = process->obj.euid;
                    }
                    parent->send_signal(siginfo);
                }

                ProcessStateChange state_change;

                if (is_wait_exited(exit_code))
                {
                    state_change.type = ProcessStateChangeType::EXIT;
                    state_change.exit_code = exit_code >> 8;
                }
                else
                {
                    state_change.type = ProcessStateChangeType::TERMINATE;
                    state_change.signal = exit_code & 0x7F;
                }

                parent->children_state_changes[get_pid()] = state_change;
            }

            // Clean up resources
            if (fd_table->refcount == 1)
            {
                for (size_t i = 0; i < fd_table->obj.fds.size(); ++i)
                    close(i);
                fd_table->obj.fds.clear();
            }
        }

        destroy_task_member(memory);
        destroy_task_member(program_brk);
        destroy_task_member(fd_table);
        destroy_task_member(process);
    }

    Task::Task(Task &&other)
        : Task()
    {
        if (this == &other)
            return;
        
        Task temp = std::move(other);
        other = std::move(*this);
        *this = std::move(temp);
    }

    Task &Task::operator=(Task &&other)
    {
        if (this == &other)
            return *this;
        
        std::swap(memory, other.memory);
        std::swap(program_brk, other.program_brk);
        std::swap(fd_table, other.fd_table);
        std::swap(process, other.process);
        std::swap(pending_signals, other.pending_signals);
        std::swap(emulator, other.emulator);
        std::swap(tid, other.tid);
        std::swap(ptid, other.ptid);
        std::swap(sig_mask, other.sig_mask);
        std::swap(blocking_operation, other.blocking_operation);
        std::swap(blocking_operation_saved, other.blocking_operation_saved);
        std::swap(exit_code, other.exit_code);
        std::swap(exit_signal, other.exit_signal);

        // bitfield, so we need to swap them manually
        bool tmp = is_paused;
        is_paused = other.is_paused;
        other.is_paused = tmp;
        tmp = is_dead;
        is_dead = other.is_dead;
        other.is_dead = tmp;

        return *this;
    }

    Process::~Process()
    {
        // Remove from process group
        if (pg)
        {
            auto it = std::find(pg->obj.processes.begin(), pg->obj.processes.end(), this);
            if (it != pg->obj.processes.end())
            {
                pg->obj.processes.erase(it);
            }
        }

        destroy_task_member(signal_handlers);
        destroy_task_member(fs_info);
        destroy_task_member(pg);
    }

    Process::Process(Process &&other)
        : Process()
    {
        if (this == &other)
            return;

        Process temp = std::move(other);
        other = std::move(*this);
        *this = std::move(temp);
    }

    Process &Process::operator=(Process &&other)
    {
        if (this == &other)
            return *this;

        std::swap(signal_handlers, other.signal_handlers);
        std::swap(fs_info, other.fs_info);
        std::swap(pg, other.pg);
        std::swap(tasks, other.tasks);
        std::swap(shared_pending_signals, other.shared_pending_signals);
        std::swap(children_state_changes, other.children_state_changes);
        std::swap(supplementary_gids, other.supplementary_gids);
        std::swap(pid, other.pid);
        std::swap(ppid, other.ppid);
        std::swap(uid, other.uid);
        std::swap(euid, other.euid);
        std::swap(suid, other.suid);
        std::swap(gid, other.gid);
        std::swap(egid, other.egid);
        std::swap(sgid, other.sgid);

        return *this;
    }

    int Process::send_signal(int signo, uint32_t uid, uint32_t pid)
    {
        sys_siginfo siginfo = {};
        siginfo.signo = signo;
        siginfo.errno_value = 0;
        siginfo.code = 0; // No specific code for this signal
        siginfo.fields.kill.uid = uid;
        siginfo.fields.kill.pid = pid;

        return send_signal(siginfo);
    }

    int Process::send_signal(const sys_siginfo &siginfo)
    {
        if (siginfo.signo < 0 || siginfo.signo >= 64)
        {
            error = EINVAL; // Invalid signal number
            return -1;
        }

        if (siginfo.signo >= H_SIGRTMIN && siginfo.signo <= H_SIGRTMAX)
        {
            shared_pending_signals.rt_sigqueue.push_back({siginfo});
        }
        else
        {
            auto it = shared_pending_signals.normal_signals.find(siginfo.signo);
            if (it != shared_pending_signals.normal_signals.end())
            {
                // Signal is already pending, do nothing
            }
            else
            {
                shared_pending_signals.normal_signals[siginfo.signo] = {siginfo};
            }
        }

        return 0;
    }

    int Process::set_default_signal_handlers()
    {
        memcpy(signal_handlers->obj.sig_handlers, default_signal_handlers, sizeof(default_signal_handlers));

        return 0;
    }

    int Process::set_signal_handler(int signo, SignalHandler handler)
    {
        if (signo < 0 || signo >= 64)
        {
            error = EINVAL; // Invalid signal number
            return -1;
        }

        // Check for unblockable signals
        if (signo == H_SIGKILL || signo == H_SIGSTOP || signo == H_SIGCONT)
        {
            error = EPERM; // Cannot set handler for unblockable signals
            return -1;
        }

        signal_handlers->obj.sig_handlers[signo] = handler;
        return 0;
    }

    int Process::ignore_signal(int signo)
    {
        if (signo < 0 || signo >= 64)
        {
            error = EINVAL; // Invalid signal number
            return -1;
        }

        // Set the handler to a no-op
        signal_handlers->obj.sig_handlers[signo] = sighand_default_nop;
        return 0;
    }

    int Process::default_signal(int signo)
    {
        if (signo < 0 || signo >= 64)
        {
            error = EINVAL; // Invalid signal number
            return -1;
        }

        // Set the handler to the default handler
        signal_handlers->obj.sig_handlers[signo] = default_signal_handlers[signo];
        return 0;
    }

    int Process::join_process_group(TaskMember<ProcessGroup> *pg)
    {
        // Leave current process group if any
        if (this->pg)
        {
            auto it = std::find(this->pg->obj.processes.begin(), this->pg->obj.processes.end(), this);
            if (it != this->pg->obj.processes.end())
            {
                this->pg->obj.processes.erase(it);
            }
            destroy_task_member(this->pg);
            this->pg = nullptr;
        }

        if (pg)
        {
            // Join new process group
            this->pg = ref_task_member(pg);
            pg->obj.processes.push_back(this);
        }

        return 0;
    }

    ProcessGroup::~ProcessGroup()
    {
        // Remove from session
        if (session)
        {
            auto it = std::find(session->obj.pgroups.begin(), session->obj.pgroups.end(), this);
            if (it != session->obj.pgroups.end())
            {
                session->obj.pgroups.erase(it);
            }
        }

        destroy_task_member(session);
    }

    ProcessGroup::ProcessGroup(ProcessGroup &&other)
        : ProcessGroup()
    {
        if (this == &other)
            return;
        std::swap(session, other.session);
        std::swap(processes, other.processes);
        std::swap(pgid, other.pgid);
    }

    ProcessGroup &ProcessGroup::operator=(ProcessGroup &&other)
    {
        if (this == &other)
            return *this;

        std::swap(session, other.session);
        std::swap(processes, other.processes);
        std::swap(pgid, other.pgid);

        return *this;
    }

    uint32_t ProcessGroup::get_sid()
    {
        if (session)
            return session->obj.sid;
        return 0;
    }

    uint32_t Process::get_sid()
    {
        if (pg)
            return pg->obj.get_sid();
        return 0;
    }

    uint32_t Process::get_pgid()
    {
        if (pg)
            return pg->obj.pgid;
        return 0;
    }

    uint32_t Task::get_sid()
    {
        assert(process);
        return process->obj.get_sid();
    }

    uint32_t Task::get_pgid()
    {
        assert(process);
        return process->obj.get_pgid();
    }

    uint32_t Task::get_pid()
    {
        assert(process);
        return process->obj.pid;
    }

    int Task::poll_block()
    {
        if (blocking_operation)
            blocking_operation(*this);
        return 0;
    }

    int Task::get_vfs_fd(int fd)
    {
        assert(fd_table);
        if (fd < 0 || fd >= (int)fd_table->obj.fds.size())
        {
            error = EBADF; // Invalid file descriptor
            return -1;
        }

        const UserFD &user_fd = fd_table->obj.fds[fd];

        if (user_fd.type != UserFDType::VFS)
        {
            error = EBADF; // Not a VFS file descriptor
            return -1;
        }

        if (user_fd.vfs_fd < 0)
        {
            error = EBADF; // Closed or invalid file descriptor
            return -1;
        }

        return user_fd.vfs_fd;
    }

    size_t Task::get_unused_fd_index(int start)
    {
        assert(fd_table);
        for (size_t i = start; i < fd_table->obj.fds.size(); ++i)
        {
            auto &entry = fd_table->obj.fds[i];
            if (entry.type == UserFDType::VFS && entry.vfs_fd < 0)
            {
                return i; // Found an unused VFS file descriptor
            }
        }

        // If no unused fd found, create a new one
        fd_table->obj.fds.push_back(UserFD{.flags = 0, .type = UserFDType::VFS, .vfs_fd = -1});
        return fd_table->obj.fds.size() - 1; // Return the index of the new fd
    }

    UserFD *Task::get_user_fd(int fd)
    {
        assert(fd_table);
        if (fd < 0 || fd >= (int)fd_table->obj.fds.size())
        {
            error = EBADF; // Invalid file descriptor
            return nullptr;
        }

        return &fd_table->obj.fds[fd];
    }

    Task *Task::clone(uint32_t clone_flags)
    {
        Task *new_task = alloc<Task>();

        new_task->ptid = tid; // Set the parent thread ID
        new_task->tid = 0;
        new_task->sig_mask = sig_mask; // Copy the signal mask
        new_task->blocking_operation = nullptr;
        new_task->exit_code = 0;
        new_task->exit_signal = exit_signal;
        new_task->is_paused = is_paused;
        new_task->is_dead = is_dead;
        new_task->last_tick = last_tick;

        new_task->emulator = emulator;

        new_task->memory = clone_flags & H_CLONE_VM ? ref_task_member(memory) : copy_task_member(memory);
        new_task->program_brk = clone_flags & H_CLONE_VM ? ref_task_member(program_brk) : copy_task_member(program_brk);
        new_task->emulator.memory = &new_task->memory->obj;
        new_task->fd_table = clone_flags & H_CLONE_FILES ? ref_task_member(fd_table) : copy_task_member(fd_table);
        if (!(clone_flags & H_CLONE_FILES))
        {
            for (UserFD &file : new_task->fd_table->obj.fds)
            {
                switch (file.type)
                {
                case UserFDType::VFS:
                    ++fd_refcount[file.vfs_fd];
                    break;
                case UserFDType::PIPE_READ:
                    ++file.pipe->readers;
                    break;
                case UserFDType::PIPE_WRITE:
                    ++file.pipe->writers;
                    break;
                default:
                    break;
                }
            }
        }

        if (clone_flags & H_CLONE_THREAD)
        {
            new_task->process = ref_task_member(process);
        }
        else
        {
            new_task->process = make_task_member<Process>();
            Process &proc = new_task->process->obj;
            proc.pg = ref_task_member(process->obj.pg);
            proc.pg->obj.processes.push_back(&proc);
            proc.signal_handlers = clone_flags & H_CLONE_SIGHAND ? ref_task_member(process->obj.signal_handlers) : copy_task_member(process->obj.signal_handlers);
            proc.fs_info = clone_flags & H_CLONE_FS ? ref_task_member(process->obj.fs_info) : copy_task_member(process->obj.fs_info);
            proc.ppid = clone_flags & H_CLONE_PARENT ? process->obj.ppid : process->obj.pid;
            proc.pid = 0;
            proc.tasks.push_back(new_task);

            proc.supplementary_gids = process->obj.supplementary_gids;
            proc.uid = process->obj.uid;
            proc.euid = process->obj.euid;
            proc.suid = process->obj.suid;
            proc.gid = process->obj.gid;
            proc.egid = process->obj.egid;
            proc.sgid = process->obj.sgid;
        }

        return new_task;
    }

    int Task::init_tid(int new_id)
    {
        if (tid == 0)
            tid = new_id;

        Process &proc = process->obj;

        // clone() sets these fields to 0

        if (proc.pid == 0)
            proc.pid = new_id;
        
        return 0;
    }

    int Task::exit(uint16_t exit_code)
    {
        is_dead = true;
        this->exit_code = exit_code;

        return 0;
    }

    int Task::send_signal(int signo, uint32_t uid, uint32_t pid)
    {
        sys_siginfo siginfo;
        siginfo.signo = signo;
        siginfo.errno_value = 0;
        siginfo.code = H_SI_USER;
        siginfo.fields.kill.pid = pid;
        siginfo.fields.kill.uid = uid;

        return send_signal(siginfo);
    }

    int Task::get_relative_fd(const char *path, int thread_at_fd)
    {
        if (!path || path[0] == '\0')
        {
            error = EINVAL; // Invalid path
            return -1;
        }

        if (path[0] == '/')
        {
            // Absolute path
            const char *root_path = process->obj.fs_info->obj.root_path.c_str();

            return vfs.open(root_path, OPEN_RDWR | OPEN_DIRECTORY);
        }
        else if (thread_at_fd == -100)
        {
            const char *cwd_path = process->obj.fs_info->obj.cwd_path.c_str();
            return vfs.open(cwd_path, OPEN_RDWR | OPEN_DIRECTORY);
        }
        else
        {
            if (thread_at_fd < 0 || thread_at_fd >= (int)fd_table->obj.fds.size())
            {
                error = EBADF; // Invalid file descriptor
                return -1;
            }

            const UserFD &user_fd = fd_table->obj.fds[thread_at_fd];

            if (user_fd.type != UserFDType::VFS || user_fd.vfs_fd < 0)
            {
                error = EBADF; // Not a valid VFS file descriptor
                return -1;
            }

            return vfs.dup(user_fd.vfs_fd);
        }

        // Should not reach here
        return -1;
    }

    char *Task::process_user_path(char *user_path)
    {
        if (!user_path || user_path[0] == '\0')
        {
            dealloc(user_path);
            error = EINVAL; // Invalid path
            return nullptr;
        }

        size_t len = strlen(user_path);
        String *root;
        if (user_path[0] == '/')
        {
            root = &process->obj.fs_info->obj.root_path;
        }
        else
        {
            root = &process->obj.fs_info->obj.cwd_path;
        }

        len += 1 + root->length(); // +1 for the '/' separator

        char *new_path = alloc<char>(len + 1); // +1 for null terminator

        strcpy(new_path, root->c_str());
        strcat(new_path, "/");
        strcat(new_path, user_path);

        dealloc(user_path); // Free the original path
        return new_path; // Return the new absolute path
    }

    int Task::send_signal(const sys_siginfo &siginfo)
    {
        if (siginfo.signo < 1 || siginfo.signo >= 64)
        {
            error = EINVAL; // Invalid signal number
            return -1;
        }

        if (siginfo.signo >= H_SIGRTMIN && siginfo.signo < H_SIGRTMAX)
        {
            pending_signals.rt_sigqueue.push_back({siginfo});
        }
        else
        {
            auto it = pending_signals.normal_signals.find(siginfo.signo);
            if (it != pending_signals.normal_signals.end())
            {
                // Signal is already pending, do nothing
            }
            else
            {
                pending_signals.normal_signals[siginfo.signo] = {siginfo};
            }
        }

        return 0;
    }

    int Task::close(int fd)
    {
        UserFD *p_user_fd = get_user_fd(fd);
        if (!p_user_fd)
            return -1;
        UserFD &user_fd = *p_user_fd;

        switch (user_fd.type)
        {
        case UserFDType::VFS:
        {
            // Close the VFS file descriptor
            int vfs_fd = user_fd.vfs_fd;

            user_fd.vfs_fd = -1; // Reset the VFS file descriptor
            
            fd_refcount[vfs_fd]--;
            if (fd_refcount[vfs_fd] == 0)
            {
                fd_refcount.erase(vfs_fd);

                return vfs.close(user_fd.vfs_fd);
            }
            break;
        }
        case UserFDType::PID:
            // Mark as closed

            user_fd.type = UserFDType::VFS;
            user_fd.vfs_fd = -1; // Reset the VFS file descriptor
            break;
        case UserFDType::PIPE_READ:
        case UserFDType::PIPE_WRITE:
            if (user_fd.type == UserFDType::PIPE_READ)
                --user_fd.pipe->readers;
            else
                --user_fd.pipe->writers;
            
            if (user_fd.pipe->is_destroyable())
                dealloc(user_fd.pipe);
            user_fd.type = UserFDType::VFS;
            user_fd.vfs_fd = -1;
        }

        return 0;
    }

    int Task::is_signal_blocked(int signo)
    {
        if (signo < 1 || signo >= 64)
        {
            error = EINVAL; // Invalid signal number
            return -1;
        }

        // Check if the signal is blocked
        if ((sig_mask & (1U << signo)) == 0)
        {
            return 1; // Signal is blocked
        }

        return 0; // Signal is not blocked
    }

    int Task::is_signal_ignored(int signo)
    {
        if (signo < 1 || signo >= 64)
        {
            error = EINVAL; // Invalid signal number
            return -1;
        }

        // Check if the signal is ignored
        if (process->obj.signal_handlers->obj.sig_handlers[signo].fn == sighand_nop)
        {
            return 1; // Signal is ignored
        }

        return 0; // Signal is not ignored
    }

    MemorySpace &Task::get_memory()
    {
        return memory->obj.memory;
    }
} // namespace Hamster
