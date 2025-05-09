// Hamster VFS

#pragma once

#include <filesystem/base_file.hpp>

namespace Hamster
{
    class VFSData;

    class VFS
    {
    public:
        // move-only
    
        VFS();
        ~VFS();
        VFS(const VFS &) = delete;
        VFS &operator=(const VFS &) = delete;
        VFS(VFS &&);
        VFS &operator=(VFS &&);    

        /**
         * @brief Mount a filesystem at a given path
         * @param path The path to mount the filesystem at
         * @param fs The filesystem to mount
         * @return 0 on success, or on error return -1 and set `error`
         * @note Takes ownership of `fs`, and will deallocate it later
         */
        int mount(const char *path, BaseFilesystem *fs);

        /**
         * @brief Unmount a filesystem at a given path
         * @param path The path to unmount the filesystem from
         * @return 0 on success, or on error return -1 and set `error`
         */
        int unmount(const char *path);

        /* POSIX and POSIX-like functions */
        /* These all return -1 and set `error` on error */

        int open(const char *path, int flags, int mode = 0);
        int close(int fd);

        // Note that this version cannot move the file, but only rename it
        int rename(int fd, const char *new_name);

        // note that this is not the same as opening a file and then renaming it
        // as this can also move files between filesystems
        int rename(const char *old_path, const char *new_path);

        int remove(int fd);

        int stat(int fd, struct ::stat *buf);

        // note that this is not the same as opening a file and then statting it
        // as opening will follow a symlink, while this will not
        int lstat(const char *path, struct ::stat *buf);

        int get_mode(int fd);

        int get_flags(int fd);

        int get_uid(int fd);

        int get_gid(int fd);

        int chmod(int fd, int mode);

        int chown(int fd, int uid, int gid);

        // `basename`s return nullptr instead of -1 on error, but still set `error`

        char *basename(int fd);

        ssize_t read(int fd, uint8_t *buf, size_t size);
        ssize_t write(int fd, const uint8_t *buf, size_t size);
        int seek(int fd, int64_t offset, int whence);
        int64_t tell(int fd);
        int truncate(int fd, int64_t size);
        int64_t size(int fd);

        // `get_target` returns nullptr instead of -1 on error, but still sets `error`

        char *get_target(int fd);

        int set_target(int fd, const char *target);

        // `list` returns nullptr instead of -1 on error, but still sets `error`

        char * const *list(int fd);

        int openat(int dir, const char *path, int flags, int mode = 0);

        int mkfile(const char *path, int flags, int mode);
        int mkfileat(int dir, const char *path, int flags, int mode);

        int mkdir(const char *path, int flags, int mode);
        int mkdirat(int dir, const char *path, int flags, int mode);

        int symlink(const char *path, const char *target);
        int symlinkat(int dir, const char *path, const char *target);

    private:
        VFSData *data;
    };
} // namespace Hamster

