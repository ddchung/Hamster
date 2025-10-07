// Hamster FD table implementation

#include <process/task_fd_table.hpp>
#include <platform/config.hpp>
#include <memory/allocator.hpp>
#include <errno/errno.h>
#include <cassert>

namespace Hamster
{
    TaskFDTable::TaskFDHolder::~TaskFDHolder()
    {
        dealloc(fd);
    }

    BaseTaskFD *TaskFDTable::get_fd(int fd)
    {
        if (fd >= (int)fd_table.size())
        {
            error = H_EBADF;
            return nullptr;
        }

        const auto &p_fd = fd_table[fd];
        if (!p_fd)
        {
            error = H_EBADF;
            return nullptr;
        }

        assert(p_fd->fd);

        return p_fd->fd;
    }

    int TaskFDTable::close(int fd)
    {
        if (fd >= (int)fd_table.size())
        {
            error = H_EBADF;
            return -1;
        }

        auto &p_fd = fd_table[fd];
        if (!p_fd)
        {
            error = H_EBADF;
            return -1;
        }

        p_fd.clear();
        return 0;
    }

    int TaskFDTable::set_fd(BaseTaskFD *task_fd, int fd)
    {
        // Allocate a fd if it's -1
        if (fd == -1)
        {
            fd = allocate_fd();
            if (fd < 0)
                return -1;
        }

        if (fd >= (int)fd_table.size())
        {
            error = H_EBADF;
            return -1;
        }

        fd_table[fd].construct(task_fd);
        return 0;
    }

    int TaskFDTable::dup(int fd, int new_fd)
    {
        if (fd >= (int)fd_table.size() || new_fd >= (int)fd_table.size())
        {
            error = H_EBADF;
            return -1;
        }

        auto &p_fd = fd_table[fd];
        if (!p_fd)
        {
            error = H_EBADF;
            return -1;
        }

        fd_table[new_fd] = p_fd;
        return 0;
    }

    int TaskFDTable::allocate_fd(int start)
    {
        // note: > and not >= because if `start` == `fd_table.size()`, we
        //      allocate a slot at the end of the table
        if (start > (int)fd_table.size())
        {
            error = H_EINVAL;
            return -1;
        }

        // Skip used fd's
        int it = start;
        for (; it < (int)fd_table.size() && fd_table[it]; ++it)
            ;

        if (it == (int)fd_table.size())
        {
            if (it > HAMSTER_MAX_FD_TABLE_SIZE)
            {
                error = EMFILE;
                return -1;
            }
            
            fd_table.emplace_back();
        }

        return it;
    }

    void TaskFDTable::close_cloexec()
    {
        for (auto &fd : fd_table)
        {
            if (fd)
            {
                // Remove if flags has OPEN_CLOEXEC
                int flags = fd->fd->get_flags();
                if (flags != -1 && (flags & OPEN_CLOEXEC))
                    fd.clear();
            }
        }
    }
} // namespace Hamster

