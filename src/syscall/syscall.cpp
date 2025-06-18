// Hamster syscalls

#include <syscall/syscall.hpp>
#include <syscall/syscall_id.hpp>
#include <process/thread.hpp>
#include <process/process.hpp>
#include <process/scheduler.hpp>
#include <errno/errno.h>
#include <filesystem/vfs.hpp>
#include <memory/allocator.hpp>
#include <memory/stl_sequential.hpp>
#include <csignal>

// debugging
#include <cstdio>

namespace Hamster
{
    namespace
    {
        uint32_t get_syscall_num(Thread &thread)
        {
            // The syscall number is in a7 register
            return thread.get_regs()[17]; // a7 is x17
        }

        uint32_t *get_syscall_args(Thread &thread)
        {
            // The syscall arguments are in a0-a6 registers
            return thread.get_regs() + 10;
        }

        int deref_fildes(int fd, Process *process)
        {
            // Check if the fd is valid
            if (fd < 0 || (size_t)fd >= process->fds.size())
                return -1;
            // Return the fd from the process's file descriptor table
            return process->fds[fd].fd;
        }

        void set_return_code(Thread &thread, uint32_t ret)
        {
            // Set the return code in a0 register
            thread.get_regs()[10] = ret; // a0 is x10
        }

        // Resolves a path relative to the current working directory, if it
        // is relative. Otherwise, it is untouched
        String resolve_path(const String &cwd, const char *path)
        {
            if (path[0] == '/')
            {
                // Absolute path, return as is
                return String(path);
            }
            else
            {
                // Relative path, prepend the current working directory
                return cwd + '/' + String(path);
            }
        }

        // Buffer for I/O operations
        uint8_t io_buf[512]{0};
    } // namespace

    int do_syscall(Thread &thread)
    {
        Process *process = thread.get_process();
        uint32_t *args = get_syscall_args(thread);

        int fd;
        switch (get_syscall_num(thread))
        {
        case SyscallID::EXIT:
            printf("Process %u exiting with code %d\n", process->pid, args[0]);
            process->exit_code = args[0];
            for (auto &t : process->threads)
            {
                t.set_state(ThreadState::ENDED);
            }
            return 0;
        case SyscallID::CLOSE:
            fd = deref_fildes(args[0], process);
            if (fd < 0)
            {
                thread.set_error_code(EBADF);
                set_return_code(thread, -1);
                return -1; // Invalid file descriptor
            }
            if (vfs.close(fd) != 0)
            {
                thread.set_error_code(error);
                error = 0;
                set_return_code(thread, -1);
                return -1;
            }
            process->fds[args[0]].fd = -1; // Mark the fd as closed
            set_return_code(thread, 0);    // Return 0 on success
            return 0;
        case SyscallID::EXECVE:
        {
            char *path = process->memory_space.get_string(args[0]);
            if (!path)
            {
                thread.set_error_code(EIO);
                error = 0;
                set_return_code(thread, -1);
                return -1;
            }
            String resolved_path = resolve_path(process->cwd, path);
            dealloc(path);
            if (process->load_elf(resolved_path.c_str()) < 0)
            {
                thread.set_error_code(error);
                error = 0;
                set_return_code(thread, -1);
                return -1;
            }
            // Close all FD's marked CLOEXEC
            for (auto &[fd, flags] : process->fds)
            {
                if (flags & FD_CLOEXEC)
                {
                    if (vfs.close(fd) != 0)
                    {
                        thread.set_error_code(error);
                        error = 0;
                        set_return_code(thread, -1);
                        return -1;
                    }
                    fd = -1;
                }
            }
            return 0;
        }
        case SyscallID::FORK:
        {
            Process *new_process = alloc<Process>(1, *process);
            new_process->ppid = process->pid;  // Set the parent PID
            new_process->pgid = process->pgid; // Set the process group ID
            new_process->sid = process->sid;   // Set the session ID
            new_process->uid = process->uid;   // Set the user ID
            new_process->gid = process->gid;   // Set the group ID
            new_process->euid = process->euid; // Set the effective user ID
            new_process->egid = process->egid; // Set the effective group ID
            new_process->cwd = process->cwd;   // Copy the current working directory
            if (scheduler.add_process(new_process) < 0)
            {
                thread.set_error_code(ENOMEM);
                dealloc(new_process);
                set_return_code(thread, -1);
                return -1; // Failed to add process
            }

            // Set the return code of the old process to the PID, and the new process to 0
            set_return_code(thread, new_process->pid); // Return the new process ID
            // Return 0 for the new process
            auto it = new_process->threads.begin();
            assert(it != new_process->threads.end());
            std::advance(it, thread.get_id());
            set_return_code(*it, 0); // Return 0 for the new process thread

            // done
            return 0;
        }
        case SyscallID::FSTAT:
        {
            fd = deref_fildes(args[0], process);
            if (fd < 0)
            {
                thread.set_error_code(EBADF);
                set_return_code(thread, -1);
                return -1;
            }
            if (vfs.stat(fd, reinterpret_cast<struct stat *>(io_buf)) != 0)
            {
                thread.set_error_code(error);
                error = 0;
                set_return_code(thread, -1);
                return -1;
            }
            // Reformat the stat structure to match the Sys_stat ABI
            struct stat *stat_buf = reinterpret_cast<struct stat *>(io_buf);
            Sys_stat sys_stat_buf;
            sys_stat_buf.dev = stat_buf->st_dev;
            sys_stat_buf.ino = stat_buf->st_ino;
            sys_stat_buf.mode = stat_buf->st_mode;
            sys_stat_buf.nlink = stat_buf->st_nlink;
            sys_stat_buf.uid = stat_buf->st_uid;
            sys_stat_buf.gid = stat_buf->st_gid;
            sys_stat_buf.rdev = stat_buf->st_rdev;
            sys_stat_buf.size = stat_buf->st_size;
            sys_stat_buf.atime = stat_buf->st_atime;
            sys_stat_buf.mtime = stat_buf->st_mtime;
            sys_stat_buf.ctime = stat_buf->st_ctime;
            sys_stat_buf.blksize = stat_buf->st_blksize;
            sys_stat_buf.blocks = stat_buf->st_blocks;
            // Copy the Sys_stat structure to user space
            if (process->memory_space.memcpy(args[1], &sys_stat_buf, sizeof(Sys_stat)) != 0)
            {
                thread.set_error_code(EIO);
                error = 0;
                set_return_code(thread, -1);
                return -1;
            }
            set_return_code(thread, 0); // Return 0 on success
            return 0;
        }
        case SyscallID::GETPID:
            // Return the process ID
            set_return_code(thread, process->pid);
            return 0;
        case SyscallID::ISATTY:
        {
            fd = deref_fildes(args[0], process);
            if (fd < 0)
            {
                thread.set_error_code(EBADF);
                set_return_code(thread, -1);
                return -1;
            }
            int is_tty = vfs.isatty(fd);
            if (is_tty < 0)
            {
                thread.set_error_code(error);
                error = 0;

                // Error checking if it's a TTY, but
                // POSIX mandates that we return 0 and set `errno`,
                // rather than returning -1 and setting `errno`
                set_return_code(thread, 0);
                return -1;
            }
            set_return_code(thread, is_tty);
            return 0;
        }
        case SyscallID::KILL:
        {
            uint32_t pid = args[0];
            int signal = args[1];

            Process *target = scheduler.get_process(pid);
            if (!target)
            {
                thread.set_error_code(ESRCH); // No such process
                set_return_code(thread, -1);
                return -1;
            }

            // Signal to the first listening thread
            for (auto &t : target->threads)
            {
                if (t.get_state() != ThreadState::ENDED && (t.get_signal_mask() & (1 << signal)) == 0)
                {
                    t.signal(signal);
                    set_return_code(thread, 0); // Success
                    return 0;
                }
            }

            // No threads listening, signal to thread 0
            if (!target->threads.empty())
            {
                target->threads.begin()->signal(signal);
                set_return_code(thread, 0); // Success
                return 0;
            }
            // No threads at all, return ESRCH
            thread.set_error_code(ESRCH);
            set_return_code(thread, -1);
            return -1; // No threads to signal
        }
        case SyscallID::LINK:
        {
            // TODO: Filesystem hard links
            thread.set_error_code(ENOSYS);
            set_return_code(thread, -1);
            return -1; // Not implemented yet
        }
        case SyscallID::READ:
        {
            fd = deref_fildes(args[0], process);
            if (fd < 0)
            {
                thread.set_error_code(EBADF);
                set_return_code(thread, -1);
                return -1;
            }
            uint32_t buf_addr = args[1];
            uint32_t count = args[2];

            // Read sizeof(io_buf) bytes at a time
            size_t total_read = 0;
            while (total_read < count)
            {
                size_t to_read = std::min(count - (uint32_t)total_read, (uint32_t)sizeof(io_buf));
                ssize_t read_bytes = vfs.read(fd, io_buf, to_read);
                if (read_bytes < 0)
                {
                    thread.set_error_code(error);
                    error = 0;
                    set_return_code(thread, -1); // Error reading
                    return -1;
                }
                if (read_bytes == 0)
                    break; // EOF reached

                // Copy the read bytes to the user space
                if (process->memory_space.memcpy(buf_addr + total_read, io_buf, read_bytes) != 0)
                {
                    // Error copying to user space

                    // Set to EINTR instead of signaling SIGSEGV
                    thread.set_error_code(EINTR);
                    error = 0;
                    set_return_code(thread, -1);
                    return -1;
                }
                total_read += read_bytes;
            }
            set_return_code(thread, total_read); // Return the number of bytes read
            return total_read;                   // Return the number of bytes read
        }
        case SyscallID::LSEEK:
        {
            fd = deref_fildes(args[0], process);
            if (fd < 0)
            {
                thread.set_error_code(EBADF);
                error = 0;
                set_return_code(thread, -1);
                return -1;
            }
            int32_t offset = args[1];
            int whence = args[2];
            int ok = vfs.seek(fd, offset, whence);
            if (ok < 0)
            {
                thread.set_error_code(error);
                error = 0;
                set_return_code(thread, -1);
                return -1;
            }
            // Return the new file offset
            set_return_code(thread, vfs.tell(fd));
            return 0;
        }
        case SyscallID::OPEN:
        {
            char *path = process->memory_space.get_string(args[0]);
            if (!path)
            {
                error = 0;
                thread.set_error_code(EINTR);
                return -1;
            }
            String resolved_path = resolve_path(process->cwd, path);
            dealloc(path);
            int flags = map_sys_to_posix_flags(args[1]);
            int mode = args[2];
            int fd = vfs.open(resolved_path.c_str(), flags, mode);
            if (fd < 0)
            {
                thread.set_error_code(error);
                error = 0;
                set_return_code(thread, -1);
                return -1;
            }
            // Place in first available slot in fds
            for (size_t i = 0; i < process->fds.size(); ++i)
            {
                if (process->fds[i].fd == -1)
                {
                    process->fds[i].fd = fd;
                    process->fds[i].fd_flags = 0;
                    set_return_code(thread, i); // Return the file descriptor index
                    return 0;
                }
            }
            // no unused fd's, make new
            process->fds.push_back({fd, 0});
            set_return_code(thread, process->fds.size() - 1); // Return the new file descriptor index
            return 0;                                         // Success
        }
        case SyscallID::TIMES:
        {
            // TODO: Time
            thread.set_error_code(ENOSYS);
            set_return_code(thread, -1);
            return -1; // Not implemented yet
        }
        case SyscallID::UNLINK:
        {
            char *path = process->memory_space.get_string(args[0]);
            if (!path)
            {
                thread.set_error_code(EIO);
                error = 0;
                set_return_code(thread, -1);
                return -1;
            }
            String resolved_path = resolve_path(process->cwd, path);
            dealloc(path);
            int ret = vfs.remove(resolved_path.c_str());
            if (ret < 0)
            {
                thread.set_error_code(error);
                error = 0;
                set_return_code(thread, -1);
                return -1;
            }
            set_return_code(thread, 0); // Return 0 on success
            return 0;                   // Success
        }
        case SyscallID::WAIT:
        {
            uint32_t pid = process->pid;
            uint32_t stat_loc = args[0];
            // Block until a child process exits
            thread.pause([pid, stat_loc](Thread &t)
                         {
                // Check if a child process has exited
                for (Process *child : scheduler.get_processes())
                {
                    if (child->ppid != pid)
                        continue; // Not a child process
                    int status = 0;
                    bool running = false;
                    for (Thread &child_thread : child->threads)
                    {
                        // Check for signals
                        if (child_thread.get_pending_signal() &&
                            (1 <<( (child_thread.get_pending_signal() - 1))) &
                            (child_thread.get_signal_mask() & ~SIGKILL & ~SIGSTOP))
                        {
                            status |= child_thread.get_pending_signal() & 0xFF;
                        }
                        if (child_thread.get_state() != ThreadState::ENDED)
                        {
                            running = true; // Child process is still running
                        }
                    }
                    if (!running)
                    {
                        status |= child->exit_code << 8; // Set exit code
                    }

                    if (status != 0)
                    {
                        set_return_code(t, child->pid);
                        t.get_process()->memory_space.memcpy(stat_loc, &status, sizeof(status));
                        t.resume();
                    }
                } });
            return 0;
        }
        case SyscallID::WRITE:
        {
            fd = deref_fildes(args[0], process);
            if (fd < 0)
            {
                thread.set_error_code(EBADF);
                set_return_code(thread, -1);
                return -1;
            }
            uint32_t buf_addr = args[1];
            uint32_t count = args[2];

            // Write sizeof(io_buf) bytes at a time
            size_t total_written = 0;
            while (total_written < count)
            {
                size_t to_write = std::min(count - (uint32_t)total_written, (uint32_t)sizeof(io_buf));
                if (process->memory_space.memcpy(io_buf, buf_addr + total_written, to_write) != 0)
                {
                    thread.set_error_code(EIO);
                    error = 0;
                    set_return_code(thread, -1);
                    return -1; // Error copying from user space
                }
                ssize_t written_bytes = vfs.write(fd, io_buf, to_write);
                if (written_bytes < 0)
                {
                    thread.set_error_code(error);
                    error = 0;
                    set_return_code(thread, -1); // Error writing
                    return -1;
                }
                total_written += written_bytes;
            }
            set_return_code(thread, total_written); // Return the number of bytes written
            return 0;
        }
        default:
            // Unknown syscall
            thread.set_error_code(ENOSYS);
            set_return_code(thread, -1);
            return -1; // Not implemented
        }
    }
};
