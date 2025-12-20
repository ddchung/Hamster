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

    BaseTaskFD *TaskFDTable::get_fd(int fd) const
    {
        if (fd < 0 || fd >= (int)fd_table.size())
        {
            error = H_EBADF;
            return nullptr;
        }

        const auto &p_fd = fd_table[fd];
        if (!p_fd.fd)
        {
            error = H_EBADF;
            return nullptr;
        }

        assert(p_fd.fd->fd);

        return p_fd.fd->fd;
    }

    int TaskFDTable::get_fd_flags(int fd) const
    {
        if (fd < 0 || fd >= (int)fd_table.size())
        {
            error = H_EBADF;
            return -1;
        }
        return fd_table[fd].fd_flags;
    }

    int TaskFDTable::set_fd_flags(int fd, int flags)
    {
        if (fd < 0 || fd >= (int)fd_table.size())
        {
            error = H_EBADF;
            return -1;
        }
        fd_table[fd].fd_flags = flags;
        return 0;
    }

    int TaskFDTable::close(int fd)
    {
        if (fd < 0 || fd >= (int)fd_table.size())
        {
            error = H_EBADF;
            return -1;
        }

        auto &p_fd = fd_table[fd];
        if (!p_fd.fd)
        {
            error = H_EBADF;
            return -1;
        }

        p_fd.fd.clear();
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

        if (fd < 0 || fd >= (int)fd_table.size())
        {
            error = H_EBADF;
            return -1;
        }

        fd_table[fd].fd.construct(task_fd);
        fd_table[fd].fd_flags = 0;
        return fd;
    }

    int TaskFDTable::dup(int fd, int new_fd)
    {
        if (fd < 0 || new_fd < 0 || fd >= (int)fd_table.size() || new_fd >= (int)fd_table.size())
        {
            error = H_EBADF;
            return -1;
        }

        auto &p_fd = fd_table[fd];
        if (!p_fd.fd)
        {
            error = H_EBADF;
            return -1;
        }

        fd_table[new_fd] = p_fd;
        return 0;
    }

    int TaskFDTable::allocate_fd(int start)
    {
        if (start < 0)
        {
            error = H_EINVAL;
            return -1;
        }

        // Skip used fd's
        int it = start;
        for (; it < (int)fd_table.size() && fd_table[it].fd; ++it)
            ;

        if (it >= (int)fd_table.size())
        {
            if (it > HAMSTER_MAX_FD_TABLE_SIZE)
            {
                error = H_EMFILE;
                return -1;
            }
            
            fd_table.resize(it + 1);
        }

        return it;
    }

    void TaskFDTable::close_cloexec()
    {
        for (auto &fd : fd_table)
        {
            if (fd.fd)
            {
                // Remove if flags has FD_CLOEXEC
                if (fd.fd_flags & H_FD_CLOEXEC)
                    fd.fd.clear();
            }
        }
    }
} // namespace Hamster

