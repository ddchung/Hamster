// Hamster read and write syscalls

#include <syscall/syscall.hpp>
#include <filesystem/vfs.hpp>
#include <memory/allocator.hpp>
#include <utility>

namespace Hamster
{
    static char IO_BUFFER[128];

    int sys_read(Thread &thread)
    {
        int32_t thread_fd = get_arg(thread, 0);
        uint32_t addr = get_arg(thread, 1);
        uint32_t size = get_arg(thread, 2);

        if (addr == 0)
        {
            return set_return(thread, -EINVAL);
        }

        if (size == 0)
        {
            return set_return(thread, 0);
        }

        int fd = deref_fd(thread, thread_fd);
        if (fd < 0)
        {
            return set_return(thread, -EBADF);
        }

        ssize_t to_read = size;
        while (to_read > 0)
        {
            ssize_t bytes_read = vfs.read(fd, IO_BUFFER, std::min<size_t>(sizeof(IO_BUFFER), to_read));
            if (bytes_read < 0)
            {
                return transfer_error(thread);
            }
            else if (bytes_read == 0)
            {
                break; // EOF
            }
            if (thread.get_process()->memory_space.memcpy(addr, IO_BUFFER, bytes_read) < 0)
            {
                return set_return(thread, -EFAULT);
            }
            addr += bytes_read;
            to_read -= bytes_read;
        }
        return set_return(thread, size - to_read);
    }

    int sys_write(Thread &thread)
    {
        int32_t thread_fd = get_arg(thread, 0);
        uint32_t addr = get_arg(thread, 1);
        uint32_t size = get_arg(thread, 2);

        if (addr == 0)
        {
            return set_return(thread, -EINVAL);
        }

        if (size == 0)
        {
            return set_return(thread, 0);
        }

        int fd = deref_fd(thread, thread_fd);
        if (fd < 0)
        {
            return set_return(thread, -EBADF);
        }

        ssize_t to_write = size;
        while (to_write > 0)
        {
            ssize_t bytes_to_write = std::min<size_t>(sizeof(IO_BUFFER), to_write);
            if (thread.get_process()->memory_space.memcpy(IO_BUFFER, addr, bytes_to_write) < 0)
            {
                return set_return(thread, -EFAULT);
            }
            ssize_t bytes_written = vfs.write(fd, IO_BUFFER, bytes_to_write);
            if (bytes_written < 0)
            {
                return transfer_error(thread);
            }

            while (bytes_written < bytes_to_write)
            {
                // If we didn't write all bytes, we need to write again
                ssize_t additional_bytes = vfs.write(fd, IO_BUFFER + bytes_written, bytes_to_write - bytes_written);
                if (additional_bytes < 0)
                {
                    return transfer_error(thread);
                }
                bytes_written += additional_bytes;

                if (bytes_written == 0)
                {
                    // No progress made, avoid infinite loop
                    return set_return(thread, -EIO);
                }
            }

            addr += bytes_written;
            to_write -= bytes_written;
        }
        return set_return(thread, size - to_write);
    }
}
