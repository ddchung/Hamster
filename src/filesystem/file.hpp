// Hamster file

#pragma once

#include <sys/stat.h>
#include <cstdint>
#include <cstddef>

namespace Hamster
{
    enum class FileType
    {
        Regular,
        Directory,
        FIFO,
    };

    class BaseFile
    {
    public:
        virtual ~BaseFile() = default;

        virtual const FileType type() const = 0;

        virtual int rename(const char *) = 0;
        virtual int remove() = 0;
    };

    class BaseFifoFile : public BaseFile
    {
    public:
        virtual const FileType type() const override { return FileType::FIFO; }
        virtual int read(uint8_t *buf, size_t size) = 0;
        virtual int write(const uint8_t *buf, size_t size) = 0;
    };

    class BaseRegularFile : public BaseFifoFile
    {
    public:
        virtual const FileType type() const override { return FileType::Regular; }
        virtual int64_t seek(int64_t offset, int whence) = 0;
    };

    class BaseDirectory : public BaseFile
    {
    public:
        virtual const FileType type() const override { return FileType::Directory; }
        virtual char * const *list() = 0;
        virtual BaseFile *get(const char *name, int flags, ...) = 0;
        virtual BaseFile *add(const char *name, int mode, BaseFile *file) = 0;

        using BaseFile::remove;
        virtual int remove(const char *name) = 0;
        virtual int close() = 0;
    };

    class BaseFilesystem
    {
    public:
        virtual ~BaseFilesystem() = default;

        virtual BaseFile *open(const char *path, int flags, ...) = 0;
        virtual int rename(const char *oldpath, const char *newpath) = 0;
        virtual int link(const char *oldpath, const char *newpath) = 0;
        virtual int unlink(const char *path) = 0;
        virtual int stat(const char *path, struct ::stat *buf) = 0;
    };
} // namespace Hamster

