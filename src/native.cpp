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

        int remove() override
        {
            return ::unlink(path.c_str());
        }

        int read(uint8_t *buf, size_t size) override
        {
            return ::read(fd, buf, size);
        }

        int write(const uint8_t *buf, size_t size) override
        {
            return ::write(fd, buf, size);
        }

        int64_t seek(int64_t offset, int whence) override
        {
            return ::lseek(fd, offset, whence);
        }
    };

    // --- FIFO File ---
    class PosixFifoFile : public BaseFifoFile
    {
        int fd;
        String path;

    public:
        PosixFifoFile(int fd, const String &path) : fd(fd), path(path) {}

        ~PosixFifoFile() override
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

        int remove() override
        {
            return ::unlink(path.c_str());
        }

        int read(uint8_t *buf, size_t size) override
        {
            return ::read(fd, buf, size);
        }

        int write(const uint8_t *buf, size_t size) override
        {
            return ::write(fd, buf, size);
        }
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

        BaseFile *get(const char *name, int flags, ...) override
        {
            String full = path + "/" + name;
            int fd = open(full.c_str(), flags);
            if (fd < 0)
                return nullptr;

            struct stat st;
            if (fstat(fd, &st) < 0)
            {
                close(fd);
                return nullptr;
            }

            if (S_ISREG(st.st_mode))
                return alloc<PosixRegularFile>(1, fd, full);
            else if (S_ISFIFO(st.st_mode))
                return alloc<PosixFifoFile>(1, fd, full);
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

        BaseFile *mkfifo(const char *name, int flags, int mode) override
        {
            String full = path + "/" + name;
            if (::mkfifo(full.c_str(), mode) < 0)
                return nullptr;
            int fd = ::open(full.c_str(), flags);
            if (fd < 0)
                return nullptr;
            return alloc<PosixFifoFile>(1, fd, full);
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
        BaseFile *open(const char *path, int flags, ...) override
        {
            va_list args;
            va_start(args, flags);
            BaseFile *f = open(path, flags, args);
            va_end(args);
            return f;
        }

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
                return alloc<PosixFifoFile>(1, fd, path);
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

        BaseFifoFile *mkfifo(const char *path_, int flags, int mode) override
        {
            if (!path_)
                return nullptr;
            String path = ROOTFS_PATH + String(path_);
            if (::mkfifo(path.c_str(), mode) < 0)
                return nullptr;
            int fd = ::open(path.c_str(), flags);
            if (fd < 0)
                return nullptr;
            return alloc<PosixFifoFile>(1, fd, path);
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
    Mounts::instance().mount(alloc<PosixFilesystem>(), "/");
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
