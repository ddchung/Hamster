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
                if (bytes_read == -EAGAIN)
                {
                    // Blocking read
                    current_task->blocking_operation = BlockingOperation::IO_READ;
                    current_task->io_block_fd = fd;
                    return cvt_error();
                }
                if (total_read == 0)
                {
                    return cvt_error(); // Return error if no bytes read
                }
                return total_read; // Return total bytes read so far
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
                if (bytes_written == -EAGAIN)
                {
                    // Blocking write
                    current_task->blocking_operation = BlockingOperation::IO_WRITE;
                    current_task->io_block_fd = fd;
                    return cvt_error();
                }
                if (total_written == 0)
                {
                    return cvt_error(); // Return error if no bytes written
                }
                return total_written; // Return total bytes written so far
            }

            total_written += bytes_written;
            count -= bytes_written;
        }

        return total_written;
    }
} // namespace Hamster


