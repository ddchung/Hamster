// Hamster P-T system calls

#include <syscall/syscall.hpp>
#include <process/task.hpp>
#include <process/task_vfs_fd.hpp>
#include <abi/values.hpp>
#include <abi/structs.hpp>

namespace Hamster
{
    int32_t sys_read(Task &task, int32_t fd, uint32_t buf_loc, uint32_t count)
    {
        BaseTaskFD *user_fd = task.get_fd_table()->get_fd(fd);
        if (!user_fd)
            return cvt_error();

        int fd_flags = user_fd->get_flags();

        // Copy as many times as needed

        size_t total_read = 0;

        while (count > 0)
        {
            // Read at most to the end of the page
            uint32_t location = buf_loc + total_read;
            size_t to_read = std::min(count, HAMSTER_PAGE_SIZE - ((location) % HAMSTER_PAGE_SIZE));
            uint8_t *mem = (uint8_t *)task.get_memory().make_iterator(location);
            if (!mem)
            {
                if (total_read > 0)
                    return total_read;
                return cvt_error();
            }

            ssize_t bytes_read = user_fd->read(mem, to_read);

            if (bytes_read < 0)
            {
                if (total_read > 0)
                {
                    // Return the total bytes read so far
                    return total_read;
                }

                if (error == H_EAGAIN)
                {
                    // Blocking read, block only if O_NONBLOCK isn't set and we aren't already
                    // blocking
                    if ((fd_flags & OPEN_NONBLOCK) == 0 && !task.is_blocking())
                    {
                        task.get_blocking_saved()[0] = fd;
                        task.block([](Task &task) {
                            int32_t blocking_fd = task.get_blocking_saved()[0];

                            BaseTaskFD *task_fd = task.get_fd_table()->get_fd(blocking_fd);
                            int res;

                            if (!task_fd)
                                res = -1;
                            else
                                res = task_fd->poll(0x1); // READ

                            if (res == 0)
                                return; // not ready
                            else if (res == 1)
                            {
                                task.get_emulator().x[10];
                                task.get_emulator().x[10] = syscall(task, sys_read);
                                if ((int32_t)task.get_emulator().x[10] == -H_EAGAIN)
                                    return;
                            }
                            else
                            {
                                // error
                                task.get_emulator().x[10] = res;
                            }


                            task.end_block();
                        });

                        return 0;
                    }

                    return -H_EAGAIN;
                }
                return cvt_error(); // Return error if it isn't EAGAIN
            }

            if (bytes_read == 0)
                break;

            total_read += bytes_read;
            count -= bytes_read;
        }

        return total_read;
    }
} // namespace Hamster

