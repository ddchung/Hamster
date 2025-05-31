// Hamster syscalls

#include <syscall/syscall.hpp>
#include <syscall/syscall_id.hpp>
#include <process/thread.hpp>
#include <process/process.hpp>
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
        uint8_t io_buf[64]{0};
    } // namespace

    int do_syscall(Thread &thread)
    {
        Process *process = thread.get_process();
        uint32_t *args = get_syscall_args(thread);

        int fd;
        switch (get_syscall_num(thread))
        {
        case SyscallID::EXIT:
            process->exit_code = args[0];
            process->threads.clear();
            printf("Process %u exited with code %u\n", process->pid, process->exit_code);
            // don't set return code, as the thread is destroyed
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
            set_return_code(thread, 0); // Return 0 on success
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
            set_return_code(thread, 0); // Return 0 on success
            return 0;
        }
        case SyscallID::FORK:
            // TODO: Implement this when we have a scheduler
            thread.set_error_code(ENOSYS);
            set_return_code(thread, -1);
            return -1;
        case SyscallID::FSTAT:
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
            // Copy the stat structure to the user space
            if (process->memory_space.memcpy(args[1], io_buf, sizeof(struct stat)) != 0)
            {
                thread.set_error_code(EIO);
                error = 0;
                set_return_code(thread, -1);
                return -1;
            }
            set_return_code(thread, 0); // Return 0 on success
            return 0;
        case SyscallID::GETPID:
            // Return the process ID
            set_return_code(thread, process->pid);
            return 0;
        case SyscallID::ISATTY:
            fd = deref_fildes(args[0], process);
            if (fd < 0)
            {
                thread.set_error_code(EBADF);
                set_return_code(thread, -1);
                return -1;
            }
            // For now, we assume all file descriptors are TTYs
            // TODO: TTY detection in special files
            set_return_code(thread, 1); // Return 1 for TTY
            return 0;
        case SyscallID::KILL:
        {
            uint32_t pid = args[0];
            int signal = args[1];
            // TODO: Search for the right process when we have a scheduler
            if (pid == process->pid)
            {
                // Signal to the first thread that doesn't mask it
                for (auto &thread : process->threads)
                {
                    if (!(thread.get_signal_mask() & (1 << signal)))
                        continue; // This thread masks the signal
                    thread.signal(signal);
                    set_return_code(thread, 0); // Signal sent
                    return 0; // Signal sent
                }
                // No thread found that doesn't mask the signal
                thread.set_error_code(ESRCH);
                set_return_code(thread, -1); // No such process
                return -1;
            }
            thread.set_error_code(ENOSYS);
            set_return_code(thread, -1); // Not implemented yet
            return -1; // Not implemented yet
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
                size_t to_read = std::min(count - total_read, sizeof(io_buf));
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
            return total_read; // Return the number of bytes read
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
            int flags = args[1];
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
            return 0; // Success
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
            int ret = vfs.unlink(resolved_path.c_str());
            if (ret < 0)
            {
                thread.set_error_code(error);
                error = 0;
                set_return_code(thread, -1);
                return -1;
            }
            set_return_code(thread, 0); // Return 0 on success
            return 0; // Success
        }
        case SyscallID::WAIT:
        {
            // TODO: Implement wait
            thread.set_error_code(ENOSYS);
            set_return_code(thread, -1);
            return -1; // Not implemented yet
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
                size_t to_write = std::min(count - total_written, sizeof(io_buf));
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
            printf("Process %u Thread %zu: Unknown syscall %u\n", process->pid, thread.get_id(), get_syscall_num(thread));
            thread.set_error_code(ENOSYS);
            set_return_code(thread, -1);
            return -1; // Not implemented
        }
    }
};
