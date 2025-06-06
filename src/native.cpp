// Native version

#if !defined(ARDUINO) && 1

#include <platform/platform.hpp>
#include <filesystem/base_file.hpp>
#include <filesystem/vfs.hpp>
#include <memory/stl_sequential.hpp>
#include <memory/stl_map.hpp>
#include <memory/allocator.hpp>
#include <errno/errno.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <cerrno>

using namespace Hamster;

#define HAMSTER_NATIVE_FS_ROOT "/home/tin/hamster_rootfs"

/* Filesystem Implementation using POSIX utils */
namespace
{
    UnorderedMap<ino_t, uint32_t> vfs_flags;

    class NativeFile
    {
    public:
        NativeFile(const String &path, int flags) : path(path), flags(flags) {}
        virtual ~NativeFile() = default;

        int rename(const char *newname)
        {
            const char *path = this->path.c_str();
            const char *last = strrchr(path, '/');
            if (!last)
            {
                int ret = ::rename(path, newname);
                if (ret < 0)
                {
                    Hamster::error = errno;
                    errno = 0;
                    return ret;
                }
                this->path = newname;
                return 0;
            }
            else
            {
                String new_path = String(path, last - path) + "/" + newname;
                int ret = ::rename(path, new_path.c_str());
                if (ret < 0)
                {
                    Hamster::error = errno;
                    errno = 0;
                    return ret;
                }
                this->path = new_path;
                return 0;
            }
        }

        int remove()
        {
            int ret = ::unlink(path.c_str());
            if (ret < 0)
            {
                Hamster::error = errno;
                errno = 0;
            }
            return ret;
        }

        int stat(struct ::stat *buf)
        {
            // lstat instead of stat to handle symlinks correctly
            int ret = ::lstat(path.c_str(), buf);
            if (ret < 0)
            {
                Hamster::error = errno;
                errno = 0;
            }
            return ret;
        }

        int get_mode()
        {
            struct ::stat buf;
            if (stat(&buf) < 0)
                return -1;
            return buf.st_mode;
        }

        int get_flags()
        {
            return flags;
        }

        int get_uid()
        {
            struct ::stat buf;
            if (stat(&buf) < 0)
                return -1;
            return buf.st_uid;
        }

        int get_gid()
        {
            struct ::stat buf;
            if (stat(&buf) < 0)
                return -1;
            return buf.st_gid;
        }

        int chmod(int mode)
        {
            int ret = ::chmod(path.c_str(), mode);
            if (ret < 0)
            {
                Hamster::error = errno;
                errno = 0;
            }
            return ret;
        }

        int chown(int uid, int gid)
        {
            int ret = ::chown(path.c_str(), uid, gid);
            if (ret < 0)
            {
                Hamster::error = errno;
                errno = 0;
            }
            return ret;
        }

        int set_flags(int flags)
        {
            this->flags = flags;
            return 0;
        }

        char *basename()
        {
            const char *path = this->path.c_str();
            const char *last = strrchr(path, '/');
            if (!last)
            {
                size_t len = strlen(path);
                char *name = alloc<char>(len + 1);
                strncpy(name, path, len);
                name[len] = '\0';
                return name;
            }
            else
            {
                size_t len = strlen(last + 1);
                char *name = alloc<char>(len + 1);
                strncpy(name, last + 1, len);
                name[len] = '\0';
                return name;
            }
        }

        uint32_t get_vfs_flags()
        {
            struct ::stat buf;
            if (stat(&buf) < 0)
                return 0;
            ino_t inode = buf.st_ino;
            return vfs_flags[inode];
        }

        int set_vfs_flags(uint32_t flags)
        {
            struct ::stat buf;
            if (stat(&buf) < 0)
                return -1;
            ino_t inode = buf.st_ino;
            vfs_flags[inode] = flags;
            return 0;
        }

    protected:
        String path;
        int flags;
    };

    class NativeRegularFile : public BaseRegularFile, private NativeFile
    {
    public:
        using NativeFile::NativeFile;
        ~NativeRegularFile() override = default;

        // Common to all files

        BaseFile *clone() override { return alloc<NativeRegularFile>(1, path, flags); }
        int rename(const char *new_name) override { return NativeFile::rename(new_name); }
        int remove() override { return NativeFile::remove(); }
        int stat(struct ::stat *buf) override { return NativeFile::stat(buf); }
        int get_mode() override { return NativeFile::get_mode(); }
        int get_flags() override { return NativeFile::get_flags(); }
        int get_uid() override { return NativeFile::get_uid(); }
        int get_gid() override { return NativeFile::get_gid(); }
        int chmod(int mode) override { return NativeFile::chmod(mode); }
        int chown(int uid, int gid) override { return NativeFile::chown(uid, gid); }
        int set_flags(int flags) override { return NativeFile::set_flags(flags); }
        char *basename() override { return NativeFile::basename(); }
        int set_vfs_flags(uint32_t flags) override { return NativeFile::set_vfs_flags(flags); }
        uint32_t get_vfs_flags() override { return NativeFile::get_vfs_flags(); }

        // Regular file specific methods

        ssize_t read(uint8_t *buf, size_t size) override
        {
            int fd = ::open(path.c_str(), flags);
            if (fd < 0)
            {
                Hamster::error = errno;
                errno = 0;
                return -1;
            }
            if (::lseek(fd, offset, SEEK_SET) < 0)
            {
                Hamster::error = EIO;
                errno = 0;
                ::close(fd);
                return -1;
            }
            ssize_t ret = ::read(fd, buf, size);
            if (ret < 0)
            {
                Hamster::error = errno;
                errno = 0;
            }
            ::close(fd);
            offset += ret;
            return ret;
        }

        ssize_t write(const uint8_t *buf, size_t size) override
        {
            int fd = ::open(path.c_str(), flags);
            if (fd < 0)
            {
                Hamster::error = errno;
                errno = 0;
                return -1;
            }
            if (::lseek(fd, offset, SEEK_SET) < 0)
            {
                Hamster::error = EIO;
                errno = 0;
                ::close(fd);
                return -1;
            }
            ssize_t ret = ::write(fd, buf, size);
            if (ret < 0)
            {
                Hamster::error = errno;
                errno = 0;
            }
            ::close(fd);
            offset += ret;
            return ret;
        }

        int64_t seek(int64_t offset, int whence) override
        {
            switch (whence)
            {
            case SEEK_SET:
                if (offset < 0)
                {
                    Hamster::error = EINVAL;
                    return -1;
                }
                this->offset = offset;
                break;
            case SEEK_CUR:
                if (this->offset + offset < 0)
                {
                    Hamster::error = EINVAL;
                    return -1;
                }
                this->offset += offset;
                break;
            case SEEK_END:
            {
                struct ::stat buf;
                if (stat(&buf) < 0)
                {
                    Hamster::error = errno;
                    errno = 0;
                    return -1;
                }
                if (buf.st_size + offset < 0)
                {
                    Hamster::error = EINVAL;
                    return -1;
                }
                this->offset = buf.st_size + offset;
                break;
            }
            default:
                Hamster::error = EINVAL;
                return -1;
            }
            return this->offset;
        }

        int64_t tell() override
        {
            return this->offset;
        }

        int truncate(int64_t size) override
        {
            if (size < 0)
            {
                Hamster::error = EINVAL;
                return -1;
            }
            int fd = ::open(path.c_str(), flags);
            if (fd < 0)
            {
                Hamster::error = errno;
                errno = 0;
                return -1;
            }
            if (::ftruncate(fd, size) < 0)
            {
                Hamster::error = errno;
                errno = 0;
                ::close(fd);
                return -1;
            }
            ::close(fd);
            return 0;
        }

        int64_t size() override
        {
            struct ::stat buf;
            if (stat(&buf) < 0)
            {
                Hamster::error = errno;
                errno = 0;
                return -1;
            }
            return buf.st_size;
        }

    private:
        int64_t offset = 0;
    };

    UnorderedMap<ino_t, int> special_dev_ids;

    class NativeSpecialFile : public BaseSpecialFile, private NativeFile
    {
    public:
        using NativeFile::NativeFile;
        ~NativeSpecialFile() override = default;

        BaseFile *clone() override { return alloc<NativeSpecialFile>(1, path, flags); }

        // Common to all files

        int rename(const char *new_name) override { return NativeFile::rename(new_name); }
        int remove() override { return NativeFile::remove(); }
        int stat(struct ::stat *buf) override { return NativeFile::stat(buf); }
        int get_mode() override { return NativeFile::get_mode(); }
        int get_flags() override { return NativeFile::get_flags(); }
        int get_uid() override { return NativeFile::get_uid(); }
        int get_gid() override { return NativeFile::get_gid(); }
        int chmod(int mode) override { return NativeFile::chmod(mode); }
        int chown(int uid, int gid) override { return NativeFile::chown(uid, gid); }
        int set_flags(int flags) override { return NativeFile::set_flags(flags); }
        char *basename() override { return NativeFile::basename(); }
        int set_vfs_flags(uint32_t flags) override { return NativeFile::set_vfs_flags(flags); }
        uint32_t get_vfs_flags() override { return NativeFile::get_vfs_flags(); }

        // Special file specific methods

        int get_device_id() override
        {
            struct ::stat buf;
            if (stat(&buf) < 0)
            {
                Hamster::error = errno;
                errno = 0;
                return -1;
            }
            ino_t inode = buf.st_ino;
            if (special_dev_ids.contains(inode))
                return special_dev_ids[inode];
            else
            {
                Hamster::error = ENODEV;
                return -1;
            }
        }
    };

    class NativeSymlink : public BaseSymlink, private NativeFile
    {
    public:
        using NativeFile::NativeFile;
        ~NativeSymlink() override = default;

        // Common to all files

        BaseFile *clone() override { return alloc<NativeRegularFile>(1, path, flags); }
        int rename(const char *new_name) override { return NativeFile::rename(new_name); }
        int remove() override { return NativeFile::remove(); }
        int stat(struct ::stat *buf) override { return NativeFile::stat(buf); }
        int get_mode() override { return NativeFile::get_mode(); }
        int get_flags() override { return NativeFile::get_flags(); }
        int get_uid() override { return NativeFile::get_uid(); }
        int get_gid() override { return NativeFile::get_gid(); }
        int chmod(int mode) override { return NativeFile::chmod(mode); }
        int chown(int uid, int gid) override { return NativeFile::chown(uid, gid); }
        int set_flags(int flags) override { return NativeFile::set_flags(flags); }
        char *basename() override { return NativeFile::basename(); }
        int set_vfs_flags(uint32_t flags) override { return NativeFile::set_vfs_flags(flags); }
        uint32_t get_vfs_flags() override { return NativeFile::get_vfs_flags(); }

        // Symlink specific methods

        char *get_target() override
        {
            // get the length, with stat
            struct ::stat buf;
            if (stat(&buf) < 0)
            {
                Hamster::error = errno;
                errno = 0;
                return nullptr;
            }
            size_t len = buf.st_size;

            char *target = alloc<char>(len + 1);
            ssize_t ret = ::readlink(path.c_str(), target, len);
            if (ret < 0)
            {
                dealloc(target);
                Hamster::error = errno;
                errno = 0;
                return nullptr;
            }
            target[ret] = '\0'; // null-terminate the string
            return target;
        };

        int set_target(const char *target) override
        {
            // unlink the old symlink
            if (remove() < 0)
                return -1;
            // create a new symlink
            int ret = ::symlink(target, path.c_str());
            if (ret < 0)
            {
                Hamster::error = errno;
                errno = 0;
                return -1;
            }
            return 0;
        }
    };

    class NativeDirectory : public BaseDirectory, private NativeFile
    {
    public:
        using NativeFile::NativeFile;
        ~NativeDirectory() override = default;

        // Common to all files

        BaseFile *clone() override { return alloc<NativeDirectory>(1, path, flags); }

        int rename(const char *new_name) override { return NativeFile::rename(new_name); }
        int remove() override { return NativeFile::remove(); }
        int stat(struct ::stat *buf) override { return NativeFile::stat(buf); }
        int get_mode() override { return NativeFile::get_mode(); }
        int get_flags() override { return NativeFile::get_flags(); }
        int get_uid() override { return NativeFile::get_uid(); }
        int get_gid() override { return NativeFile::get_gid(); }
        int chmod(int mode) override { return NativeFile::chmod(mode); }
        int chown(int uid, int gid) override { return NativeFile::chown(uid, gid); }
        int set_flags(int flags) override { return NativeFile::set_flags(flags); }
        char *basename() override { return NativeFile::basename(); }
        int set_vfs_flags(uint32_t flags) override { return NativeFile::set_vfs_flags(flags); }
        uint32_t get_vfs_flags() override { return NativeFile::get_vfs_flags(); }

        // Directory specific methods

        char *const *list() override
        {
            DIR *dir = ::opendir(path.c_str());
            if (!dir)
            {
                Hamster::error = errno;
                errno = 0;
                return nullptr;
            }

            size_t count = 0;
            struct dirent *entry;
            while ((entry = ::readdir(dir)) != nullptr)
            {
                if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
                    count++;
            }
            ::rewinddir(dir); // Reset the directory stream to read again

            char **names = alloc<char *>(count + 1);
            names[count] = nullptr; // Null-terminate the array

            while ((entry = ::readdir(dir)) != nullptr)
            {
                if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
                {
                    size_t len = strlen(entry->d_name);
                    names[count - 1] = alloc<char>(len + 1);
                    strncpy(names[count - 1], entry->d_name, len);
                    names[count - 1][len] = '\0'; // Null-terminate the string
                    count--;
                }
            }

            // clean up
            ::closedir(dir);

            if (count != 0)
            {
                // If we didn't fill all the slots, deallocate the remaining ones
                for (size_t i = 0; i < count; ++i)
                {
                    dealloc(names[i]);
                    names[i] = nullptr;
                }
            }

            return names;
        }

        BaseFile *get(const char *name, int flags, int mode = 0) override
        {
            String full_path = path + "/" + name;
            
            // Check if the file exists
            struct ::stat buf;
            if (::lstat(full_path.c_str(), &buf) < 0)
            {
                // File doesn't exist, create as either regular or directory

                if ((flags & O_CREAT) == 0)
                {
                    Hamster::error = ENOENT;
                    errno = 0;
                    return nullptr; // File doesn't exist and not creating
                }

                if (flags & O_DIRECTORY)
                {
                    // mkdir
                    int ret = ::mkdir(full_path.c_str(), mode);
                    if (ret < 0)
                    {
                        Hamster::error = errno;
                        errno = 0;
                        return nullptr; // Failed to create directory
                    }
                }
                else
                {
                    // Create a regular file
                    int fd = ::open(full_path.c_str(), flags | O_CREAT, mode);
                    if (fd < 0)
                    {
                        Hamster::error = errno;
                        errno = 0;
                        return nullptr; // Failed to create file
                    }
                    ::close(fd);
                }

                // re-stat
                if (::lstat(full_path.c_str(), &buf) < 0)
                {
                    Hamster::error = errno;
                    errno = 0;
                    return nullptr; // Failed to stat newly created file
                }
            }

            if (S_ISREG(buf.st_mode))
            {
                // Regular file
                return alloc<NativeRegularFile>(1, full_path, flags);
            }
            else if (S_ISDIR(buf.st_mode))
            {
                // Directory
                return alloc<NativeDirectory>(1, full_path, flags);
            }
            else if (S_ISLNK(buf.st_mode))
            {
                // Symlink
                return alloc<NativeSymlink>(1, full_path, flags);
            }
            else if (S_ISCHR(buf.st_mode) || S_ISBLK(buf.st_mode) || S_ISFIFO(buf.st_mode) || S_ISSOCK(buf.st_mode))
            {
                // Special file
                return alloc<NativeSpecialFile>(1, full_path, flags);
            }
            else
            {
                Hamster::error = ENOTSUP;
                errno = 0;
                return nullptr; // Unsupported file type
            }
        }

        BaseRegularFile *mkfile(const char *name, int flags, int mode) override
        {
            if (strchr(name, '/') != nullptr)
            {
                Hamster::error = EINVAL;
                return nullptr; // Invalid name, cannot contain slashes
            }
            String full_path = path + "/" + name;

            // Create a regular file
            int fd = ::open(full_path.c_str(), flags | O_CREAT, mode);
            if (fd < 0)
            {
                Hamster::error = errno;
                errno = 0;
                return nullptr; // Failed to create file
            }
            ::close(fd);

            return alloc<NativeRegularFile>(1, full_path, flags);
        }

        BaseDirectory *mkdir(const char *name, int flags, int mode) override
        {
            if (strchr(name, '/') != nullptr)
            {
                Hamster::error = EINVAL;
                return nullptr; // Invalid name, cannot contain slashes
            }
            String full_path = path + "/" + name;

            // Create a directory
            int ret = ::mkdir(full_path.c_str(), mode);
            if (ret < 0)
            {
                Hamster::error = errno;
                errno = 0;
                return nullptr; // Failed to create directory
            }

            return alloc<NativeDirectory>(1, full_path, flags);
        }

        BaseSymlink *mksym(const char *name, const char *target) override
        {
            if (strchr(name, '/') != nullptr)
            {
                Hamster::error = EINVAL;
                return nullptr; // Invalid name, cannot contain slashes
            }
            String full_path = path + "/" + name;

            // Create a symlink
            int ret = ::symlink(target, full_path.c_str());
            if (ret < 0)
            {
                Hamster::error = errno;
                errno = 0;
                return nullptr; // Failed to create symlink
            }

            return alloc<NativeSymlink>(1, full_path, flags);
        }

        BaseSpecialFile *mksfile(const char *name, int flags, int type, int mode)
        {
            if (strchr(name, '/') != nullptr)
            {
                Hamster::error = EINVAL;
                return nullptr; // Invalid name, cannot contain slashes
            }
            String full_path = path + "/" + name;

            // Create a FIFO as a marker
            int ret = ::mknod(full_path.c_str(), (mode & 0777) | S_IFIFO, 0);
            if (ret < 0)
            {
                Hamster::error = errno;
                errno = 0;
                return nullptr; // Failed to create special file
            }

            struct ::stat buf;
            if (::lstat(full_path.c_str(), &buf) < 0)
            {
                Hamster::error = errno;
                errno = 0;
                return nullptr; // Failed to stat newly created file
            }

            assert(S_ISFIFO(buf.st_mode) && "mknod did not create a FIFO");
            ino_t inode = buf.st_ino;

            special_dev_ids[inode] = type; // Store the device ID for this special file

            // Now we can return a NativeSpecialFile
            return alloc<NativeSpecialFile>(1, full_path, flags);
        }

        int remove(const char *name) override
        {
            String full_path = path + "/" + name;

            int ret = ::remove(full_path.c_str());
            if (ret < 0)
            {
                Hamster::error = errno;
                errno = 0;
                return -1; // Failed to remove file or directory
            }
            return 0;
        }
    };

    class NativeFilesystem : public BaseFilesystem
    {
    public:
        BaseDirectory *open_root(int flags) override
        {
            // Ensure that the root directory exists
            if (::mkdir(HAMSTER_NATIVE_FS_ROOT, 0755) < 0 && errno != EEXIST)
            {
                Hamster::error = errno;
                errno = 0;
                return nullptr; // Failed to create root directory
            }
            errno = 0;

            return alloc<NativeDirectory>(1, HAMSTER_NATIVE_FS_ROOT, flags);
        }
    };
} // namespace

int Hamster::_init_platform()
{
    umask(0); // Allow creating any permissions

    // Mount
    BaseFilesystem *fs = alloc<NativeFilesystem>(1);
    int ret = vfs.mount("/", fs);
    if (ret < 0)
    {
        dealloc(fs);
        return -1;
    }
    return 0;
}

void *Hamster::_malloc(size_t size)
{
    void *mem = malloc(size);
    return mem;
}

int Hamster::_free(void *ptr)
{
    free(ptr);
    return 0;
}

int Hamster::_log(const char *msg)
{
    int out = printf("%s", msg);
    fflush(stdout);
    return out;
}

int Hamster::_log(char c)
{
    int out = printf("%c", c);
    fflush(stdout);
    return out;
}

#endif
