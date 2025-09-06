// Hamster read and write system calls

#include <syscall/syscall.hpp>
#include <process/scheduler.hpp>
#include <filesystem/vfs.hpp>
#include <errno/errno.h>
#include <cassert>

namespace Hamster
{
    namespace
    {
        // Buffer for read/write operations
        char IO_BUFFER[512];

        void poll_read(Task &task)
        {
            int32_t blocking_fd = task.blocking_operation_saved[0];

            UserFD *user_fd = task.get_user_fd(blocking_fd);
            int res;

            if (!user_fd)
            {
                res = cvt_error();
            }
            else
            {
                switch (user_fd->type)
                {
                case UserFDType::VFS:
                    res = vfs.poll(user_fd->vfs_fd, 0x1); // Poll for read
                    break;
                case UserFDType::PIPE_READ:
                    res = user_fd->pipe->poll(0x1); // Poll for read
                    break;
                case UserFDType::PIPE_WRITE:
                case UserFDType::PID:
                default:
                    res = -EINVAL;
                    break;
                }
            }

            if (res == 0)
                return; // not ready
            else if (res == 1)
            {
                task.emulator.x[10] = blocking_fd;
                task.emulator.x[10] = syscall(sys_read);
                if ((int32_t)task.emulator.x[10] == -EAGAIN)
                    return;
            }
            else
            {
                // error
                task.emulator.x[10] = res;
            }


            task.blocking_operation = nullptr;
        }

        void poll_write(Task &task)
        {
            int32_t blocking_fd = task.blocking_operation_saved[0];

            UserFD *user_fd = task.get_user_fd(blocking_fd);
            int res;

            if (!user_fd)
            {
                res = cvt_error();
            }
            else
            {
                switch (user_fd->type)
                {
                case UserFDType::VFS:
                    res = vfs.poll(user_fd->vfs_fd, 0x2); // Poll for write
                    break;
                case UserFDType::PIPE_WRITE:
                    res = user_fd->pipe->poll(0x2); // Poll for write
                    break;
                case UserFDType::PIPE_READ:
                case UserFDType::PID:
                default:
                    res = -EINVAL;
                    break;
                }
            }

            if (res == 0)
                return; // not ready
            else if (res == 1)
            {
                task.emulator.x[10] = blocking_fd;
                task.emulator.x[10] = syscall(sys_write);
                if ((int32_t)task.emulator.x[10] == -EAGAIN)
                    return;
            }
            else
            {
                // error
                task.emulator.x[10] = res;
            }


            task.blocking_operation = nullptr;
        }
    } // namespace

    int32_t sys_read(int32_t fd, uint32_t buf_loc, uint32_t count)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr && "No current task");

        UserFD *user_fd = current_task->get_user_fd(fd);
        if (!user_fd)
            return cvt_error();

        // Copy as many times as needed

        size_t total_read = 0;

        while (count > 0)
        {
            size_t to_read = std::min(count, (uint32_t)sizeof(IO_BUFFER));

            ssize_t bytes_read = 0;
            bool is_open_nonblock = false;

            switch (user_fd->type)
            {
            case UserFDType::VFS:
                bytes_read = vfs.read(user_fd->vfs_fd, IO_BUFFER, to_read);
                is_open_nonblock = vfs.get_flags(user_fd->vfs_fd) & OPEN_NONBLOCK;
                break;
            case UserFDType::PIPE_READ:
                bytes_read = user_fd->pipe->read(IO_BUFFER, to_read);
                is_open_nonblock = user_fd->flags & USER_FD_PIPE_NONBLOCK;
                break;
            default:
                return -EPERM;
            }

            if (bytes_read < 0)
            {
                if (total_read > 0)
                {
                    // Return the total bytes read so far
                    return total_read;
                }

                if (error == EAGAIN)
                {
                    // Blocking read
                    if (!is_open_nonblock)
                    {
                        if (!current_task->blocking_operation)
                            _trace("sys_read: blocking read on Thread FD %d, count %u\n", fd, count);

                        current_task->blocking_operation = poll_read;
                        current_task->blocking_operation_saved[0] = fd;
                    }

                    return -EAGAIN;
                }
                return cvt_error(); // Return error if no bytes read
            }

            if (bytes_read == 0)
                break;

            // Copy to user memory
            if (current_task->memory->obj.memory.memcpy_alloc(buf_loc + total_read, IO_BUFFER, bytes_read) < 0)
            {
                error = EFAULT;
                return cvt_error();
            }
            total_read += bytes_read;
            count -= bytes_read;
        }

        return total_read;
    }

    int32_t sys_write(int32_t fd, uint32_t buf_loc, uint32_t count)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr && "No current task");

        UserFD *user_fd = current_task->get_user_fd(fd);
        if (!user_fd)
            return cvt_error();

        // Copy as many times as needed

        size_t total_written = 0;

        while (count > 0)
        {
            size_t to_write = std::min(count, (uint32_t)sizeof(IO_BUFFER));
            if (current_task->memory->obj.memory.memcpy(IO_BUFFER, buf_loc + total_written, to_write) < 0)
            {
                error = EFAULT;
                return cvt_error();
            }

            ssize_t bytes_written = 0;
            bool is_open_nonblock = false;

            switch (user_fd->type)
            {
            case UserFDType::VFS:
                bytes_written = vfs.write(user_fd->vfs_fd, IO_BUFFER, to_write);
                is_open_nonblock = vfs.get_flags(user_fd->vfs_fd) & OPEN_NONBLOCK;
                break;
            case UserFDType::PIPE_WRITE:
                bytes_written = user_fd->pipe->write(IO_BUFFER, to_write);
                is_open_nonblock = user_fd->flags & USER_FD_PIPE_NONBLOCK;
                break;
            default:
                return -EPERM;
            }

            if (bytes_written < 0)
            {
                if (total_written > 0)
                {
                    // Return the total bytes written so far
                    return total_written;
                }

                if (error == EAGAIN)
                {
                    // Blocking write
                    if (!current_task->blocking_operation && !is_open_nonblock)
                    {
                        _trace("sys_write: blocking write on Thread FD %d, count %u\n", fd, count);

                        current_task->blocking_operation = poll_write;
                        current_task->blocking_operation_saved[0] = fd;
                    }

                    return -EAGAIN;
                }
                return cvt_error(); // Return error if no bytes written
            }

            total_written += bytes_written;
            count -= bytes_written;
        }

        return total_written;
    }
} // namespace Hamster
