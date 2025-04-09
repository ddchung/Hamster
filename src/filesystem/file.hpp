// Hamster file

#pragma once

#include <sys/stat.h>
#include <cstdint>
#include <cstddef>

namespace Hamster
{
    class BaseFile
    {
    public:
        virtual ~BaseFile() = default;

        virtual int rename(const char *) = 0;
        virtual int isatty() = 0;
        virtual int unlink() = 0;
        virtual int stat(struct ::stat *) = 0;
        virtual int close() = 0;
        virtual int write(const void *, size_t) = 0;
        virtual int64_t lseek(int64_t, int) = 0;
        virtual int read(void *, size_t) = 0;
    };

    class BaseFilesystem
    {
    public:
        virtual ~BaseFilesystem() = default;

        // opens a file
        // returns a pointer to file to be freed with:
        // `Hamster::dealloc<BaseFile>(file)`
        virtual BaseFile *open(const char *, int, ...) = 0;
        virtual int rename(const char *, const char *) = 0;
        virtual int unlink(const char *) = 0;
        virtual int link(const char *, const char *) = 0;
        virtual int stat(const char *, struct ::stat *) = 0;
    };
} // namespace Hamster

