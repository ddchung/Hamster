// Hamster pipe

#include <process/task_pipe.hpp>
#include <process/task.hpp>

namespace Hamster
{
    TaskPipe::~TaskPipe()
    {
        switch (flags & OPEN_ACCMODE)
        {
        case OPEN_RDONLY:
            --data->readers;
            break;
        case OPEN_WRONLY:
            --data->writers;
            break;
        default:
            __builtin_unreachable();
            return;
        }

        if (data->readers == 0 && data->writers == 0)
        {
            // Destroy the pipe
            dealloc(data);
        }

        data = nullptr;
    }

    std::pair<TaskPipe *, TaskPipe *> TaskPipe::make_pair(int flags)
    {
        flags &= ~OPEN_NONBLOCK;

        TaskPipeData *data = alloc<TaskPipeData>();
        data->readers = 1;
        data->writers = 1;

        auto pair = std::make_pair(alloc<TaskPipe>(), alloc<TaskPipe>());

        pair.first->data = data;
        pair.second->data = data;

        pair.first->flags = OPEN_RDONLY | flags;
        pair.second->flags = OPEN_WRONLY | flags;

        return pair;
    }

    ssize_t TaskPipe::write(const void *buf, size_t len)
    {
        assert(buf && data && data->writers > 0);

        if (data->readers == 0)
        {
            // Send SIGPIPE to userland, and error with EPIPE
            Task *current_task = Task::get_current_task();
            if (current_task)
                current_task->send_signal_process(make_kill_siginfo(H_SIGPIPE));
            
            error = H_EPIPE;
            return -1;
        }

        if (data->data.size() + len > HAMSTER_MAX_PIPE_BUFFERED)
        {
            // Block until more space available
            error = H_EAGAIN;
            return -1;
        }

        for (size_t it = 0; it < len; ++it)
            data->data.push_back(((const char *)buf)[it]);
        return len;
    }

    ssize_t TaskPipe::read(void *buf, size_t len)
    {
        assert(buf && data && data->readers > 0);

        if (data->data.size() == 0)
        {
            if (data->writers == 0)
                return 0; // No writers and out of data, EOF
            
            // Block for more data
            error = H_EAGAIN;
            return -1;
        }

        len = std::min(len, data->data.size());

        for (size_t i = 0; i < len; ++i)
        {
            ((char *)buf)[i] = data->data.front();
            data->data.pop_front();
        }

        return len;
    }

    int64_t TaskPipe::seek(int64_t, int) { return tell(); }
    int64_t TaskPipe::tell()
    {
        error = H_ESPIPE;
        return -1;
    }

    int TaskPipe::stat(sys_stat *buf)
    {
        assert(buf);

        memset(buf, 0, sizeof(sys_stat));

        buf->mode = 0777 | STAT_IFIFO;
        buf->nlink = data->readers + data->writers;
        buf->size = data->data.size();

        return 0;
    }

    int TaskPipe::truncate(int64_t)
    {
        error = H_EINVAL;
        return -1;
    }

    int64_t TaskPipe::size()
    {
        return data->data.size();
    }

    int TaskPipe::ioctl(int req, IoctlArg arg)
    {
        switch (req)
        {
        case H_FIONREAD:
            *(uint32_t *)arg.p = data->data.size();
            return 0;
        default:
            error = H_EINVAL;
            return -1;
        }
    }

    int TaskPipe::set_flags(int flags)
    {
        this->flags = flags;
        return 0;
    }

    int TaskPipe::get_flags()
    {
        return flags;
    }

    int TaskPipe::poll(int op)
    {
        if ((op & POLL_READ) && size() == 0 && data->writers != 0)
            return 0;
        if ((op & POLL_WRITE) && (size() >= HAMSTER_MAX_PIPE_BUFFERED || data->readers == 0))
            return 0;
        return 1;
    }

    int TaskPipe::sync()
    {
        return 0;
    }

    int TaskPipe::datasync()
    {
        return 0;
    }

    int TaskPipe::get_vfs_fd()
    {
        error = H_EINVAL;
        return -1;
    }
} // namespace Hamster

