// Hamster file wrapper

#pragma once

#include <filesystem/vfs.hpp>

namespace Hamster
{
    class File
    {
    public:
        File(int fd = -1);

        File(const File &other);

        File &operator=(const File &other);

        File(File &&other);

        File &operator=(File &&other);

        ~File();

        operator bool() const
        { return fd >= 0; }

        int get_fd() const
        { return fd; }

        // Note: These are mostly just wrappers around VFS functions
        // However, the *at functions are relative to this, if it is
        // a directory, AND the path is relative. Otherwise, they are
        // relative to the root of the VFS.
        //
        // This is in contrast to the VFS *at functions, which are always
        // relative to the directory specified, even if the path is absolute.

        File openat(const char *path, int flags, int mode = 0);

        // This replaces the current file
        int openat_replace(const char *path, int flags, int mode = 0);

        int close();

        int removeat(const char *path);

        int stat(sys_stat *buf);

        int statat(const char *path, sys_stat *buf);

        int lstatat(const char *path, sys_stat *buf);

        int get_mode();
        int get_flags();
        int get_uid();
        int get_gid();

        int chmod(int mode);

        int chown(int uid, int gid);

        ssize_t read(void *buf, size_t size);

        ssize_t write(void *buf, size_t size);

        int seek(int offset, int whence);

        int64_t tell();

        int truncate(int64_t size);

        int64_t size();

        int set_targetat(const char *path, const char *target);

        char *const *list(size_t count = SIZE_MAX);

        int mkfileat(const char *path, int mode);

        File mkfileat(const char *path, int flags, int mode);

        int mkdirat(const char *path, int mode);

        File mkdirat(const char *path, int flags, int mode);

        int symlinkat(const char *path, const char *target);

        int mknodat(const char *path, DeviceID id, int mode);

        File mknodat(const char *path, int flags, DeviceID id, int mode);

        int ioctl(int request, IoctlArg arg = IoctlArg());

        int set_flags(int flags);

    private:
        int fd;
    };
} // namespace Hamster

