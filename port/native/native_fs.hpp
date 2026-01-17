// Native filesystem

#pragma once

#include <errno/errno.h>

#if __has_include ("native_fs.local.hpp")
# include "native_fs.local.hpp"
#endif

// Change this to 1 or create a file 'native_fs.local.hpp' and define it there to enable native filesystem support on Linux
#ifndef LINUX_NATIVE_FS
# define LINUX_NATIVE_FS 0
#endif

#if defined(__linux__) && LINUX_NATIVE_FS

#include <filesystem/base_file.hpp>
#include <memory/allocator.hpp>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <queue>
#include <cstring>
#include <errno.h>
#include <limits.h>
#include <algorithm>

#ifndef HAMSTER_NATIVE_FS_ROOT
# define HAMSTER_NATIVE_FS_ROOT "rootfs"
#endif

namespace Hamster
{
    void swap_error()
    {
        // Swap the error code with the global error code
        int err = Hamster::error;
        Hamster::error = errno;
        errno = err;
    }

    class NativeFileHandle
    {
    public:
        NativeFileHandle(int fd, BaseFilesystem *fs)
            : fd(fd), filesystem(fs) {}
        
        ~NativeFileHandle()
        {
            if (fd >= 0)
            {
                close(fd);
            }
            fd = -1;
            filesystem = nullptr;
        }

        BaseFilesystem *get_filesystem()
        { return filesystem; }

        int get_id() const
        {
            if (fd < 0)
            {
                error = H_EBADF;
                return -1;
            }

            struct stat st;
            if (fstat(fd, &st) < 0)
            {
                swap_error();
                return -1;
            }

            return st.st_ino; // Return inode number as ID
        }

        int stat(sys_stat *buf)
        {
            if (fd < 0)
            {
                error = H_EBADF;
                return -1;
            }

            struct stat st;
            if (fstat(fd, &st) < 0)
            {
                swap_error();
                return -1;
            }

            buf->dev = st.st_dev;
            buf->ino = st.st_ino;
            buf->mode = st.st_mode & 07777;
            buf->nlink = st.st_nlink;
            buf->uid = st.st_uid;
            buf->gid = st.st_gid;
            buf->size = st.st_size;
            buf->atime = st.st_atime;
            buf->mtime = st.st_mtime;
            buf->ctime = st.st_ctime;

            if (S_ISREG(st.st_mode))
            {
                buf->mode |= STAT_IFREG;
            }
            else if (S_ISDIR(st.st_mode))
            {
                buf->mode |= STAT_IFDIR;
            }
            // Skip Character and Block devices
            else if (S_ISFIFO(st.st_mode))
            {
                buf->mode |= STAT_IFIFO;
            }
            else if (S_ISLNK(st.st_mode))
            {
                buf->mode |= STAT_IFLNK;
            }
            else if (S_ISSOCK(st.st_mode))
            {
                buf->mode |= STAT_IFSOCK;
            }

            return 0; // Success
        }

        int get_mode()
        {
            sys_stat st;
            if (stat(&st) < 0)
            {
                return -1;
            }
            return st.mode;
        }

        int get_flags()
        {
            if (fd < 0)
            {
                error = H_EBADF;
                return -1;
            }

            int flags = fcntl(fd, F_GETFL);
            if (flags < 0)
            {
                swap_error();
                return -1;
            }
            return flags;
        }

        int get_uid()
        {
            sys_stat st;
            if (stat(&st) < 0)
            {
                return -1;
            }
            return st.uid;
        }

        int get_gid()
        {
            sys_stat st;
            if (stat(&st) < 0)
            {
                return -1;
            }
            return st.gid;
        }

        int chmod(int mode)
        {
            if (fd < 0)
            {
                error = H_EBADF;
                return -1;
            }

            if (fchmod(fd, mode) < 0)
            {
                swap_error();
                return -1;
            }
            return 0; // Success
        }

        int chown(int uid, int gid)
        {
            if (fd < 0)
            {
                error = H_EBADF;
                return -1;
            }

            if (fchown(fd, uid, gid) < 0)
            {
                swap_error();
                return -1;
            }
            return 0; // Success
        }

        int set_flags(int flags)
        {
            if (fd < 0)
            {
                error = H_EBADF;
                return -1;
            }

            if (fcntl(fd, F_SETFL, flags) < 0)
            {
                swap_error();
                return -1;
            }
            return 0; // Success
        }

        // not actually from basefile, but used to get the protected fd
        int get_fd() const
        {
            return fd;
        }

    protected:
        int fd;
        BaseFilesystem *filesystem;
    };

    class NativeRegularFileHandle : public BaseRegularFile, public NativeFileHandle
    {
    public:
        using NativeFileHandle::NativeFileHandle;

        // Common to all file types

        BaseFile *clone() override
        {
            int new_fd = dup(fd);
            if (new_fd < 0)
            {
                swap_error();
                return nullptr;
            }
            return alloc<NativeRegularFileHandle>(1, new_fd, filesystem);
        }

        BaseFilesystem *get_filesystem() override { return NativeFileHandle::get_filesystem(); }
        int get_id() const override { return NativeFileHandle::get_id(); }
        int stat(sys_stat *buf) override { return NativeFileHandle::stat(buf); }
        int get_mode() override { return NativeFileHandle::get_mode(); }
        int get_flags() override { return NativeFileHandle::get_flags(); }
        int get_uid() override { return NativeFileHandle::get_uid(); }
        int get_gid() override { return NativeFileHandle::get_gid(); }
        int chmod(int mode) override { return NativeFileHandle::chmod(mode); }
        int chown(int uid, int gid) override { return NativeFileHandle::chown(uid, gid); }
        int set_flags(int flags) override { return NativeFileHandle::set_flags(flags); }

        ssize_t pread(uint8_t *buf, size_t size, int64_t offset) override
        {
            ssize_t ret = ::pread(fd, buf, size, offset);
            if (ret < 0)
            {
                swap_error();
                return -1;
            }
            return ret;
        }

        ssize_t pwrite(const uint8_t *buf, size_t size, int64_t offset) override
        {
            ssize_t ret = ::pwrite(fd, buf, size, offset);
            if (ret < 0)
            {
                swap_error();
                return -1;
            }
            return ret;
        }

        int truncate(int64_t size) override
        {
            if (ftruncate(fd, size) < 0)
            {
                swap_error();
                return -1;
            }
            return 0; // Success
        }

        int64_t size() override
        {
            sys_stat st;
            if (stat(&st) < 0)
            {
                return -1;
            }
            return st.size;
        }
    };

    class NativeSpecialFileHandle : public BaseSpecialFile, public NativeFileHandle
    {
    public:
        using NativeFileHandle::NativeFileHandle;

        BaseFile *clone() override
        {
            int new_fd = dup(fd);
            if (new_fd < 0)
            {
                swap_error();
                return nullptr;
            }
            return alloc<NativeSpecialFileHandle>(1, new_fd, filesystem);
        }

        BaseFilesystem *get_filesystem() override { return NativeFileHandle::get_filesystem(); }
        int get_id() const override { return NativeFileHandle::get_id(); }
        int stat(sys_stat *buf) override { return NativeFileHandle::stat(buf); }
        int get_mode() override { return NativeFileHandle::get_mode(); }
        int get_flags() override { return NativeFileHandle::get_flags(); }
        int get_uid() override { return NativeFileHandle::get_uid(); }
        int get_gid() override { return NativeFileHandle::get_gid(); }
        int chmod(int mode) override { return NativeFileHandle::chmod(mode); }
        int chown(int uid, int gid) override { return NativeFileHandle::chown(uid, gid); }
        int set_flags(int flags) override { return NativeFileHandle::set_flags(flags); }

        DeviceID get_device_id() override
        {
            sys_stat st = {};
            if (stat(&st) < 0)
            {
                return {0, 0}; // Return invalid device ID on error
            }
            return {static_cast<uint32_t>(st.rdev >> 20), static_cast<uint32_t>(st.rdev & 0xFFFFF)};
        }
    };

    class NativeSymlinkHandle : public BaseSymlink, public NativeFileHandle
    {
    public:
        using NativeFileHandle::NativeFileHandle;

        BaseFile *clone() override
        {
            int new_fd = dup(fd);
            if (new_fd < 0)
            {
                swap_error();
                return nullptr;
            }
            return alloc<NativeSymlinkHandle>(1, new_fd, filesystem);
        }

        BaseFilesystem *get_filesystem() override { return NativeFileHandle::get_filesystem(); }
        int get_id() const override { return NativeFileHandle::get_id(); }
        int stat(sys_stat *buf) override { return NativeFileHandle::stat(buf); }
        int get_mode() override { return NativeFileHandle::get_mode(); }
        int get_flags() override { return NativeFileHandle::get_flags(); }
        int get_uid() override { return NativeFileHandle::get_uid(); }
        int get_gid() override { return NativeFileHandle::get_gid(); }
        int chmod(int mode) override { return NativeFileHandle::chmod(mode); }
        int chown(int uid, int gid) override { return NativeFileHandle::chown(uid, gid); }
        int set_flags(int flags) override { return NativeFileHandle::set_flags(flags); }

        char *get_target() override
        {
            if (fd < 0)
            {
                error = H_EBADF;
                return nullptr;
            }

            char target[PATH_MAX];
            ssize_t len = readlinkat(fd, "", target, sizeof(target) - 1);
            if (len < 0)
            {
                swap_error();
                return nullptr;
            }

            target[len] = '\0'; // Null-terminate the string
            char *result = alloc<char>(len + 1);
            strcpy(result, target);
            return result;
        }

        int set_target(const char *target) override
        {
            // TODO: change the target of the symlink
            error = H_ENOSYS;
            return -1; // Not implemented
        }
    };

    class NativeDirectoryHandle : public BaseDirectory, public NativeFileHandle
    {
    public:
        using NativeFileHandle::NativeFileHandle;

        BaseFile *clone() override
        {
            int new_fd = dup(fd);
            if (new_fd < 0)
            {
                swap_error();
                return nullptr;
            }
            return alloc<NativeDirectoryHandle>(1, new_fd, filesystem);
        }

        BaseFilesystem *get_filesystem() override { return NativeFileHandle::get_filesystem(); }
        int get_id() const override { return NativeFileHandle::get_id(); }
        int stat(sys_stat *buf) override { return NativeFileHandle::stat(buf); }
        int get_mode() override { return NativeFileHandle::get_mode(); }
        int get_flags() override { return NativeFileHandle::get_flags(); }
        int get_uid() override { return NativeFileHandle::get_uid(); }
        int get_gid() override { return NativeFileHandle::get_gid(); }
        int chmod(int mode) override { return NativeFileHandle::chmod(mode); }
        int chown(int uid, int gid) override { return NativeFileHandle::chown(uid, gid); }
        int set_flags(int flags) override { return NativeFileHandle::set_flags(flags); }

        char * const *list(size_t count) override
        {
            if (fd < 0)
            {
                error = H_EBADF;
                return nullptr;
            }

            int dup_fd = dup(fd);
            DIR *dir = fdopendir(dup_fd);
            if (!dir)
            {
                swap_error();
                return nullptr;
            }

            rewinddir(dir); // Reset the directory stream

            std::vector<char *> entries;
            struct dirent *entry;

            while ((entry = readdir(dir)) != nullptr)
            {
                if (count == 0)
                    break;
                const char *name = entry->d_name;
                size_t len = strlen(name);
                char *name_copy = alloc<char>(len + 1);
                strcpy(name_copy, name);
                entries.push_back(name_copy);

                --count;
            }

            closedir(dir);

            // Sort entries alphabetically to maintain consistent outputs
            // when called multiple times
            std::sort(entries.begin(), entries.end(), [](const char *a, const char *b) {
                return std::strcmp(a, b) < 0;
            });

            char **result = alloc<char *>(entries.size() + 1);
            result[entries.size()] = nullptr; // Null-terminate the array
            memcpy(result, entries.data(), entries.size() * sizeof(char *));
            return result;
        }

        BaseFile *get(const char *name, int flags, int mode) override
        {
            if (fd < 0)
            {
                error = H_EBADF;
                return nullptr;
            }

            int new_fd = openat(fd, name, O_RDWR | O_NONBLOCK | O_NOFOLLOW |
                                          (flags & OPEN_APPEND ? O_APPEND : 0) |
                                          (flags & OPEN_TRUNC ? O_TRUNC : 0));
            if (new_fd < 0)
            {
                if (errno == ENOENT && (flags & OPEN_CREAT))
                {
                    return flags & OPEN_DIRECTORY ?
                        (BaseFile*)mkdir(name, O_RDWR | O_NONBLOCK, mode) :
                        (BaseFile*)mkfile(name, O_RDWR | O_NONBLOCK, mode);
                }
                else if (errno == ELOOP)
                {
                    new_fd = openat(fd, name, O_PATH | O_NOFOLLOW);
                    if (new_fd < 0)
                    {
                        swap_error();
                        return nullptr;
                    }
                }
                else if (errno == EISDIR)
                {
                    new_fd = openat(fd, name, O_RDONLY | O_DIRECTORY);
                    if (new_fd < 0)
                    {
                        swap_error();
                        return nullptr;
                    }
                }
                else
                {
                    swap_error();
                    return nullptr;
                }
            }
            struct stat st;
            if (fstat(new_fd, &st) < 0)
            {
                swap_error();
                close(new_fd);
                return nullptr;
            }

            BaseFile *file = nullptr;

            if ((flags & OPEN_DIRECTORY) && !S_ISDIR(st.st_mode))
            {
                error = H_ENOTDIR;
                return nullptr;
            }

            if (S_ISREG(st.st_mode))
            {
                file = alloc<NativeRegularFileHandle>(1, new_fd, filesystem);
            }
            else if (S_ISDIR(st.st_mode))
            {
                file = alloc<NativeDirectoryHandle>(1, new_fd, filesystem);
            }
            else if (S_ISLNK(st.st_mode))
            {
                file = alloc<NativeSymlinkHandle>(1, new_fd, filesystem);
            }
            else if (S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode))
            {
                file = alloc<NativeSpecialFileHandle>(1, new_fd, filesystem);
            }
            else
            {
                close(new_fd);
                error = H_ENOSYS; // Unsupported file type
                return nullptr;
            }

            return file;
        }

        BaseRegularFile *mkfile(const char *name, int flags, int mode) override
        {
            if (fd < 0)
            {
                error = H_EBADF;
                return nullptr;
            }

            int new_fd = openat(fd, name, O_RDWR | O_NONBLOCK | O_CREAT | O_EXCL, mode);

            if (new_fd < 0)
            {
                swap_error();
                return nullptr;
            }
            
            return alloc<NativeRegularFileHandle>(1, new_fd, filesystem);
        }

        BaseDirectory *mkdir(const char *name, int flags, int mode) override
        {
            if (fd < 0)
            {
                error = H_EBADF;
                return nullptr;
            }

            if (mkdirat(fd, name, mode) < 0)
            {
                swap_error();
                return nullptr;
            }

            int new_fd = openat(fd, name, O_RDONLY | O_DIRECTORY, mode);
            if (new_fd < 0)
            {
                swap_error();
                return nullptr;
            }

            return alloc<NativeDirectoryHandle>(1, new_fd, filesystem);
        }

        BaseSymlink *mksym(const char *name, const char *target) override
        {
            if (fd < 0)
            {
                error = H_EBADF;
                return nullptr;
            }

            int ret = symlinkat(target, fd, name);
            if (ret < 0)
            {
                swap_error();
                return nullptr;
            }

            int new_fd = openat(fd, name, O_PATH | O_NOFOLLOW);
            if (new_fd < 0)
            {
                swap_error();
                return nullptr;
            }

            return alloc<NativeSymlinkHandle>(1, new_fd, filesystem);
        }

        BaseSpecialFile *mksfile(const char *name, int flags, DeviceID device_id, int mode) override
        {
            if (fd < 0)
            {
                error = H_EBADF;
                return nullptr;
            }

            // Create a special file (character or block device)
            int new_fd = mknodat(fd, name, mode, device_id.major << 20 | (device_id.minor & 0xFFFFF));
            if (new_fd < 0)
            {
                swap_error();
                return nullptr;
            }

            return alloc<NativeSpecialFileHandle>(1, new_fd, filesystem);
        }

        int link(BaseFile *file, const char *name) override
        {
            if (fd < 0)
            {
                error = H_EBADF;
                return -1;
            }

            if (get_filesystem() != file->get_filesystem())
            {
                error = H_EXDEV; // Cross-device link not permitted
                return -1;
            }

            int file_fd = -1;
            switch (file->type())
            {
            case FileType::Regular:
                file_fd = ((NativeRegularFileHandle *)file)->get_fd();
                break;
            case FileType::Directory:
                file_fd = ((NativeDirectoryHandle *)file)->get_fd();
                break;
            case FileType::Symlink:
                file_fd = ((NativeSymlinkHandle *)file)->get_fd();
                break;
            case FileType::Special:
                file_fd = ((NativeSpecialFileHandle *)file)->get_fd();
                break;
            default:
                error = H_EBADF; // Invalid file type for linking
                return -1;
            }

            if (file_fd < 0)
            {
                error = H_EBADF; // Bad file descriptor
                return -1;
            }

            int ret = linkat(file_fd, "", fd, name, AT_EMPTY_PATH);

            if (ret < 0)
            {
                swap_error();
                return -1;
            }

            return 0; // Success
        }

        int remove(const char *name) override
        {
            if (fd < 0)
            {
                error = H_EBADF;
                return -1;
            }

            if (unlinkat(fd, name, 0) == 0)
                return 0;
            if (unlinkat(fd, name, AT_REMOVEDIR) == 0)
                return 0;
            swap_error();
            return -1;
        }
    };

    class NativeFilesystem : public BaseFilesystem
    {
    public:
        const char *root_start = HAMSTER_NATIVE_FS_ROOT;

        BaseDirectory *open_root(int flags) override
        {
            int fd = open(root_start, O_RDONLY | O_DIRECTORY);
            if (fd < 0)
            {
                swap_error();
                return nullptr;
            }

            return alloc<NativeDirectoryHandle>(1, fd, this);
        }
    };
} // namespace Hamster

#else // defined(__linux__) && X

#include <filesystem/base_romfs.hpp>
#include <fcntl.h>

#ifndef HAMSTER_ROOT_IMG_LOC
# define HAMSTER_ROOT_IMG_LOC "rootfs.img"
#endif

namespace Hamster
{
    class NativeDisk
    {
    public:
        NativeDisk()
        {
            fd = open(HAMSTER_ROOT_IMG_LOC, O_RDONLY);
        }
        ~NativeDisk()
        {
            close(fd);
        }

        ssize_t read(uint32_t loc, void *buf, size_t len)
        {
            lseek(fd, loc, SEEK_SET);
            return ::read(fd, buf, len);
        }
    private:
        int fd;
    };

    using NativeFilesystem = BaseRomFs<NativeDisk>;
} // namespace Hamster


#endif // defined(__linux__) && X

