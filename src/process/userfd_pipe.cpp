// Hamster pipe

#include <process/task.hpp>
#include <errno/errno.h>
#include <platform/config.hpp>
#include <process/scheduler.hpp>
#include <cassert>

namespace Hamster
{
    bool UserFDPipe::is_destroyable()
    {
        return readers == 0 && writers == 0;
    }

    ssize_t UserFDPipe::write(const void *buf, size_t size)
    {
        assert(buf);
        assert(writers > 0);

        if (readers == 0)
        {
            Task *current_task = scheduler.get_current_task();
            if (current_task)
                current_task->process->obj.send_signal(H_SIGPIPE);
            error = H_EPIPE;
            return -1;
        }

        if (buffer.size() + size > HAMSTER_MAX_PIPE_BUFFERED)
        {
            // Block until space is available
            error = H_EAGAIN;
            return -1;
        }

        for (size_t i = 0; i < size; ++i)
            buffer.push_back(((const char *)buf)[i]);
        return size;
    }

    ssize_t UserFDPipe::read(void *buf, size_t size)
    {
        assert(buf);
        assert(readers > 0);

        if (buffer.size() == 0)
        {
            if (writers == 0)
                return 0; // EOF

            // Block
            error = H_EAGAIN;
            return -1;
        }

        if (size > buffer.size())
            size = buffer.size();

        for (size_t i = 0; i < size; ++i)
        {
            ((char *)buf)[i] = buffer.front();
            buffer.pop_front();
        }

        return size;
    }

    int UserFDPipe::poll(int op)
    {
        if (op & 0x1) // Read
        {
            if (buffer.size() == 0 && writers != 0)
                return 0; // No data, not ready
        }
        if (op & 0x2) // Write
        {
            if (buffer.size() >= HAMSTER_MAX_PIPE_BUFFERED ||
                readers == 0)
                return 0; // No space, not ready
        }

        return 1;
    }
} // namespace Hamster
