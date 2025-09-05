// Native filesystem

#pragma once

#include <errno/errno.h>

#if defined(__linux__) && 0

#include <filesystem/base_file.hpp>
#include <memory/allocator.hpp>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <queue>
#include <cstring>

#define HAMSTER_NATIVE_FS_ROOT "/home/tin/hamster_rootfs"

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
                error = EBADF;
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
                error = EBADF;
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
                error = EBADF;
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
                error = EBADF;
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
                error = EBADF;
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
                error = EBADF;
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

        ssize_t read(uint8_t *buf, size_t size) override
        {
            ssize_t ret = ::read(fd, buf, size);
            if (ret < 0)
            {
                swap_error();
                return -1;
            }
            return ret;
        }

        ssize_t write(const uint8_t *buf, size_t size) override
        {
            ssize_t ret = ::write(fd, buf, size);
            if (ret < 0)
            {
                swap_error();
                return -1;
            }
            return ret;
        }

        int64_t seek(int64_t offset, int whence) override
        {
            off_t ret = lseek(fd, offset, whence);
            if (ret < 0)
            {
                swap_error();
                return -1;
            }
            return ret;
        }

        int64_t tell() override
        {
            off_t ret = lseek(fd, 0, SEEK_CUR);
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
                error = EBADF;
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
            error = ENOSYS;
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
                error = EBADF;
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

            std::queue<char *> entries;
            struct dirent *entry;

            // Go to position in directory
            for (int64_t i = 0; i < pos && (entry = readdir(dir)) != nullptr; ++i)
            {
                // Just read entries until we reach the desired position
            }

            while ((entry = readdir(dir)) != nullptr)
            {
                if (count == 0)
                    break;
                const char *name = entry->d_name;
                size_t len = strlen(name);
                char *name_copy = alloc<char>(len + 1);
                strcpy(name_copy, name);
                entries.push(name_copy);

                ++pos;
                --count;
            }

            closedir(dir);

            char **result = alloc<char *>(entries.size() + 1);
            result[entries.size()] = nullptr; // Null-terminate the array

            size_t i = 0;
            while (!entries.empty())
            {
                result[i++] = entries.front();
                entries.pop();
            }

            return result;
        }

        int64_t seek(int64_t offset, int whence) override
        {
            switch (whence)
            {
            case H_SEEK_SET:
                if (offset < 0)
                {
                    error = EINVAL;
                    return -1; // Invalid offset
                }
                pos = offset;
                break;
            case H_SEEK_CUR:
                if (pos + offset < 0)
                {
                    error = EINVAL;
                    return -1; // Invalid offset
                }
                pos += offset;
                break;
            case H_SEEK_END:
                error = ENOTSUP;
                return -1; // Not supported for directories
            default:
                error = EINVAL;
                return -1; // Invalid whence
            }

            return pos;
        }

        int64_t tell() override
        {
            return pos;
        }

        BaseFile *get(const char *name, int flags, int mode) override
        {
            if (fd < 0)
            {
                error = EBADF;
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
                error = ENOTDIR;
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
                error = ENOSYS; // Unsupported file type
                return nullptr;
            }

            return file;
        }

        BaseRegularFile *mkfile(const char *name, int flags, int mode) override
        {
            if (fd < 0)
            {
                error = EBADF;
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
                error = EBADF;
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
                error = EBADF;
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
                error = EBADF;
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
                error = EBADF;
                return -1;
            }

            if (get_filesystem() != file->get_filesystem())
            {
                error = EXDEV; // Cross-device link not permitted
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
                error = EBADF; // Invalid file type for linking
                return -1;
            }

            if (file_fd < 0)
            {
                error = EBADF; // Bad file descriptor
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
                error = EBADF;
                return -1;
            }

            if (unlinkat(fd, name, 0) == 0)
                return 0;
            if (unlinkat(fd, name, AT_REMOVEDIR) == 0)
                return 0;
            swap_error();
            return -1;
        }
    private:
        off_t pos = 0;
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

#include <driver/base_romfs.hpp>
#include <fcntl.h>

#define HAMSTER_ROOT_IMG_LOC "/home/tin/hroot.romfs"

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

