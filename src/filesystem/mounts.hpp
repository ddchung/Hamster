// Mounted filesystems

#pragma once

#include <filesystem/file.hpp>
#include <memory/stl_sequential.hpp>
#include <platform/config.hpp>

namespace Hamster
{
    class Mounts : public BaseFilesystem
    {
        struct Mount
        {
            Mount(BaseFilesystem *fs, const char *path);
            Mount(const Mount &) = delete;
            Mount &operator=(const Mount &) = delete;
            Mount(Mount &&);
            Mount &operator=(Mount &&);
            ~Mount();

            BaseFilesystem *fs;
            char *path;
        };

    public:
        struct Path {
            BaseFilesystem *fs;
            const char *path;
        };

        // mounts a filesystem
        // takes ownership of fs
        // will free with `Hamster::dealloc<BaseFilesystem>(fs)`
        int mount(BaseFilesystem *fs, const char *path);

        int umount(const char *path);

        bool is_valid(const char *path);
        bool is_mounted(const char *path);

        // returns the mountpoint and the remaining path
        Path get_mount(const char *path);

        // Filesystem functions

        using BaseFilesystem::open;
        BaseFile *open(const char *path, int flags, va_list args) override;
        int rename(const char *old_path, const char *new_path) override;
        int unlink(const char *path) override;
        int link(const char *old_path, const char *new_path) override;
        int stat(const char *path, struct ::stat *buf) override;
        int lstat(const char *path, struct ::stat *buf) override;

        BaseRegularFile *mkfile(const char *name, int flags, int mode) override;
        BaseDirectory *mkdir(const char *name, int flags, int mode) override;
        BaseSymbolicFile *mksfile(const char *name, int flags, FileType type, int mode) override;

        int symlink(const char *old_path, const char *new_path) override;
        int readlink(const char *path, char *buf, size_t buf_size) override;
        char *readlink(const char *path) override;

        FileType type(const char *path) override;
    private:
        List<Mount> mounts;
    };

    inline Mounts mounts;
} // namespace Hamster
