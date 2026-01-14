#include <filesystem/base_file.hpp>
#include <errno/errno.h>

namespace Hamster
{
    ssize_t BaseRegularFile::read(uint8_t *buf, size_t size)
    {
        ssize_t ret = pread(buf, size, position);
        if (ret < 0)
            return ret;
        position += ret;
        return ret;
    }

    ssize_t BaseRegularFile::write(const uint8_t *buf, size_t size)
    {
        ssize_t ret = pwrite(buf, size, position);
        if (ret < 0)
            return ret;
        position += ret;
        return ret;
    }

    int64_t BaseRegularFile::seek(int64_t offset, int whence)
    {
        int64_t new_pos;
        switch (whence)
        {
        case H_SEEK_SET:
            new_pos = offset;
            break;
        case H_SEEK_CUR:
            new_pos = position + offset;
            break;
        case H_SEEK_END:
        {
            int64_t file_size = size();
            if (file_size < 0)
                return -1;
            new_pos = file_size + offset;
            break;
        }
        default:
            error = H_EINVAL;
            return -1;
        }

        // disallow negative positions, but not beyond end of file
        // See lseek(2)
        if (new_pos < 0)
        {
            error = H_EINVAL;
            return -1;
        }

        position = new_pos;
        return position;
    }
} // namespace Hamster

