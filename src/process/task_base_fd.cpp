
#include <process/task_base_fd.hpp>
#include <errno/errno.h>

namespace Hamster
{
    ssize_t BaseTaskFD::read(void *buf, size_t size)
    {
        IOVec vec{.data = buf, .size = size};
        return readv(&vec, 1);
    }

    ssize_t BaseTaskFD::write(const void *buf, size_t size)
    {
        IOVec vec{.data = (void *)buf, .size = size};
        return writev(&vec, 1);
    }

    ssize_t BaseTaskFD::readv(const IOVec *iov, size_t iov_cnt)
    {
        size_t total_read = 0;
        for (const IOVec *it = iov; it - iov < iov_cnt; ++it)
        {
            // skip zero size buffers
            if (it->size == 0)
                continue;
            ssize_t bytes_read = read(it->data, it->size);
            
            // return success if bytes already read
            if (bytes_read < 0)
                return total_read > 0 ? total_read : -1;
            total_read += bytes_read;
            
            // short read, break
            if (bytes_read < it->size)
                break;
        }
        return total_read;
    }

    ssize_t BaseTaskFD::writev(const IOVec *iov, size_t iov_cnt)
    {
        // same as above
        size_t total_written = 0;
        for (const IOVec *it = iov; it - iov < iov_cnt; ++it)
        {
            if (it->size == 0) continue;
            ssize_t bytes_written = write(it->data, it->size);
            if (bytes_written < 0) return total_written > 0 ? total_written : -1;
            total_written += bytes_written;
            if (bytes_written < it->size) break;
        }
        return total_written;
    }

    int64_t BaseTaskFD::seek(int64_t, int)
    {
        error = H_ESPIPE;
        return -1;
    }

    int BaseTaskFD::truncate(int64_t)
    {
        error = H_EINVAL;
        return -1;
    }
} // namespace Hamster
