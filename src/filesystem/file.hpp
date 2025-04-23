// Hamster file

#pragma once

#include <sys/stat.h>
#include <cstdint>
#include <cstddef>
#include <cstdarg>

// expose some macros
#include <fcntl.h>

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
        virtual int stat(struct ::stat *buf) = 0;

        virtual int mode() const = 0;

        virtual int chmod(int mode) = 0;
        virtual int chown(int uid, int gid) = 0;

        // returns a weak pointer, so don't modify or delete
        // data should be valid until the file is closed or renamed
        virtual const char *name() const = 0;
    };

    class BaseFifoFile : public BaseFile
    {
    public:
        virtual const FileType type() const override { return FileType::FIFO; }
        virtual ssize_t read(uint8_t *buf, size_t size) = 0;
        virtual ssize_t write(const uint8_t *buf, size_t size) = 0;
    };

    class BaseRegularFile : public BaseFifoFile
    {
    public:
        virtual const FileType type() const override { return FileType::Regular; }
        virtual int64_t seek(int64_t offset, int whence) = 0;
        virtual int64_t tell() const = 0;
        virtual int truncate(int64_t size) = 0;
        virtual int64_t size() const = 0;
    };

    class BaseDirectory : public BaseFile
    {
    public:
        virtual const FileType type() const override { return FileType::Directory; }
        virtual char * const *list() = 0;
        virtual BaseFile *get(const char *name, int flags, ...) = 0;

        virtual BaseFile *mkfile(const char *name, int flags, int mode) = 0;
        virtual BaseFile *mkdir(const char *name, int flags, int mode) = 0;
        virtual BaseFile *mkfifo(const char *name, int flags, int mode) = 0;

        using BaseFile::remove;
        virtual int remove(const char *name) = 0;
    };

    class BaseFilesystem
    {
    public:
        virtual ~BaseFilesystem() = default;

        virtual BaseFile *open(const char *path, int flags, ...) = 0;
        virtual BaseFile *open(const char *path, int flags, va_list args) = 0;
        virtual int rename(const char *oldpath, const char *newpath) = 0;
        virtual int link(const char *oldpath, const char *newpath) = 0;
        virtual int unlink(const char *path) = 0;
        virtual int stat(const char *path, struct ::stat *buf) = 0;
        
        virtual BaseDirectory *mkdir(const char *path, int flags, int mode) = 0;
        virtual BaseFifoFile *mkfifo(const char *path, int flags, int mode) = 0;
        virtual BaseRegularFile *mkfile(const char *path, int flags, int mode) = 0;
    };
} // namespace Hamster

