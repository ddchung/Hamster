
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

        void sighand_nop(Task *, sys_siginfo *, void *)
        {
            Task *current_task = scheduler.get_current_task();
            if (current_task)
                _trace("TID %d received signal %d, doing nothing\n", current_task->tid, current_task->emulator.x[17]);
        }

        void sighand_term(Task *task, sys_siginfo *siginfo, void *)
        {
            assert(task);
            assert(siginfo);
            task->exit(make_wait_terminated(siginfo->signo));

            Task *current_task = scheduler.get_current_task();
            if (current_task)
                _trace("TID %d received signal %d, terminating\n", current_task->tid, siginfo->signo);
        }

        void sighand_dump(Task *task, sys_siginfo *siginfo, void *)
        {
            assert(task);
            assert(siginfo);
            task->exit(make_wait_terminated_coredump(siginfo->signo));

            Task *current_task = scheduler.get_current_task();
            if (current_task)
                _trace("TID %d received signal %d, terminating with core dump at PC 0x%08x\n", current_task->tid, siginfo->signo, current_task->emulator.pc);
        }

        void sighand_stop(Task *task, sys_siginfo * siginfo, void *)
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

        void sighand_cont(Task *task, sys_siginfo *siginfo, void *)
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

        SignalHandler default_signal_handlers[32] = {
            {nullptr, nullptr},              // 0 — not used
            {sighand_term, nullptr},         // 1 — SIGHUP (terminate)
            {sighand_term, nullptr},         // 2 — SIGINT (terminate)
            {sighand_term, nullptr},         // 3 — SIGQUIT (terminate + core)
            {sighand_dump, nullptr},         // 4 — SIGILL (terminate + core)
            {sighand_dump, nullptr},         // 5 — SIGTRAP (terminate + core)
            {sighand_dump, nullptr},         // 6 — SIGABRT/SIGIOT (terminate + core)
            {sighand_dump, nullptr},         // 7 — SIGBUS (terminate + core)
            {sighand_dump, nullptr},         // 8 — SIGFPE (terminate + core)
            {sighand_term, nullptr},         // 9 — SIGKILL (terminate, cannot catch/ignore)
            {sighand_term, nullptr},         // 10 — SIGUSR1 (terminate)
            {sighand_dump, nullptr},         // 11 — SIGSEGV (terminate + core)
            {sighand_term, nullptr},         // 12 — SIGUSR2 (terminate)
            {sighand_term, nullptr},         // 13 — SIGPIPE (terminate)
            {sighand_term, nullptr},         // 14 — SIGALRM (terminate)
            {sighand_term, nullptr},         // 15 — SIGTERM (terminate)
            {sighand_term, nullptr},         // 16 — SIGSTKFLT (terminate)
            {sighand_nop, nullptr},          // 17 — SIGCHLD (ignore)
            {sighand_cont, nullptr},         // 18 — SIGCONT (continue, if stopped)
            {sighand_stop, nullptr},         // 19 — SIGSTOP (stop, cannot catch/ignore)
            {sighand_stop, nullptr},         // 20 — SIGTSTP (stop)
            {sighand_stop, nullptr},         // 21 — SIGTTIN (stop)
            {sighand_stop, nullptr},         // 22 — SIGTTOU (stop)
            {sighand_nop, nullptr},          // 23 — SIGURG (ignore)
            {sighand_dump, nullptr},         // 24 — SIGXCPU (terminate + core)
            {sighand_dump, nullptr},         // 25 — SIGXFSZ (terminate + core)
            {sighand_term, nullptr},         // 26 — SIGVTALRM (terminate)
            {sighand_term, nullptr},         // 27 — SIGPROF (terminate)
            {sighand_nop, nullptr},          // 28 — SIGWINCH (ignore)
            {sighand_nop, nullptr},          // 29 — SIGIO/SIGPOLL (ignore)
            {sighand_term, nullptr},         // 30 — SIGPWR (terminate)
            {sighand_dump, nullptr},         // 31 — SIGSYS (terminate + core)
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
                        siginfo.fields.child.uid = process->obj.uid;
                        siginfo.fields.child.status = exit_code;
                        siginfo.fields.child.utime = 0;
                        siginfo.fields.child.stime = 0;
                    }
                    else
                    {
                        siginfo.code = H_SI_USER;
                        siginfo.fields.kill.pid = process->obj.pid;
                        siginfo.fields.kill.uid = process->obj.uid;
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
                for (auto &fd : fd_table->obj.fds)
                {
                    if (fd.type == UserFDType::VFS && fd.vfs_fd >= 0)
                    {
                        fd_refcount[fd.vfs_fd]--;
                        if (fd_refcount[fd.vfs_fd] == 0)
                        {
                            vfs.close(fd.vfs_fd);
                            fd_refcount.erase(fd.vfs_fd);
                        }
                        fd.vfs_fd = -1; // Mark as closed
                    }
                }
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
        std::swap(sig_queue, other.sig_queue);
        std::swap(emulator, other.emulator);
        std::swap(tid, other.tid);
        std::swap(ptid, other.ptid);
        std::swap(sig_mask, other.sig_mask);
        std::swap(io_block_fd, other.io_block_fd);
        std::swap(blocking_operation, other.blocking_operation);
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
        std::swap(shared_sig_queue, other.shared_sig_queue);
        std::swap(children_state_changes, other.children_state_changes);
        std::swap(pid, other.pid);
        std::swap(ppid, other.ppid);
        std::swap(uid, other.uid);
        std::swap(euid, other.euid);
        std::swap(gid, other.gid);
        std::swap(egid, other.egid);

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
        shared_sig_queue.push_back(siginfo);
        return 0;
    }

    int Process::set_default_signal_handlers()
    {
        memcpy(signal_handlers->obj.sig_handlers, default_signal_handlers, sizeof(default_signal_handlers));

        return 0;
    }

    int Process::set_signal_handler(int signo, SignalHandler handler)
    {
        if (signo < 0 || signo >= 32)
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
        switch (blocking_operation)
        {
            case BlockingOperation::IO_READ:
                return poll_read();
            case BlockingOperation::IO_WRITE:
                return poll_write();
            case BlockingOperation::WAIT:
                return poll_wait();
            default:
                return 0; // No blocking operation
        }
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

    Task *Task::clone(uint32_t clone_flags)
    {
        Task *new_task = alloc<Task>();

        new_task->ptid = tid; // Set the parent thread ID
        new_task->tid = 0;
        new_task->sig_mask = sig_mask; // Copy the signal mask
        new_task->io_block_fd = -1;
        new_task->blocking_operation = BlockingOperation::NONE;
        new_task->exit_code = 0;
        new_task->exit_signal = exit_signal;
        new_task->is_paused = is_paused;
        new_task->is_dead = is_dead;

        new_task->emulator = emulator;

        new_task->memory = clone_flags & H_CLONE_VM ? ref_task_member(memory) : copy_task_member(memory);
        new_task->program_brk = clone_flags & H_CLONE_VM ? ref_task_member(program_brk) : copy_task_member(program_brk);
        new_task->emulator.memory = &new_task->memory->obj;
        new_task->fd_table = clone_flags & H_CLONE_FILES ? ref_task_member(fd_table) : copy_task_member(fd_table);
        if (!(clone_flags & H_CLONE_FILES))
        {
            for (UserFD &file : new_task->fd_table->obj.fds)
            {
                if (file.type == UserFDType::VFS && file.vfs_fd >= 0)
                {
                    fd_refcount[file.vfs_fd]++;
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
            proc.signal_handlers = clone_flags & H_CLONE_SIGHAND ? ref_task_member(process->obj.signal_handlers) : copy_task_member(process->obj.signal_handlers);
            proc.fs_info = clone_flags & H_CLONE_FS ? ref_task_member(process->obj.fs_info) : copy_task_member(process->obj.fs_info);
            proc.ppid = clone_flags & H_CLONE_PARENT ? process->obj.ppid : process->obj.pid;
            proc.pid = 0;
            proc.tasks.push_back(new_task);

            proc.uid = process->obj.uid;
            proc.euid = process->obj.euid;
            proc.gid = process->obj.gid;
            proc.egid = process->obj.egid;
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

    int Task::send_signal(const sys_siginfo &siginfo)
    {
        sig_queue.push_back(siginfo);
        return 0;
    }
} // namespace Hamster
