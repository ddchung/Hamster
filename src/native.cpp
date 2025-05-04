// Native version

#if !defined(ARDUINO) && 1

#include <platform/platform.hpp>
#include <filesystem/file.hpp>
#include <filesystem/mounts.hpp>
#include <memory/allocator.hpp>
#include <memory/stl_sequential.hpp>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <unordered_map>

#define ROOTFS_PATH "/Users/tin/hamster_rootfs"

using namespace Hamster;

namespace
{
    // posix
    class PosixRegularFile : public BaseRegularFile
    {
        int fd;
        String path;

    public:
        PosixRegularFile(int fd, const String &path) : fd(fd), path(path) {}

        ~PosixRegularFile() override
        {
            if (fd >= 0)
                close(fd);
        }

        int rename(const char *newname) override
        {
            int res = ::rename(path.c_str(), newname);
            if (res == 0)
                path = newname;
            return res;
        }

        int stat(struct ::stat *buf) override
        {
            return ::fstat(fd, buf);
        }

        int get_mode() override
        {
            struct ::stat st;
            if (::fstat(fd, &st) < 0)
                return -1;
            return st.st_mode;
        }

        int get_uid() override
        {
            struct ::stat st;
            if (::fstat(fd, &st) < 0)
                return -1;
            return st.st_uid;
        }

        int get_gid() override
        {
            struct ::stat st;
            if (::fstat(fd, &st) < 0)
                return -1;
            return st.st_gid;
        }

        int chmod(int mode) override
        {
            return ::fchmod(fd, mode);
        }

        int chown(int uid, int gid) override
        {
            return ::fchown(fd, uid, gid);
        }

        const char *name() override
        {
            return strrchr(path.c_str(), '/');
        }

        int remove() override
        {
            return ::unlink(path.c_str());
        }

        ssize_t read(uint8_t *buf, size_t size) override
        {
            return ::read(fd, buf, size);
        }

        ssize_t write(const uint8_t *buf, size_t size) override
        {
            return ::write(fd, buf, size);
        }

        int64_t seek(int64_t offset, int whence) override
        {
            return ::lseek(fd, offset, whence);
        }

        int64_t tell() override
        {
            return ::lseek(fd, 0, SEEK_CUR);
        }

        int truncate(int64_t size) override
        {
            return ::ftruncate(fd, size);
        }

        int64_t size() override
        {
            struct ::stat st;
            if (::fstat(fd, &st) < 0)
                return -1;
            return st.st_size;
        }
    };

    class PosixSymbolicFile : public BaseSymbolicFile
    {
        String path;
        FileType t;
    public:
        PosixSymbolicFile(const String &path, FileType type) : path(path), t(type) {}

        ~PosixSymbolicFile() override
        {
            if (path.length() > 0)
                unlink(path.c_str());
        }

        int rename(const char *newname) override
        {
            int res = ::rename(path.c_str(), newname);
            if (res == 0)
                path = newname;
            return res;
        }

        int remove() override
        {
            return ::unlink(path.c_str());
        }

        int stat(struct ::stat *buf) override
        {
            return ::lstat(path.c_str(), buf);
        }

        int get_mode() override
        {
            struct ::stat st;
            if (::lstat(path.c_str(), &st) < 0)
                return -1;
            return st.st_mode;
        }

        int get_uid() override
        {
            struct ::stat st;
            if (::lstat(path.c_str(), &st) < 0)
                return -1;
            return st.st_uid;
        }

        int get_gid() override
        {
            struct ::stat st;
            if (::lstat(path.c_str(), &st) < 0)
                return -1;
            return st.st_gid;
        }

        int chmod(int mode) override
        {
            return ::chmod(path.c_str(), mode);
        }

        int chown(int uid, int gid) override
        {
            return ::chown(path.c_str(), uid, gid);
        }

        const char *name() override
        {
            const char *last = strrchr(path.c_str(), '/');
            if (last)
                return last + 1;
            return path.c_str();
        }

        const FileType type() const override { return t; }
    };

    // --- Directory ---
    class PosixDirectory : public BaseDirectory
    {
        String path;

    public:
        PosixDirectory(const String &path) : path(path) {}

        int rename(const char *newname) override
        {
            int res = ::rename(path.c_str(), newname);
            if (res == 0)
                path = newname;
            return res;
        }

        int remove() override
        {
            return ::rmdir(path.c_str());
        }

        int stat(struct ::stat *buf) override
        {
            return ::stat(path.c_str(), buf);
        }

        int get_mode() override
        {
            struct ::stat st;
            if (::stat(path.c_str(), &st) < 0)
                return -1;
            return st.st_mode;
        }

        int get_uid() override
        {
            struct ::stat st;
            if (::stat(path.c_str(), &st) < 0)
                return -1;
            return st.st_uid;
        }

        int get_gid() override
        {
            struct ::stat st;
            if (::stat(path.c_str(), &st) < 0)
                return -1;
            return st.st_gid;
        }

        int chmod(int mode) override
        {
            return ::chmod(path.c_str(), mode);
        }
        
        int chown(int uid, int gid) override
        {
            return ::chown(path.c_str(), uid, gid);
        }

        const char *name() override
        {
            return strrchr(path.c_str(), '/') + 1;
        }

        char * const *list() override {
            DIR *dir = opendir(path.c_str());
            if (!dir) return nullptr;
        
            Vector<char *> results;
            struct dirent *entry;
        
            while ((entry = readdir(dir))) {
                const char *name = entry->d_name;
                if (std::strcmp(name, ".") == 0 || std::strcmp(name, "..") == 0)
                    continue;
                char *copy = alloc<char>(strlen(name) + 1);
                std::strcpy(copy, name);
                results.push_back(copy);
            }
        
            closedir(dir);
        
            // Allocate an array of pointers (+1 for the null terminator)
            char **listArray = alloc<char*>(results.size() + 1);
            for (size_t i = 0; i < results.size(); ++i) {
                listArray[i] = results[i];
            }
            listArray[results.size()] = nullptr; // null-terminated
        
            return listArray;
        }
        BaseFile *open(const char *name, int flags, va_list args) override
        {
            String full = path + "/" + name;
            int fd = ::open(full.c_str(), flags);
            if (fd < 0 && errno != ENOENT)
                return nullptr;
            else if (fd < 0)
            {
                int mode = va_arg(args, int);
                fd = ::open(full.c_str(), flags | O_CREAT, mode);
                if (fd < 0)
                    return nullptr;
            }

            struct stat st;
            if (fstat(fd, &st) < 0)
            {
                close(fd);
                return nullptr;
            }

            if (S_ISREG(st.st_mode))
                return alloc<PosixRegularFile>(1, fd, full);
            else if (S_ISFIFO(st.st_mode))
                return alloc<PosixSymbolicFile>(1, full, FileType::FIFO);
            else if (S_ISCHR(st.st_mode))
                return alloc<PosixSymbolicFile>(1, full, FileType::CharDevice);
            else if (S_ISBLK(st.st_mode))
                return alloc<PosixSymbolicFile>(1, full, FileType::BlockDevice);
            else if (S_ISDIR(st.st_mode))
            {
                close(fd);
                return alloc<PosixDirectory>(1, full);
            }
            else
            {
                close(fd);
                return nullptr;
            }
        }

        BaseFile *mkfile(const char *name, int flags, int mode) override
        {
            String full = path + "/" + name;
            int fd = ::open(full.c_str(), flags | O_CREAT, mode);
            if (fd < 0)
                return nullptr;
            return alloc<PosixRegularFile>(1, fd, full);
        }

        BaseFile *mkdir(const char *name, int flags, int mode) override
        {
            String full = path + "/" + name;
            if (::mkdir(full.c_str(), mode) < 0)
                return nullptr;
            return alloc<PosixDirectory>(1, full);
        }

        BaseFile *mksfile(const char *name, int flags, FileType type, int mode) override
        {
            String full = path + "/" + name;
            mode_t m;
            if (type == FileType::FIFO)
                m = S_IFIFO;
            else if (type == FileType::CharDevice)
                m = S_IFCHR;
            else if (type == FileType::BlockDevice)
                m = S_IFBLK;
            else
                return nullptr;
            if (::mknod(full.c_str(), m | mode, 0) < 0)
                return nullptr;
            return alloc<PosixSymbolicFile>(1, full, type);
        }

        int remove(const char *name) override
        {
            String full = path + "/" + name;
            return ::unlink(full.c_str());
        }
    };

    // --- Filesystem ---
    class PosixFilesystem : public BaseFilesystem
    {
    public:
        BaseFile *open(const char *_path, int flags, va_list args) override
        {
            if (!_path)
                return nullptr;
            String path = ROOTFS_PATH + String(_path);
            struct stat st;
            
            if (::stat(path.c_str(), &st) <= 0 && S_ISDIR(st.st_mode))
            {
                return alloc<PosixDirectory>(1, path);
            }

            int fd;

            if (flags & O_CREAT)
            {
                int mode = va_arg(args, int);
                fd = ::open(path.c_str(), flags, mode);
            }
            else
            {
                fd = ::open(path.c_str(), flags);
            }

            if (fd < 0)
                return nullptr;

            if (fstat(fd, &st) < 0)
            {
                close(fd);
                return nullptr;
            }

            if (S_ISREG(st.st_mode))
                return alloc<PosixRegularFile>(1, fd, path);
            else if (S_ISFIFO(st.st_mode))
                return alloc<PosixSymbolicFile>(1, path, FileType::FIFO);
            else if (S_ISCHR(st.st_mode))
                return alloc<PosixSymbolicFile>(1, path, FileType::CharDevice);
            else if (S_ISBLK(st.st_mode))
                return alloc<PosixSymbolicFile>(1, path, FileType::BlockDevice);
            else if (S_ISDIR(st.st_mode))
            {
                close(fd);
                return alloc<PosixDirectory>(1, path);
            }
            else
            {
                close(fd);
                return nullptr;
            }
        }

        int rename(const char *oldpath_, const char *newpath_) override
        {
            if (!oldpath_ || !newpath_)
                return -1;
            String oldpath = ROOTFS_PATH + String(oldpath_);
            String newpath = ROOTFS_PATH + String(newpath_);
            return ::rename(oldpath.c_str(), newpath.c_str());
        }

        int link(const char *oldpath_, const char *newpath_) override
        {
            if (!oldpath_ || !newpath_)
                return -1;
            String oldpath = ROOTFS_PATH + String(oldpath_);
            String newpath = ROOTFS_PATH + String(newpath_);
            return ::link(oldpath.c_str(), newpath.c_str());
        }

        int unlink(const char *path_) override
        {
            if (!path_)
                return -1;
            String path = ROOTFS_PATH + String(path_);
            return ::unlink(path.c_str());
        }

        int stat(const char *path_, struct ::stat *buf) override
        {
            if (!path_)
                return -1;
            String path = ROOTFS_PATH + String(path_);
            return ::stat(path.c_str(), buf);
        }

        BaseDirectory *mkdir(const char *path_, int flags, int mode) override
        {
            if (!path_)
                return nullptr;
            String path = ROOTFS_PATH + String(path_);
            if (::mkdir(path.c_str(), mode) < 0)
                return nullptr;
            return alloc<PosixDirectory>(1, path);
        }

        BaseSymbolicFile *mksfile(const char *path_, int flags, FileType type, int mode) override
        {
            if (!path_)
                return nullptr;
            String path = ROOTFS_PATH + String(path_);
            mode_t m;
            if (type == FileType::FIFO)
                m = S_IFIFO;
            else if (type == FileType::CharDevice)
                m = S_IFCHR;
            else if (type == FileType::BlockDevice)
                m = S_IFBLK;
            else
                return nullptr;
            if (::mknod(path.c_str(), m | mode, 0) < 0)
                return nullptr;
            return alloc<PosixSymbolicFile>(1, path, type);
        }

        BaseRegularFile *mkfile(const char *path_, int flags, int mode) override
        {
            if (!path_)
                return nullptr;
            String path = ROOTFS_PATH + String(path_);
            int fd = ::open(path.c_str(), flags | O_CREAT, mode);
            if (fd < 0)
                return nullptr;
            return alloc<PosixRegularFile>(1, fd, path);
        }

        int symlink(const char *oldpath_, const char *newpath_) override
        {
            if (!oldpath_ || !newpath_)
                return -1;
            String oldpath = ROOTFS_PATH + String(oldpath_);
            String newpath = ROOTFS_PATH + String(newpath_);
            return ::symlink(oldpath.c_str(), newpath.c_str());
        }

        int readlink(const char *path_, char *buf, size_t buf_size) override
        {
            if (!path_)
                return -1;
            String path = ROOTFS_PATH + String(path_);
            char *buf2 = alloc<char>(buf_size);
            int res = ::readlink(path.c_str(), buf2, buf_size);
            if (res < 0)
            {
                dealloc(buf2);
                return -1;
            }
            buf2[res] = '\0';
            strcpy(buf + sizeof(ROOTFS_PATH), buf2);
            dealloc(buf2);
            return res;
        }

        char *readlink(const char *path_) override
        {
            if (!path_)
                return nullptr;
            String path = ROOTFS_PATH + String(path_);
            char *buf = alloc<char>(PATH_MAX);
            int res = ::readlink(path.c_str(), buf, PATH_MAX);
            if (res < 0)
            {
                dealloc(buf);
                return nullptr;
            }
            buf[res] = '\0';
            return buf;
        }

        FileType type(const char *path_) override
        {
            struct ::stat st;
            if (this->lstat(path_, &st) < 0)
                return FileType::Invalid;
            if (S_ISREG(st.st_mode))
                return FileType::Regular;
            else if (S_ISDIR(st.st_mode))
                return FileType::Directory;
            else if (S_ISFIFO(st.st_mode))
                return FileType::FIFO;
            else if (S_ISCHR(st.st_mode))
                return FileType::CharDevice;
            else if (S_ISBLK(st.st_mode))
                return FileType::BlockDevice;
            else if (S_ISLNK(st.st_mode))
                return FileType::SymbolicLink;
            else
                return FileType::Invalid;
        }

        int lstat(const char *path_, struct ::stat *buf) override
        {
            if (!path_)
                return -1;
            String path = ROOTFS_PATH + String(path_);
            return ::lstat(path.c_str(), buf);
        }
    };
}

namespace
{
    std::unordered_map<void*, size_t> allocated_memory;
}

int Hamster::_init_platform()
{
    // nothing to do
    return 0;
}

int Hamster::_mount_rootfs()
{
    mounts.mount(alloc<PosixFilesystem>(), "/");
    return 0;
}

void *Hamster::_malloc(size_t size)
{
    void *mem = malloc(size);
    assert(allocated_memory.find(mem) == allocated_memory.end());
    allocated_memory[mem] = size;
    return mem;
}

int Hamster::_free(void *ptr)
{
    assert(allocated_memory.find(ptr) != allocated_memory.end());
    free(ptr);
    allocated_memory.erase(ptr);
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
