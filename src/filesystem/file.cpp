// Hamster file

#include <filesystem/vfs.hpp>
#include <filesystem/file.hpp>
#include <memory/allocator.hpp>
#include <errno/errno.h>

namespace Hamster
{
    File::File(int fd)
        : fd(fd)
    {
    }

    File::File(const File &other)
        : fd(vfs.dup(other.fd))
    {
    }

    File &File::operator=(const File &other)
    {
        if (this != &other)
        {
            vfs.close(fd);
            fd = vfs.dup(other.fd);
        }
        return *this;
    }

    File::File(File &&other)
        : fd(other.fd)
    {
        // Don't clear `other.vfs` as it is still valid
        other.fd = -1;
    }

    File &File::operator=(File &&other)
    {
        if (this != &other)
        {
            vfs.close(fd);
            fd = other.fd;
            other.fd = -1; // Invalidate the moved-from file descriptor
        }
        return *this;
    }

    File::~File()
    {
        if (fd >= 0)
        {
            vfs.close(fd);
            fd = -1;
        }
    }

    File File::openat(const char *path, int flags, int mode)
    {
        if (!path)
        {
            errno = EINVAL;
            return File();
        }

        int fd = path[0] == '/' 
            ? vfs.open(path, flags, mode) : 
              vfs.openat(this->fd, path, flags, mode);
        
        if (fd < 0)
        {
            return File();
        }

        return File(fd);
    }

    int File::openat_replace(const char *path, int flags, int mode)
    {
        int newfd = path[0] == '/' ? vfs.open(path, flags, mode) :
                                     vfs.openat(this->fd, path, flags, mode);
        
        if (newfd < 0)
            return -1;
        
        vfs.close(fd);
        fd = newfd;

        return 0;
    }

    int File::close()
    {
        if (fd < 0)
        {
            errno = EBADF;
            return -1;
        }

        int result = vfs.close(fd);
        fd = -1; // Invalidate the file descriptor after closing
        return result;
    }

    int File::removeat(const char *path)
    {
        if (!path)
        {
            errno = EINVAL;
            return -1;
        }

        return path[0] == '/' 
            ? vfs.remove(path) : 
              vfs.removeat(this->fd, path);
    }

    int File::stat(sys_stat *buf)
    {
        return vfs.stat(fd, buf);
    }

    int File::statat(const char *path, sys_stat *buf)
    {
        if (!path)
        {
            errno = EINVAL;
            return -1;
        }

        return path[0] == '/' 
            ? vfs.stat(path, buf) : 
              vfs.statat(this->fd, path, buf);
    }

    int File::lstatat(const char *path, sys_stat *buf)
    {
        if (!path)
        {
            errno = EINVAL;
            return -1;
        }

        return path[0] == '/' 
            ? vfs.lstat(path, buf) : 
              vfs.lstatat(this->fd, path, buf);
    }

    int File::get_mode()
    {
        return vfs.get_mode(fd);
    }

    int File::get_flags()
    {
        return vfs.get_flags(fd);
    }

    int File::get_uid()
    {
        return vfs.get_uid(fd);
    }

    int File::get_gid()
    {
        return vfs.get_gid(fd);
    }

    int File::chmod(int mode)
    {
        return vfs.chmod(fd, mode);
    }

    int File::chown(int uid, int gid)
    {
        return vfs.chown(fd, uid, gid);
    }

    ssize_t File::read(void *buf, size_t size)
    {
        return vfs.read(fd, buf, size);
    }

    ssize_t File::write(void *buf, size_t size)
    {
        return vfs.write(fd, buf, size);
    }

    int File::seek(int offset, int whence)
    {
        return vfs.seek(fd, offset, whence);
    }

    int64_t File::tell()
    {
        return vfs.tell(fd);
    }

    int File::truncate(int64_t size)
    {
        return vfs.truncate(fd, size);
    }

    int64_t File::size()
    {
        return vfs.size(fd);
    }

    int File::set_targetat(const char *path, const char *target)
    {
        if (!path || !target)
        {
            errno = EINVAL;
            return -1;
        }

        return path[0] == '/' 
            ? vfs.set_target(path, target) : 
              vfs.set_targetat(this->fd, path, target);
    }

    char *const *File::list(size_t count)
    {
        return vfs.list(fd, count);
    }

    int File::mkfileat(const char *path, int mode)
    {
        if (!path)
        {
            errno = EINVAL;
            return -1;
        }

        return path[0] == '/' 
            ? vfs.mkfile(path, mode) : 
              vfs.mkfileat(this->fd, path, mode);
    }

    File File::mkfileat(const char *path, int flags, int mode)
    {
        if (!path)
        {
            errno = EINVAL;
            return File();
        }

        int fd = path[0] == '/' 
            ? vfs.mkfile(path, flags, mode) : 
              vfs.mkfileat(this->fd, path, flags, mode);

        if (fd < 0)
        {
            return File();
        }

        return File(fd);
    }

    int File::mkdirat(const char *path, int mode)
    {
        if (!path)
        {
            errno = EINVAL;
            return -1;
        }

        return path[0] == '/' 
            ? vfs.mkdir(path, mode) : 
              vfs.mkdirat(this->fd, path, mode);
    }

    File File::mkdirat(const char *path, int flags, int mode)
    {
        if (!path)
        {
            errno = EINVAL;
            return File();
        }

        int fd = path[0] == '/' 
            ? vfs.mkdir(path, flags, mode) : 
              vfs.mkdirat(this->fd, path, flags, mode);

        if (fd < 0)
        {
            return File();
        }

        return File(fd);
    }

    int File::symlinkat(const char *path, const char *target)
    {
        if (!path || !target)
        {
            errno = EINVAL;
            return -1;
        }

        return path[0] == '/' 
            ? vfs.symlink(path, target) : 
              vfs.symlinkat(this->fd, path, target);
    }

    int File::mknodat(const char *path, DeviceID id, int mode)
    {
        if (!path)
        {
            errno = EINVAL;
            return -1;
        }

        return path[0] == '/' 
            ? vfs.mknod(path, id, mode) : 
              vfs.mknodat(this->fd, path, id, mode);
    }

    File File::mknodat(const char *path, int flags, DeviceID id, int mode)
    {
        if (!path)
        {
            errno = EINVAL;
            return File();
        }

        int fd = path[0] == '/' 
            ? vfs.mknod(path, flags, id, mode) : 
              vfs.mknodat(this->fd, path, flags, id, mode);

        if (fd < 0)
        {
            return File();
        }

        return File(fd);
    }

    int File::ioctl(int request, IoctlArg arg)
    {
        return vfs.ioctl(fd, request, arg);
    }

    int File::set_flags(int flags)
    {
        return vfs.set_flags(fd, flags);
    }

} // namespace Hamster

