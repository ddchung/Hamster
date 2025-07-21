// Hamster VFS Implementation

#include <filesystem/vfs.hpp>
#include <filesystem/vfs_mounts.hpp>
#include <filesystem/vfs_fdman.hpp>
#include <filesystem/vfs_special.hpp>
#include <memory/allocator.hpp>
#include <memory/stl_sequential.hpp>
#include <memory/stl_map.hpp>
#include <errno/errno.h>
#include <cstring>

namespace Hamster
{
    /* Special File Helpers */
    namespace
    {
        int open_special_handle(BaseSpecialFile *file, SpecialDriverManager &sp_mgr) 
        {
            if (!file)
            {
                error = EINVAL;
                return -1;
            }

            dealloc(file->get_handle());
            BaseSpecialDriver *driver = sp_mgr.get_driver(file->get_device_id());

            if (!driver)
                return -1;

            BaseSpecialDriverHandle *handle = driver->create_handle(file->get_flags());
            if (!handle)
            {
                error = EIO;
                return -1;
            }

            file->set_handle(handle);
            return 0;
        }

        BaseSpecialDriverHandle *get_special_handle(BaseSpecialFile *file, SpecialDriverManager &sp_mgr)
        {
            if (!file)
            {
                error = EINVAL;
                return nullptr;
            }

            BaseSpecialDriverHandle *handle = file->get_handle();
            if (!handle)
            {
                if (open_special_handle(file, sp_mgr) < 0)
                    return nullptr;
                handle = file->get_handle();
            }

            return handle;
        }
    } // namespace

    class VFSData
    {
    public:
        VFSData() = default;
        ~VFSData() = default;
        VFSData(const VFSData &) = delete;
        VFSData &operator=(const VFSData &) = delete;
        VFSData(VFSData &&) = default;
        VFSData &operator=(VFSData &&) = default;

        Mounts mounts;
        FDManager fd_manager;
        SpecialDriverManager special_driver_manager;
    };

    VFS::VFS()
        : data(alloc<VFSData>(1))
    {
    }

    VFS::~VFS()
    {
        dealloc(data);
    }

    VFS::VFS(VFS &&other)
        : data(other.data)
    {
        other.data = nullptr;
    }

    VFS &VFS::operator=(VFS &&other)
    {
        if (this == &other)
            return *this;
        dealloc(data);
        data = other.data;
        other.data = nullptr;
        return *this;
    }

    int VFS::mount(const char *path, BaseFilesystem *fs)
    {
        return strcmp(path, "/") != 0 ? data->mounts.mount(path, fs)
                                      : data->mounts.mount_root(fs);
    }

    int VFS::unmount(const char *path)
    {
        return data->mounts.unmount(path);
    }

    int VFS::open(const char *path, int flags, int mode)
    {
        BaseFile *file = data->mounts.lopen(path, flags, mode);
        if (!file)
            return -1;

        if (file->type() == FileType::Symlink)
        {
            if (flags & OPEN_NOFOLLOW)
            {
                dealloc(file);
                error = ELOOP;
                return -1;
            }
            BaseSymlink *link = (BaseSymlink *)file;
            char *target = link->get_target();
            int id = link->get_id();
            dealloc(link);

            if (!target)
            {
                error = ENOENT;
                return -1;
            }

            file = data->mounts.lopen(target, flags, mode);
            dealloc(target);
            if (!file)
                return -1;
            if (file->get_id() == id)
            {
                dealloc(file);
                error = ELOOP; // Loop detected
                return -1;
            }
        }

        int fd = data->fd_manager.add_fd(file);
        if (fd < 0)
        {
            dealloc(file);
            return -1;
        }

        return fd;
    }

    int VFS::close(int fd)
    {
        return data->fd_manager.remove_fd(fd);
    }

    int VFS::renameat(int old_dfd, const char *old_path, int new_dfd, const char *new_path)
    {
        if (!old_path || !new_path)
        {
            error = EINVAL;
            return -1;
        }

        const char *last_old = strrchr(old_path, '/');
        const char *last_new = strrchr(new_path, '/');
        const char *old_name, *new_name;

        BaseFile *f = data->fd_manager.get_fd(old_dfd);
        if (!f || f->type() != FileType::Directory)
        {
            error = EBADF;
            return -1;
        }
        BaseDirectory *old_dir = (BaseDirectory*)f;
        f = data->fd_manager.get_fd(new_dfd);
        if (!f || f->type() != FileType::Directory)
        {
            error = EBADF;
            return -1;
        }
        BaseDirectory *new_dir = (BaseDirectory*)f;

        old_dir = (BaseDirectory*)old_dir->clone();
        new_dir = (BaseDirectory*)new_dir->clone();
        if (!old_dir || !new_dir)
        {
            dealloc(old_dir);
            dealloc(new_dir);
            return -1;
        }

        if (last_old)
        {
            old_name = last_old + 1;
            String old_dir_name{old_path, (size_t)(last_old - old_path)};
            old_dir = (BaseDirectory*)data->mounts.lopen(old_dir_name.c_str(), OPEN_RDONLY | OPEN_DIRECTORY, 0, old_dir);
        }
        else
        {
            old_name = old_path;
            old_dir = (BaseDirectory*)data->mounts.lopen("/", OPEN_RDONLY | OPEN_DIRECTORY, 0, old_dir);
        }

        if (last_new)
        {
            new_name = last_new + 1;
            new_dir = (BaseDirectory*)data->mounts.lopen(String{new_path, (size_t)(last_new - new_path)}.c_str(), OPEN_WRONLY | OPEN_DIRECTORY, 0, new_dir);
        }
        else
        {
            new_name = new_path;
            new_dir = (BaseDirectory*)data->mounts.lopen("/", OPEN_WRONLY | OPEN_DIRECTORY, 0, new_dir);
        }

        if (!old_dir || !new_dir)
        {
            dealloc(old_dir);
            dealloc(new_dir);

            return -1;
        }

        assert(old_name && new_name);

        BaseFile *old_file = old_dir->get(old_name, OPEN_RDONLY, 0);
        if (!old_file)
        {
            dealloc(old_dir);
            dealloc(new_dir);
            error = ENOENT;
            return -1;
        }

        if (old_file->get_filesystem() != new_dir->get_filesystem())
        {
            dealloc(old_file);
            dealloc(old_dir);
            dealloc(new_dir);
            error = EXDEV; // Cross-device link
            return -1;
        }

        int ret = new_dir->link(old_file, new_name);

        if (ret < 0)
        {
            dealloc(old_file);
            dealloc(old_dir);
            dealloc(new_dir);
            return -1;
        }

        ret = old_dir->remove(old_name);
        dealloc(old_file);
        dealloc(old_dir);
        dealloc(new_dir);
        return ret;
    }

    int VFS::rename(const char *old_path, const char *new_path)
    {
        int rootfd = open("/", OPEN_RDONLY | OPEN_DIRECTORY);
        if (rootfd < 0)
            return -1;

        int ret = renameat(rootfd, old_path, rootfd, new_path);
        close(rootfd);
        return ret;
    }

    int VFS::removeat(int dfd, const char *path)
    {
        BaseFile *file = data->fd_manager.get_fd(dfd);
        if (!file)
        {
            error = EBADF;
            return -1;
        }
        
        if (file->type() != FileType::Directory)
        {
            error = ENOTDIR;
            return -1;
        }

        BaseDirectory *dir = (BaseDirectory *)file->clone();
        if (!dir)
            return -1;
        
        const char *last = strrchr(path, '/');
        if (!last)
        {
            error = EINVAL;
            return -1;
        }

        String dir_name{path, (size_t)(last - path)};
        dir = (BaseDirectory*)data->mounts.lopen(dir_name.c_str(), OPEN_WRONLY, 0, dir);

        if (!dir)
            return -1;

        int res = dir->remove(last + 1);
        dealloc(dir);
        return res;
    }

    int VFS::remove(const char *path)
    {
        if (!path)
        {
            error = EINVAL;
            return -1;
        }

        int rootfd = open("/", OPEN_RDWR | OPEN_DIRECTORY);
        int res = removeat(rootfd, path);
        close(rootfd);

        return res;
    }

    int VFS::stat(int fd, sys_stat *buf)
    {
        BaseFile *file = data->fd_manager.get_fd(fd);
        if (!file)
            return -1;

        int ret = file->stat(buf);

        if (file->type() == FileType::Special)
        {
            // Get the special file handle
            BaseSpecialDriverHandle *handle = get_special_handle((BaseSpecialFile *)file, data->special_driver_manager);
            if (!handle)
            {
                error = EBADF;
                return -1;
            }

            buf->mode &= ~STAT_IFMT; // Clear the file type bits

            switch (handle->special_type())
            {
                case SpecialFileType::CharacterDevice:
                    buf->mode |= STAT_IFCHR;
                    break;
                case SpecialFileType::BlockDevice:
                    buf->mode |= STAT_IFBLK;
                    break;
                case SpecialFileType::Socket:
                    buf->mode |= STAT_IFSOCK;
                    break;
                case SpecialFileType::Fifo:
                    buf->mode |= STAT_IFIFO;
                    break;
                default:
                    // Do nothing for other types
                    break;
            }
        }

        return ret;
    }

    int VFS::statat(int dfd, const char *path, sys_stat *buf)
    {
        int fd = openat(dfd, path, OPEN_RDONLY);
        if (fd < 0)
            return -1;
        int res = stat(fd, buf);
        close(fd);
        return res;
    }

    int VFS::stat(const char *path, sys_stat *buf)
    {
        int fd = open(path, OPEN_RDONLY);
        if (fd < 0)
            return -1;
        int res = stat(fd, buf);
        close(fd);
        return res;
    }

    int VFS::lstatat(int dfd, const char *path, sys_stat *buf)
    {
        if (!path)
        {
            error = EINVAL;
            return -1;
        }

        const char *last = strrchr(path, '/');
        BaseFile *file = nullptr;
        if (last)
        {
            String dir_name{path, (size_t)(last - path)};

            int parent_fd = openat(dfd, dir_name.c_str(), OPEN_RDONLY | OPEN_DIRECTORY);
            if (parent_fd < 0)
                return -1;
            
            BaseDirectory *parent_dir = (BaseDirectory *)data->fd_manager.get_fd(parent_fd);
            
            assert(parent_dir);

            file = parent_dir->get(last + 1, OPEN_RDONLY, 0);
            close(parent_fd);
        }
        else
        {
            BaseDirectory *parent_dir = (BaseDirectory *)data->fd_manager.get_fd(dfd);
            if (!parent_dir)
            {
                error = EBADF;
                return -1;
            }

            file = parent_dir->get(path, OPEN_RDONLY, 0);
        }

        if (!file)
        {
            return -1;
        }

        int ret = file->stat(buf);

        if (ret < 0)
        {
            dealloc(file);
            return -1;
        }

        if (file->type() == FileType::Special)
        {
            // Get the special file handle
            BaseSpecialDriverHandle *handle = get_special_handle((BaseSpecialFile *)file, data->special_driver_manager);
            if (!handle)
            {
                dealloc(file);
                error = EBADF;
                return -1;
            }

            buf->mode &= ~STAT_IFMT; // Clear the file type bits

            switch (handle->special_type())
            {
                case SpecialFileType::CharacterDevice:
                    buf->mode |= STAT_IFCHR;
                    break;
                case SpecialFileType::BlockDevice:
                    buf->mode |= STAT_IFBLK;
                    break;
                case SpecialFileType::Socket:
                    buf->mode |= STAT_IFSOCK;
                    break;
                case SpecialFileType::Fifo:
                    buf->mode |= STAT_IFIFO;
                    break;
                default:
                    // Do nothing for other types
                    break;
            }

            dealloc(handle);
        }

        dealloc(file);

        return ret;
    }

    int VFS::lstat(const char *path, sys_stat *buf)
    {
        if (!path)
        {
            error = EINVAL;
            return -1;
        }

        BaseFile *file = data->mounts.lopen(path, OPEN_RDONLY, 0);
        if (!file)
            return -1;

        int ret = file->stat(buf);
        dealloc(file);
        return ret;
    }

    int VFS::linkat(int target_dfd, const char *target_path, int dfd, const char *path)
    {
        if (target_dfd < 0 || dfd < 0)
        {
            error = EBADF;
            return -1;
        }

        if (!target_path || !path)
        {
            error = EINVAL;
            return -1;
        }

        BaseFile *file = data->fd_manager.get_fd(dfd);
        if (!file)
        {
            error = EBADF;
            return -1;
        }
        
        if (file->type() != FileType::Directory)
        {
            error = ENOTDIR;
            return -1;
        }

        BaseFile *file2 = data->fd_manager.get_fd(target_dfd);
        if (!file2)
        {
            error = EBADF;
            return -1;
        }
        
        if (file2->type() != FileType::Directory)
        {
            error = ENOTDIR;
            return -1;
        }
        
        BaseDirectory *dir = (BaseDirectory*)file->clone();
        BaseDirectory *target_dir = (BaseDirectory*)file2->clone();

        if (!dir || !target_dir)
        {
            dealloc(dir);
            dealloc(target_dir);
            return -1;
        }

        const char *last = strrchr(path, '/');
        const char *name = last ? last + 1 : path;
        char *dirname;

        if (last)
        {
            dirname = alloc<char>(last - path + 1);
            strncpy(dirname, path, last - path);
            dirname[last - path] = '\0';
        }
        else
        {
            dirname = alloc<char>(2);
            dirname[0] = '/';
            dirname[1] = '\0';
        }

        BaseDirectory *parent_dir = (BaseDirectory*)data->mounts.lopen(dirname, OPEN_RDWR | OPEN_DIRECTORY, 0, target_dir);
        dealloc(dirname);
        BaseFile *target_file = data->mounts.lopen(target_path, OPEN_RDONLY, 0, dir);

        if (!parent_dir || !target_file)
        {
            dealloc(parent_dir);
            dealloc(target_file);
            return -1;
        }

        if (target_file->type() == FileType::Symlink)
        {
            BaseSymlink *link = (BaseSymlink *)target_file;
            char *target = link->get_target();
            dealloc(link);

            if (!target)
            {
                dealloc(parent_dir);
                error = ENOENT;
                return -1;
            }

            // Note: do not specify dir in this one, as symlinks are absolute and should
            // be relative to the root directory
            target_file = data->mounts.lopen(target, OPEN_RDONLY, 0);
            dealloc(target);

            if (!target_file)
            {
                dealloc(parent_dir);
                error = ENOENT;
                return -1;
            }
        }

        int res;

        if (target_file->type() != FileType::Directory)
            res = parent_dir->link(target_file, name);
        else
        {
            error = EPERM;
            res = -1;
        }

        dealloc(parent_dir);
        dealloc(target_file);
        
        return res;
    }

    int VFS::link(const char *target, const char *path)
    {
        if (!target || !path)
        {
            error = EINVAL;
            return -1;
        }

        int rootfd = open("/", OPEN_RDWR | OPEN_DIRECTORY);
        if (rootfd < 0)
            return -1;

        int res = linkat(rootfd, target, rootfd, path);
        close(rootfd);

        return res;
    }

    int VFS::get_mode(int fd)
    {
        BaseFile *file = data->fd_manager.get_fd(fd);
        if (!file)
            return -1;

        int ret = file->get_mode();
        return ret;
    }

    int VFS::get_flags(int fd)
    {
        BaseFile *file = data->fd_manager.get_fd(fd);
        if (!file)
            return -1;

        int ret = file->get_flags();
        return ret;
    }

    int VFS::get_uid(int fd)
    {
        BaseFile *file = data->fd_manager.get_fd(fd);
        if (!file)
            return -1;

        int ret = file->get_uid();
        return ret;
    }

    int VFS::get_gid(int fd)
    {
        BaseFile *file = data->fd_manager.get_fd(fd);
        if (!file)
            return -1;

        int ret = file->get_gid();
        return ret;
    }

    int VFS::chmod(int fd, int mode)
    {
        BaseFile *file = data->fd_manager.get_fd(fd);
        if (!file)
            return -1;

        int ret = file->chmod(mode);
        return ret;
    }

    int VFS::chown(int fd, int uid, int gid)
    {
        BaseFile *file = data->fd_manager.get_fd(fd);
        if (!file)
            return -1;

        int ret = file->chown(uid, gid);
        return ret;
    }

    ssize_t VFS::read(int fd, void *buf, size_t size)
    {
        BaseFile *file = data->fd_manager.get_fd(fd);
        if (!file)
            return -1;

        switch (file->type())
        {
        case FileType::Regular:
            return ((BaseRegularFile *)file)->read((uint8_t*)buf, size);
        case FileType::Special:
        {
            auto handle = get_special_handle((BaseSpecialFile *)file, data->special_driver_manager);
            if (!handle)
                return -1;
            return handle->read((uint8_t*)buf, size);
        }
        default:
            error = EISDIR;
            return -1;
        }
    }

    ssize_t VFS::write(int fd, const void *buf, size_t size)
    {
        BaseFile *file = data->fd_manager.get_fd(fd);
        if (!file)
            return -1;

        switch (file->type())
        {
        case FileType::Regular:
            return ((BaseRegularFile *)file)->write((const uint8_t*)buf, size);
        case FileType::Special:
        {
            auto handle = get_special_handle((BaseSpecialFile *)file, data->special_driver_manager);
            if (!handle)
                return -1;
            return handle->write((const uint8_t*)buf, size);
        }
        default:
            error = EISDIR;
            return -1;
        }
    }

    int VFS::seek(int fd, int64_t offset, int whence)
    {
        BaseFile *file = data->fd_manager.get_fd(fd);
        if (!file)
            return -1;

        switch (file->type())
        {
        case FileType::Regular:
            return ((BaseRegularFile *)file)->seek(offset, whence);
        case FileType::Special:
        {
            auto handle = get_special_handle((BaseSpecialFile *)file, data->special_driver_manager);
            if (!handle)
                return -1;
            if (handle->special_type() != SpecialFileType::BlockDevice)
            {
                error = EISDIR;
                return -1;
            }
            return ((BaseBlockDeviceHandle *)handle)->seek(offset, whence);
        }
        case FileType::Directory:
            // Seeking in directories changes the offset for list()
            return ((BaseDirectory *)file)->seek(offset, whence);
        default:
            error = ESPIPE;
            return -1;
        }
    }

    int64_t VFS::tell(int fd)
    {
        BaseFile *file = data->fd_manager.get_fd(fd);
        if (!file)
            return -1;

        switch (file->type())
        {
        case FileType::Regular:
            return ((BaseRegularFile *)file)->tell();
        case FileType::Special:
        {
            auto handle = get_special_handle((BaseSpecialFile *)file, data->special_driver_manager);
            if (!handle)
                return -1;
            if (handle->special_type() != SpecialFileType::BlockDevice)
            {
                error = EISDIR;
                return -1;
            }
            return ((BaseBlockDeviceHandle *)handle)->tell();
        }
        default:
            error = EISDIR;
            return -1;
        }
    }

    int VFS::truncate(int fd, int64_t size)
    {
        BaseFile *file = data->fd_manager.get_fd(fd);
        if (!file)
            return -1;

        if (file->type() != FileType::Regular)
        {
            error = EISDIR;
            return -1;
        }
        int ret = ((BaseRegularFile *)file)->truncate(size);
        return ret;
    }

    int64_t VFS::size(int fd)
    {
        BaseFile *file = data->fd_manager.get_fd(fd);
        if (!file)
            return -1;

        switch (file->type())
        {
        case FileType::Regular:
            return ((BaseRegularFile *)file)->size();
        case FileType::Special:
        {
            auto handle = get_special_handle((BaseSpecialFile *)file, data->special_driver_manager);
            if (!handle)
                return -1;
            if (handle->special_type() != SpecialFileType::BlockDevice)
            {
                error = EISDIR;
                return -1;
            }
            return ((BaseBlockDeviceHandle *)handle)->size();
        }
        default:
            error = EISDIR;
            return -1;
        }
    }

    char *VFS::get_targetat(int dfd, const char *path)
    {
        BaseFile *file = data->fd_manager.get_fd(dfd);
        if (!file || file->type() != FileType::Directory)
        {
            error = EBADF;
            return nullptr;
        }   

        BaseDirectory *dir = (BaseDirectory *)file->clone();
        if (!dir)
            return nullptr;

        file = data->mounts.lopen(path, OPEN_RDONLY, 0, dir);
        if (!file)
            return nullptr;

        if (file->type() != FileType::Symlink)
        {
            dealloc(file);
            error = EINVAL;
            return nullptr;
        }

        char *ret = ((BaseSymlink *)file)->get_target();
        dealloc(file);
        return ret;
    }

    char *VFS::get_target(const char *path)
    {
        int rootfd = open("/", OPEN_RDONLY | OPEN_DIRECTORY);
        if (rootfd < 0)
            return nullptr;
        char *ret = get_targetat(rootfd, path);
        close(rootfd);
        return ret;
    }

    int VFS::set_targetat(int dfd, const char *path, const char *target)
    {
        if (!path || !target)
        {
            error = EINVAL;
            return -1;
        }

        BaseFile *file = data->fd_manager.get_fd(dfd);
        if (!file || file->type() != FileType::Directory)
        {
            error = EBADF;
            return -1;
        }   

        BaseDirectory *dir = (BaseDirectory *)file->clone();
        if (!dir)
            return -1;

        file = data->mounts.lopen(path, OPEN_WRONLY, 0, dir);
        if (!file)
            return -1;

        if (file->type() != FileType::Symlink)
        {
            dealloc(file);
            error = EINVAL;
            return -1;
        }

        int ret = ((BaseSymlink *)file)->set_target(target);
        dealloc(file);
        return ret;
    }

    int VFS::set_target(const char *path, const char *target)
    {
        if (!path || !target)
        {
            error = EINVAL;
            return -1;
        }

        int rootfd = open("/", OPEN_RDONLY | OPEN_DIRECTORY);
        if (rootfd < 0)
            return -1;

        int ret = set_targetat(rootfd, path, target);
        close(rootfd);
        return ret;
    }

    char *const *VFS::list(int fd, size_t count /* = SIZE_MAX */)
    {
        BaseFile *file = data->fd_manager.get_fd(fd);
        if (!file)
            return nullptr;

        if (file->type() != FileType::Directory)
        {
            dealloc(file);
            error = ENOTDIR;
            return nullptr;
        }

        char *const *ret = ((BaseDirectory *)file)->list(count);
        return ret;
    }

    int VFS::openat(int dir, const char *path, int flags, int mode)
    {
        BaseFile *file = data->fd_manager.get_fd(dir);
        if (!file)
            return -1;
        
        if (path[0] == '\0')
        {
            int fd = dup(dir);
            if (fd < 0) return -1;

            BaseFile *fdfile = data->fd_manager.get_fd(fd);
            fdfile->set_flags(flags);
            return fd;
        }

        if (file->type() != FileType::Directory)
        {
            error = ENOTDIR;
            return -1;
        }

        BaseFile *cloned_file = file->clone();
        if (!cloned_file)
            return -1;
        assert(cloned_file->type() == FileType::Directory);

        BaseFile *new_file = data->mounts.lopen(path, flags, mode, (BaseDirectory *)cloned_file);
        if (!new_file)
            return -1;

        if (new_file->type() == FileType::Symlink)
        {
            if (flags & OPEN_NOFOLLOW)
            {
                dealloc(new_file);
                error = ELOOP;
                return -1;
            }
            BaseSymlink *link = (BaseSymlink *)new_file;
            char *target = link->get_target();
            int id = link->get_id();
            dealloc(link);

            if (!target)
            {
                error = ENOENT;
                return -1;
            }

            //re-clone the file
            cloned_file = file->clone();
            if (!cloned_file)
            {
                dealloc(target);
                return -1;
            }

            new_file = data->mounts.lopen(target, flags, mode, (BaseDirectory*)cloned_file);
            dealloc(target);
            if (!new_file)
                return -1;
            if (new_file->get_id() == id)
            {
                dealloc(new_file);
                error = ELOOP; // Loop detected
                return -1;
            }
        }

        int fd = data->fd_manager.add_fd(new_file);
        if (fd < 0)
        {
            dealloc(new_file);
            return -1;
        }
        return fd;
    }

    int VFS::mkfile(const char *path, int flags, int mode)
    {
        return open(path, flags | OPEN_CREAT | OPEN_EXCL, mode);
    }

    int VFS::mkfileat(int dir, const char *path, int flags, int mode)
    {
        return openat(dir, path, flags | OPEN_CREAT | OPEN_EXCL, mode);
    }

    int VFS::mkdir(const char *path, int flags, int mode)
    {
        return open(path, flags | OPEN_CREAT | OPEN_EXCL | OPEN_DIRECTORY, mode);
    }

    int VFS::mkdirat(int dir, const char *path, int flags, int mode)
    {
        return openat(dir, path, flags | OPEN_CREAT | OPEN_EXCL | OPEN_DIRECTORY, mode);
    }

    int VFS::symlink(const char *path, const char *target)
    {
        if (!path || !target)
        {
            error = EINVAL;
            return -1;
        }

        const char *last = strrchr(path, '/');
        if (!last)
        {
            error = EINVAL;
            return -1;
        }

        String parent_path(path, last - path);

        BaseFile *parent = data->mounts.lopen(parent_path.c_str(), OPEN_RDONLY | OPEN_DIRECTORY, 0);
        if (!parent)
            return -1;
        assert(parent->type() == FileType::Directory);
        auto sym = ((BaseDirectory *)parent)->mksym(last + 1, target);
        dealloc(parent);
        dealloc(sym);
        return sym == nullptr ? -1 : 0;
    }

    int VFS::symlinkat(int dir_fd, const char *path, const char *target)
    {
        if (!path || !target)
        {
            error = EINVAL;
            return -1;
        }

        BaseFile *dir = data->fd_manager.get_fd(dir_fd);
        if (!dir)
            return -1;
        if (dir->type() != FileType::Directory)
        {
            error = ENOTDIR;
            return -1;
        }

        BaseFile *cloned_file = dir->clone();
        if (!cloned_file)
            return -1;
        assert(cloned_file->type() == FileType::Directory);

        const char *last = strrchr(path, '/');
        if (last)
        {
            String parent_path(path, last - path);

            BaseFile *parent = data->mounts.lopen(parent_path.c_str(), OPEN_RDONLY | OPEN_DIRECTORY, 0, (BaseDirectory *)cloned_file);
            if (!parent)
                return -1;
            assert(parent->type() == FileType::Directory);
            auto sym = ((BaseDirectory *)parent)->mksym(last + 1, target);
            dealloc(parent);
            dealloc(sym);
            return sym == nullptr ? -1 : 0;
        }
        else
        {
            // No parent, create a symlink in the directory itself
            auto sym = ((BaseDirectory *)cloned_file)->mksym(path, target);
            dealloc(cloned_file);
            dealloc(sym);
            return sym == nullptr ? -1 : 0;
        }
    }

    int VFS::mksfile(const char *path, int flags, BaseSpecialDriver *driver, int mode)
    {
        if (!path || !driver)
        {
            error = EINVAL;
            return -1;
        }

        const char *last = strrchr(path, '/');
        if (!last)
        {
            error = EINVAL;
            return -1;
        }

        String parent_path(path, last - path);

        BaseFile *parent = data->mounts.lopen(parent_path.c_str(), OPEN_RDONLY | OPEN_DIRECTORY, 0);
        if (!parent)
        {
            return -1;
        }

        assert(parent->type() == FileType::Directory);

        // Register the driver
        int driver_id = data->special_driver_manager.add_driver(driver);
        if (driver_id < 0)
        {
            dealloc(parent);
            return -1;
        }

        // Create the special file
        BaseSpecialFile *sfile = ((BaseDirectory *)parent)->mksfile(last + 1, flags, driver_id, mode);
        dealloc(parent);
        if (!sfile)
        {
            data->special_driver_manager.remove_driver(driver_id);
            return -1;
        }

        // Create the handle
        int fd = data->fd_manager.add_fd(sfile);
        if (fd < 0)
        {
            dealloc(sfile);
            data->special_driver_manager.remove_driver(driver_id);
            return -1;
        }

        return fd;
    }

    int VFS::mksfileat(int dir_fd, const char *path, int flags, BaseSpecialDriver *driver, int mode)
    {
        if (!path || !driver)
        {
            error = EINVAL;
            return -1;
        }

        BaseFile *dir = data->fd_manager.get_fd(dir_fd);
        if (!dir)
            return -1;
        if (dir->type() != FileType::Directory)
        {
            error = ENOTDIR;
            return -1;
        }

        BaseFile *cloned_file = dir->clone();
        if (!cloned_file)
            return -1;
        assert(cloned_file->type() == FileType::Directory);

        // Register the driver
        int driver_id = data->special_driver_manager.add_driver(driver);
        if (driver_id < 0)
        {
            dealloc(cloned_file);
            return -1;
        }

        // Create the special file
        const char *last = strrchr(path, '/');
        if (!last)
        {
            error = EINVAL;
            dealloc(cloned_file);
            return -1;
        }

        String parent_path(path, last - path);

        BaseFile *parent = data->mounts.lopen(parent_path.c_str(), OPEN_RDONLY | OPEN_DIRECTORY, 0, (BaseDirectory *)cloned_file);
        if (!parent)
            return -1;

        assert(parent->type() == FileType::Directory);
        auto sfile = ((BaseDirectory *)parent)->mksfile(last + 1, flags, driver_id, mode);
        dealloc(parent);

        if (!sfile)
        {
            data->special_driver_manager.remove_driver(driver_id);
            return -1;
        }

        // Create the handle
        int fd = data->fd_manager.add_fd(sfile);

        if (fd < 0)
        {
            dealloc(sfile);
            data->special_driver_manager.remove_driver(driver_id);
            return -1;
        }

        return fd;
    }

    int VFS::mkfile(const char *path, int mode)
    {
        int fd = mkfile(path, OPEN_RDONLY, mode);
        if (fd < 0)
        {
            return -1;
        }
        close(fd);
        return 0;
    }
    
    int VFS::mkfileat(int dir_fd, const char *path, int mode)
    {
        int fd = mkfileat(dir_fd, path, OPEN_RDONLY, mode);
        if (fd < 0)
        {
            return -1;
        }
        close(fd);
        return 0;
    }

    int VFS::mkdir(const char *path, int mode)
    {
        int fd = mkdir(path, OPEN_RDONLY, mode);
        if (fd < 0)
        {
            return -1;
        }
        close(fd);
        return 0;
    }

    int VFS::mkdirat(int dir_fd, const char *path, int mode)
    {
        int fd = mkdirat(dir_fd, path, OPEN_RDONLY, mode);
        if (fd < 0)
        {
            return -1;
        }
        close(fd);
        return 0;
    }

    int VFS::mksfile(const char *path, BaseSpecialDriver *driver, int mode)
    {
        int fd = mksfile(path, OPEN_RDONLY, driver, mode);
        if (fd < 0)
        {
            return -1;
        }
        close(fd);
        return 0;
    }

    int VFS::mksfileat(int dir_fd, const char *path, BaseSpecialDriver *driver, int mode)
    {
        int fd = mksfileat(dir_fd, path, OPEN_RDONLY, driver, mode);
        if (fd < 0)
        {
            return -1;
        }
        close(fd);
        return 0;
    }

    int VFS::ioctl(int fd, int req, IoctlArg arg)
    {
        BaseFile *file = data->fd_manager.get_fd(fd);
        if (!file)
            return -1;

        if (file->type() != FileType::Special)
        {
            error = ENOTTY;
            return -1;
        }
        
        BaseSpecialFile *sp_file = (BaseSpecialFile *)file;
        BaseSpecialDriverHandle *handle = sp_file->get_handle();
        if (!handle)
        {
            // Try to open the handle
            if (open_special_handle(sp_file, data->special_driver_manager) < 0)
                return -1;
            handle = sp_file->get_handle();
        }
        if (!handle)
            return -1;
        return handle->ioctl(req, arg);
    }

    int VFS::dup(int fd)
    {
        BaseFile *file = data->fd_manager.get_fd(fd);
        if (!file)
            return -1;

        BaseFile *cloned_file = file->clone();
        if (!cloned_file)
        {
            error = EIO;
            return -1;
        }

        int new_fd = data->fd_manager.add_fd(cloned_file);
        if (new_fd < 0)
        {
            dealloc(cloned_file);
            error = EIO;
            return -1;
        }

        return new_fd;
    }

    int VFS::set_flags(int fd, int flags)
    {
        BaseFile *file = data->fd_manager.get_fd(fd);
        if (!file)
            return -1;

        return file->set_flags(flags);
    }
} // namespace Hamster
