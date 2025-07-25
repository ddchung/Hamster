
#include <process/task.hpp>
#include <process/scheduler.hpp>
#include <errno/errno.h>
#include <memory/allocator.hpp>
#include <abi/values.hpp>
#include <cassert>
#include <utility>

namespace Hamster
{
    Task::~Task()
    {
        destroy_task_member(memory);
        destroy_task_member(fd_table);
        destroy_task_member(filesystem);
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
        std::swap(fd_table, other.fd_table);
        std::swap(filesystem, other.filesystem);
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
        std::swap(zombies, other.zombies);
        std::swap(pid, other.pid);
        std::swap(ppid, other.ppid);
        std::swap(uid, other.uid);
        std::swap(euid, other.euid);
        std::swap(gid, other.gid);
        std::swap(egid, other.egid);

        return *this;
    }

    ProcessGroup::~ProcessGroup()
    {
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

        new_task->memory = make_task_member(memory, !(clone_flags & H_CLONE_VM));
        new_task->filesystem = make_task_member(filesystem, false);
        new_task->fd_table = make_task_member(fd_table, !(clone_flags & H_CLONE_FILES));
        if (!(clone_flags & H_CLONE_FILES))
        {
            for (UserFD &file : new_task->fd_table->obj.fds)
            {
                if (file.type == UserFDType::VFS && file.vfs_fd >= 0)
                {
                    filesystem->obj.fd_refcount[file.vfs_fd]++;
                }
            }
        }

        if (clone_flags & H_CLONE_THREAD)
        {
            new_task->process = make_task_member(process, false);
        }
        else
        {
            new_task->process = make_task_member<Process>();
            Process &proc = new_task->process->obj;
            proc.pg = make_task_member(process->obj.pg);
            proc.signal_handlers = make_task_member(process->obj.signal_handlers, !(clone_flags & H_CLONE_SIGHAND));
            proc.fs_info = make_task_member(process->obj.fs_info, !(clone_flags & H_CLONE_FS));
            proc.ppid = clone_flags & H_CLONE_PARENT ? process->obj.ppid : process->obj.pid;
            proc.pid = 0;
            proc.tasks.push_back(0);

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
        
        if (proc.tasks.empty())
            proc.tasks.push_back(new_id);
        else if (proc.tasks.size() == 1 && proc.tasks[0] == 0)
            proc.tasks[0] = new_id;
        
        return 0;
    }

    int Task::exit(uint16_t exit_code)
    {
        is_dead = true;
        this->exit_code = exit_code;

        if (process->refcount > 1)
            return 0; // We are done here, as there are still other threads running

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
                    siginfo.code = H_CLD_EXITED;
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
        }

        // Clean up resources
        if (fd_table->refcount == 1)
        {
            for (auto &fd : fd_table->obj.fds)
            {
                if (fd.type == UserFDType::VFS && fd.vfs_fd >= 0)
                {
                    filesystem->obj.fd_refcount[fd.vfs_fd]--;
                    if (filesystem->obj.fd_refcount[fd.vfs_fd] == 0)
                    {
                        filesystem->obj.vfs.close(fd.vfs_fd);
                        filesystem->obj.fd_refcount.erase(fd.vfs_fd);
                    }
                    fd.vfs_fd = -1; // Mark as closed
                }
            }
            fd_table->obj.fds.clear();
        }
    }
} // namespace Hamster
