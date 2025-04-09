// Mounted filesystems

#pragma once

#include <filesystem/file.hpp>
#include <memory/stl_sequential.hpp>
#include <platform/config.hpp>

namespace Hamster
{
    class Mounts
    {
        Mounts() = default;
        Mounts(const Mounts &) = delete;
        Mounts &operator=(const Mounts &) = delete;
        Mounts(Mounts &&) = delete;
        Mounts &operator=(Mounts &&) = delete;
        ~Mounts() = default;

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

        inline static Mounts &instance()
        {
            static Mounts instance;
            return instance;
        }

        // mounts a filesystem
        // takes ownership of fs
        // will free with `Hamster::dealloc<BaseFilesystem>(fs)`
        int mount(BaseFilesystem *fs, const char *path);

        int umount(const char *path);

        bool is_valid(const char *path);
        bool is_mounted(const char *path);

        // returns the mountpoint and the remaining path
        Path get_mount(const char *path);

    private:
        List<Mount> mounts;
    };
} // namespace Hamster
