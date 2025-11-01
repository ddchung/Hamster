// Hamster U-Z system calls

#include <syscall/syscall.hpp>
#include <process/task.hpp>
#include <process/task_vfs_fd.hpp>
#include <abi/values.hpp>
#include <abi/structs.hpp>

namespace Hamster
{
    int32_t sys_write(Task &task, int32_t fd, uint32_t buf_loc, uint32_t count)
    {
        BaseTaskFD *user_fd = task.get_fd(fd);
        if (!user_fd)
            return cvt_error();

        int fd_flags = user_fd->get_flags();

        // Copy as many times as needed

        size_t total_written = 0;

        while (count > 0)
        {
            // Read at most to the end of the page
            uint32_t location = buf_loc + total_written;
            size_t to_write = std::min(count, HAMSTER_PAGE_SIZE - ((location) % HAMSTER_PAGE_SIZE));
            const uint8_t *mem = (const uint8_t *)task.mem_make_iterator_read(location);
            if (!mem)
            {
                if (total_written > 0)
                    return total_written;
                return cvt_error();
            }

            ssize_t bytes_written = user_fd->write(mem, to_write);

            if (bytes_written < 0)
            {
                if (total_written > 0)
                {
                    // Return the total bytes read so far
                    return total_written;
                }

                if (error == H_EAGAIN)
                {
                    // Blocking read, block only if O_NONBLOCK isn't set and we aren't already
                    // blocking
                    if ((fd_flags & OPEN_NONBLOCK) == 0 && !task.is_blocking())
                    {
                        task.block([](Task &task, uint64_t saved) {
                            int32_t blocking_fd = saved;

                            BaseTaskFD *task_fd = task.get_fd(blocking_fd);
                            int res;

                            if (!task_fd)
                                res = -1;
                            else
                                res = task_fd->poll(0x2); // WRITE

                            if (res == 0)
                                return; // not ready
                            else if (res == 1)
                            {
                                task.get_emulator().x[10];
                                task.get_emulator().x[10] = syscall(task, sys_write);
                                if ((int32_t)task.get_emulator().x[10] == -H_EAGAIN)
                                    return;
                            }
                            else
                            {
                                // error
                                task.get_emulator().x[10] = res;
                            }


                            task.end_block();
                        }, fd);

                        return 0;
                    }

                    return -H_EAGAIN;
                }
                return cvt_error(); // Return error if it isn't EAGAIN
            }

            if (bytes_written == 0)
                break;

            total_written += bytes_written;
            count -= bytes_written;
        }

        return total_written;
    }
} // namespace Hamster

