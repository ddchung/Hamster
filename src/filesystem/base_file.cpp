#include <filesystem/base_file.hpp>
#include <memory/allocator.hpp>
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

    BaseSpecialFile::BaseSpecialFile(BaseSpecialDriverHandle *handle)
        : handle(handle)
    {
    }

    BaseSpecialFile::~BaseSpecialFile()
    {
        dealloc(handle);
        handle = nullptr;
    }

    BaseSpecialFile *BaseDirectory::mksfile(const char *name, int flags, BaseSpecialDriver *driver, int mode)
    {
        error = H_ENOTSUP;
        return nullptr;
    }

    int64_t BaseSpecialDriverHandle::seek(int64_t offset, int whence)
    {
        error = H_ESPIPE;
        return -1;
    }

    int64_t BaseSpecialDriverHandle::tell()
    {
        return seek(0, H_SEEK_CUR);
    }
} // namespace Hamster

