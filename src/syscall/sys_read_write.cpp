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
    } // namespace

    int Task::poll_read()
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr && "No current task");
        assert(current_task->blocking_operation == BlockingOperation::IO_READ && "Not a blocking read operation");
        
        // Call the system call

        // Load the blocking file descriptor
        current_task->emulator.x[10] = current_task->io_block_fd;

        int32_t result = syscall(sys_read);

        if (result < 0 && result == -EAGAIN)
        {
            // Still blocking, do nothing and check again next time
            return 0;
        }

        _trace("sys_read: completed read on Thread FD %d, bytes read %d\n",
               current_task->io_block_fd, result);
        
        // Completed successfully
        // Copy to a0 register (return value)

        current_task->emulator.x[10] = result;
        current_task->blocking_operation = BlockingOperation::NONE;
        current_task->io_block_fd = -1; // Reset the blocking FD
        return 0;
    }

    int Task::poll_write()
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr && "No current task");
        assert(current_task->blocking_operation == BlockingOperation::IO_WRITE && "Not a blocking write operation");

        // Call the system call

        // Load the blocking file descriptor
        current_task->emulator.x[10] = current_task->io_block_fd;

        int32_t result = syscall(sys_write);
        if (result < 0 && result == -EAGAIN)
        {
            // Still blocking, do nothing and check again next time
            return 0;
        }

        _trace("sys_write: completed write on Thread FD %d, bytes written %d\n",
               current_task->io_block_fd, result);

        // Completed successfully

        // Copy to a0 register (return value)
        current_task->emulator.x[10] = result;
        current_task->blocking_operation = BlockingOperation::NONE;
        current_task->io_block_fd = -1; // Reset the blocking FD
        return 0;
    }

    int32_t sys_read(int32_t fd, uint32_t buf_loc, uint32_t count)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr && "No current task");

        int vfs_fd = current_task->get_vfs_fd(fd);
        if (vfs_fd < 0)
            return cvt_error();

        // Copy as many times as needed

        size_t total_read = 0;

        while (count > 0)
        {
            size_t to_read = std::min(count, (uint32_t)sizeof(IO_BUFFER));
            ssize_t bytes_read = vfs.read(vfs_fd, IO_BUFFER, to_read);
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
                    if ((vfs.get_flags(vfs_fd) & OPEN_NONBLOCK) == 0)
                    {
                        if (current_task->blocking_operation == BlockingOperation::NONE)
                            _trace("sys_read: blocking read on Thread FD %d, count %u\n", fd, count);

                        current_task->blocking_operation = BlockingOperation::IO_READ;
                        current_task->io_block_fd = fd;
                    }

                    return -EAGAIN;
                }
                return cvt_error(); // Return error if no bytes read
            }

            if (bytes_read == 0)
                break;
            
            // Copy to user memory
            if (current_task->memory->obj.memory.memcpy(buf_loc + total_read, IO_BUFFER, bytes_read) < 0)
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

        int vfs_fd = current_task->get_vfs_fd(fd);
        if (vfs_fd < 0)
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

            ssize_t bytes_written = vfs.write(vfs_fd, IO_BUFFER, to_write);
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
                    if (current_task->blocking_operation == BlockingOperation::NONE &&
                        (vfs.get_flags(vfs_fd) & OPEN_NONBLOCK) == 0)
                    {
                        _trace("sys_write: blocking write on Thread FD %d, count %u\n", fd, count);

                        current_task->blocking_operation = BlockingOperation::IO_WRITE;
                        current_task->io_block_fd = fd;
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


