// Hamster VFS Implementation

#include <filesystem/vfs.hpp>
#include <memory/allocator.hpp>
#include <memory/stl_sequential.hpp>
#include <memory/stl_map.hpp>
#include <errno/errno.h>
#include <cstring>

namespace Hamster
{
    /* Mounts */
    namespace
    {
        class MountPoint
        {
        public:
            MountPoint(const char *path, BaseFilesystem *fs)
                : path(alloc<char>(strlen(path) + 1)), fs(fs)
            {
                strcpy(this->path, path);
            }

            MountPoint(const MountPoint &) = delete;
            MountPoint &operator=(const MountPoint &) = delete;

            MountPoint(MountPoint &&other)
                : path(other.path), fs(other.fs)
            {
                other.path = nullptr;
                other.fs = nullptr;
            }

            MountPoint &operator=(MountPoint &&other)
            {
                if (this == &other)
                    return *this;
                dealloc(path);
                dealloc(fs);
                path = other.path;
                fs = other.fs;
                other.path = nullptr;
                other.fs = nullptr;
                return *this;
            }

            ~MountPoint()
            {
                dealloc(path);
                dealloc(fs);

                path = nullptr;
                fs = nullptr;
            }

            char *path;
            BaseFilesystem *fs;
        };

        struct RelativePath
        {
            MountPoint *mount;
            const char *path;
        };

        class Mounts
        {
        public:
            Mounts() = default;

            Mounts(const Mounts &) = delete;
            Mounts &operator=(const Mounts &) = delete;

            Mounts(Mounts &&other)
                : mounts(std::move(other.mounts))
            {
                other.mounts.clear();
            }
            Mounts &operator=(Mounts &&other)
            {
                if (this == &other)
                    return *this;
                for (MountPoint *mp : mounts)
                {
                    dealloc(mp);
                }
                mounts.clear();
                mounts = std::move(other.mounts);
                other.mounts.clear();
                return *this;
            }

            ~Mounts()
            {
                for (MountPoint *mp : mounts)
                {
                    dealloc(mp);
                }
                mounts.clear();
            }

            // Takes ownership of `file`
            BaseDirectory *resolve_mount(BaseFile *file)
            {
                if (!file || file->type() != FileType::Directory)
                    return nullptr;

                uint16_t mount_id = (file->get_vfs_flags() & MOUNT_ID_MASK) >> 16;
                if (mount_id >= mounts.size())
                    return nullptr;
                MountPoint *mount = mounts[mount_id];
                if (!mount)
                    return nullptr;
                BaseDirectory *dir = mount->fs->open_root(file->get_flags());
                dealloc(file);
                return dir;
            }

            // This will follow all symlinks except for the last one
            // This takes ownership of `dir`
            BaseFile *lopen(const char *path, int flags, int mode, BaseDirectory *dir = nullptr)
            {
                if (!path)
                {
                    error = EINVAL;
                    dealloc(dir);
                    return nullptr;
                }

                if (!dir)
                {
                    // Open the root directory
                    MountPoint *root_mnt = nullptr;
                    for (MountPoint *mp : mounts)
                    {
                        if (!mp)
                            continue;
                        if (strcmp(mp->path, "/") == 0 || strcmp(mp->path, "") == 0)
                        {
                            root_mnt = mp;
                            break;
                        }
                    }

                    if (!root_mnt)
                    {
                        error = ENOENT;
                        return nullptr;
                    }

                    dir = root_mnt->fs->open_root(flags);
                    if (!dir)
                        return nullptr;
                }

                while (*path == '/')
                    ++path;

                if (*path == '\0')
                    return dir;

                if (path[0] == '.' && (path[1] == '/' || path[1] == '\0'))
                    return dir;

                if (path[0] == '.' && path[1] == '.' && (path[2] == '/' || path[2] == '\0'))
                {
                    dealloc(dir);
                    // '..' will fail at root, but not in the middle of a path because
                    // of the check below
                    error = ENOENT;
                    return nullptr;
                }

                const char *next = strchr(path, '/');

                // 'blah/..' will just return the current directory
                if (next && next[0] == '.' && next[1] == '.' && (next[2] == '/' || next[2] == '\0'))
                    return dir;

                if (!next)
                {
                    BaseFile *file = dir->get(path, flags, mode);
                    dealloc(dir);
                    if (!file)
                        return nullptr;

                    if (file->type() != FileType::Directory && (flags & O_DIRECTORY))
                    {
                        dealloc(file);
                        error = ENOTDIR;
                        return nullptr;
                    }

                    if (file->type() == FileType::Directory && file->get_vfs_flags() & FLAG_MOUNTPOINT)
                    {
                        // mountpoint
                        file = resolve_mount(file);
                    }
                    return file;
                }
                else
                {
                    String next_name(path, next - path);

                    BaseFile *next_file = dir->get(next_name.c_str(), (flags & ~O_CREAT & ~O_EXCL));
                    dealloc(dir);
                    if (!next_file)
                        return nullptr;

                    switch (next_file->type())
                    {
                    case FileType::Symlink:
                    {
                        BaseSymlink *link = (BaseSymlink *)next_file;
                        char *target = link->get_target();
                        dealloc(link);
                        if (!target)
                        {
                            error = ENOENT;
                            return nullptr;
                        }

                        BaseDirectory *dir = (BaseDirectory*)lopen(target, (flags & ~O_CREAT & ~O_EXCL) | O_DIRECTORY, mode);
                        dealloc(target);

                        if (!dir)
                        {
                            error = ENOENT;
                            return nullptr;
                        }

                        if (dir->get_vfs_flags() & FLAG_MOUNTPOINT)
                        {
                            dir = resolve_mount(dir);
                        }

                        BaseFile *file = lopen(next, flags, mode, dir);
                        return file;
                    }
                    case FileType::Directory:
                    {
                        BaseDirectory *next_dir = (BaseDirectory *)next_file;

                        if (next_dir->get_vfs_flags() & FLAG_MOUNTPOINT)
                        {
                            next_dir = resolve_mount(next_dir);
                        }

                        BaseFile *file = lopen(next, flags, mode, next_dir);
                        return file;
                    }
                    default:
                    {
                        dealloc(next_file);
                        error = ENOTDIR;
                        return nullptr;
                    }
                    }

                    // unreachable
                    dealloc(next_file);
                    return nullptr;
                }
            }

            int mount(const char *path, BaseFilesystem *fs)
            {
                if (!path || !fs)
                {
                    error = EINVAL;
                    return -1;
                }

                // too many mounts
                if (mounts.size() >= 0xFFFF)
                {
                    error = ENOSPC;
                    return -1;
                }

                BaseFile *file = lopen(path, O_RDONLY | O_DIRECTORY, 0);
                if (!file)
                    return -1;
                assert(file->type() == FileType::Directory);

                uint32_t flags = file->get_vfs_flags();
                if (flags & FLAG_MOUNTPOINT)
                {
                    dealloc(file);
                    error = EBUSY;
                    return -1;
                }

                // Get the first available slot
                uint16_t mount_id = mounts.size();
                for (uint16_t i = 0; i < mounts.size(); ++i)
                {
                    if (mounts[i] == nullptr)
                    {
                        mount_id = i;
                        break;
                    }
                }

                if (mount_id >= mounts.size())
                {
                    mounts.resize(mount_id + 1);
                }

                // Mount the filesystem
                MountPoint *mount = alloc<MountPoint>(1, path, fs);
                mounts[mount_id] = mount;

                // Set flags
                file->set_vfs_flags(flags | FLAG_MOUNTPOINT | (mount_id << 16));

                // OK
                dealloc(file);
                return 0;
            }

            int mount_root(BaseFilesystem *fs)
            {
                if (!fs)
                {
                    error = EINVAL;
                    return -1;
                }

                for (auto &mp : mounts)
                {
                    if (mp)
                    {
                        error = EBUSY;
                        return -1;
                    }
                }

                mounts.push_back(alloc<MountPoint>(1, "/", fs));

                // set the mountpoint flag on the root
                BaseFile *file = lopen("/", O_RDONLY | O_DIRECTORY, 0);
                if (!file)
                    return -1;
                uint32_t flags = file->get_vfs_flags();
                file->set_vfs_flags(flags | FLAG_MOUNTPOINT | (0 << 16));
                dealloc(file);

                return 0;
            }

            int unmount(const char *path)
            {
                if (!path)
                {
                    error = EINVAL;
                    return -1;
                }

                BaseFile *file = lopen(path, O_RDONLY | O_DIRECTORY, 0);
                if (!file)
                    return -1;

                uint32_t flags = file->get_vfs_flags();
                if (!(flags & FLAG_MOUNTPOINT))
                {
                    dealloc(file);
                    error = ENOTDIR;
                    return -1;
                }

                uint16_t mount_id = (flags & MOUNT_ID_MASK) >> 16;
                if (mount_id >= mounts.size() || mounts[mount_id] == nullptr)
                {
                    dealloc(file);
                    error = ENOENT;
                    return -1;
                }

                // Clear flags
                file->set_vfs_flags(flags & ~FLAG_MOUNTPOINT & ~MOUNT_ID_MASK);

                // Unmount the filesystem
                dealloc(mounts[mount_id]);
                mounts[mount_id] = nullptr;

                // OK
                dealloc(file);
                return 0;
            }

            // Set this to identify as a mountpoint
            static constexpr uint32_t FLAG_MOUNTPOINT = 1 << 0;

            // Upper 16 bits are the mount id
            static constexpr uint32_t MOUNT_ID_MASK = 0xFFFF << 16;

        private:
            Vector<MountPoint *> mounts;
        };
    } // namespace

    /* File Descriptor Manager */
    namespace
    {
        class FDManager
        {
            struct Entry
            {
                BaseFile *file;
            };

        public:
            FDManager() = default;
            FDManager(const FDManager &) = delete;
            FDManager &operator=(const FDManager &) = delete;
            ~FDManager()
            {
                close_all();
            }

            FDManager(FDManager &&other)
            {
                fds = std::move(other.fds);
                other.fds = Vector<Entry>();
            }

            FDManager &operator=(FDManager &&other)
            {
                if (this == &other)
                    return *this;
                std::swap(fds, other.fds);
                return *this;
            }

            // Takes ownership of file
            int add_fd(BaseFile *file)
            {
                if (file == nullptr)
                    return -1;

                int fd = -1;
                for (int i = 0; i < (int)fds.size(); ++i)
                {
                    if (fds[i].file == nullptr)
                    {
                        fd = i;
                        break;
                    }
                }
                if (fd == -1)
                {
                    fd = fds.size();
                    fds.push_back(Entry{nullptr});
                }
                fds[fd].file = file;

                return fd;
            }

            int remove_fd(int fd)
            {
                if (fd < 0 || fd >= (int)fds.size())
                {
                    error = EBADF;
                    return -1;
                }

                if (fds[fd].file == nullptr)
                {
                    error = EBADF;
                    return -1;
                }

                auto &file = fds[fd].file;
                if (file->type() == FileType::Special)
                {
                    BaseSpecialFile *sp_file = (BaseSpecialFile *)file;
                    BaseSpecialDriverHandle *handle = sp_file->get_handle();
                    dealloc(handle);
                    sp_file->set_handle(nullptr);
                }

                dealloc(file);
                file = nullptr;

                return 0;
            }

            BaseFile *get_fd(int fd)
            {
                if (fd < 0 || fd >= (int)fds.size())
                {
                    error = EBADF;
                    return nullptr;
                }

                if (fds[fd].file == nullptr)
                {
                    error = EBADF;
                    return nullptr;
                }

                return fds[fd].file;
            }

            void close_all()
            {
                for (auto &fd : fds)
                {
                    if (fd.file == nullptr)
                        continue;
                    if (fd.file->type() == FileType::Special)
                    {
                        // Clean up special file handles
                        BaseSpecialFile *sp_file = (BaseSpecialFile *)fd.file;
                        BaseSpecialDriverHandle *handle = sp_file->get_handle();
                        dealloc(handle);
                        sp_file->set_handle(nullptr);
                    }
                    dealloc(fd.file);
                    fd.file = nullptr;
                }
                fds.clear();
            }

        private:
            Vector<Entry> fds;
        };
    } // namespace

    /* Special File Driver Manager */

    namespace
    {
        class SpecialDriverManager
        {
        public:
            SpecialDriverManager() = default;
            SpecialDriverManager(const SpecialDriverManager &) = delete;
            SpecialDriverManager &operator=(const SpecialDriverManager &) = delete;

            SpecialDriverManager(SpecialDriverManager &&other)
                : drivers(std::move(other.drivers))
            {
                other.drivers = Vector<BaseSpecialDriver *>();
            }

            SpecialDriverManager &operator=(SpecialDriverManager &&other)
            {
                if (this == &other)
                    return *this;
                std::swap(drivers, other.drivers);
                return *this;
            }

            ~SpecialDriverManager()
            {
                remove_all_drivers();
            }

            int remove_all_drivers()
            {
                for (BaseSpecialDriver *driver : drivers)
                {
                    dealloc(driver);
                }
                drivers.clear();
                return 0;
            }

            int add_driver(BaseSpecialDriver *driver)
            {
                if (!driver)
                {
                    error = EINVAL;
                    return -1;
                }

                // Find the first available slot
                int id = -1;
                for (int i = 0; i < (int)drivers.size(); ++i)
                {
                    if (drivers[i] == nullptr)
                    {
                        id = i;
                        break;
                    }
                }
                if (id == -1)
                {
                    id = drivers.size();
                    drivers.push_back(nullptr);
                }
                drivers[id] = driver;
                return id;
            }

            int remove_driver(int id)
            {
                if (id < 0 || id >= (int)drivers.size())
                {
                    error = EINVAL;
                    return -1;
                }

                if (drivers[id] == nullptr)
                {
                    error = EINVAL;
                    return -1;
                }

                dealloc(drivers[id]);
                drivers[id] = nullptr;

                return 0;
            }

            BaseSpecialDriver *get_driver(int id)
            {
                if (id < 0 || id >= (int)drivers.size())
                {
                    error = EINVAL;
                    return nullptr;
                }

                if (drivers[id] == nullptr)
                {
                    error = EINVAL;
                    return nullptr;
                }

                return drivers[id];
            }

        private:
            Vector<BaseSpecialDriver *> drivers;
        };
    } // namespace

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
            if (flags & O_NOFOLLOW)
            {
                dealloc(file);
                error = ELOOP;
                return -1;
            }
            BaseSymlink *link = (BaseSymlink *)file;
            char *target = link->get_target();
            dealloc(link);

            if (!target)
            {
                error = ENOENT;
                return -1;
            }

            int fd = open(target, flags, mode);
            dealloc(target);
            return fd;
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

    int VFS::rename(const char *old_path, const char *new_path)
    {
        if (!old_path || !new_path)
        {
            error = EINVAL;
            return -1;
        }

        const char *last_old = strrchr(old_path, '/');
        const char *last_new = strrchr(new_path, '/');
        const char *old_name, *new_name;
        BaseDirectory *old_dir, *new_dir;
        if (last_old)
        {
            old_name = last_old + 1;
            String old_dir_name{old_path, (size_t)(last_old - old_path)};
            old_dir = (BaseDirectory*)data->mounts.lopen(old_dir_name.c_str(), O_RDONLY | O_DIRECTORY, 0);
        }
        else
        {
            old_name = old_path;
            old_dir = (BaseDirectory*)data->mounts.lopen("/", O_RDONLY | O_DIRECTORY, 0);
        }

        if (last_new)
        {
            new_name = last_new + 1;
            new_dir = (BaseDirectory*)data->mounts.lopen(String{new_path, (size_t)(last_new - new_path)}.c_str(), O_WRONLY | O_DIRECTORY, 0);
        }
        else
        {
            new_name = new_path;
            new_dir = (BaseDirectory*)data->mounts.lopen("/", O_WRONLY | O_DIRECTORY, 0);
        }

        if (!old_dir || !new_dir)
        {
            dealloc(old_dir);
            dealloc(new_dir);

            return -1;
        }

        assert(old_name && new_name);

        BaseFile *old_file = old_dir->get(old_name, O_RDONLY, 0);
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

    int VFS::remove(const char *path)
    {
        const char *last = strrchr(path, '/');
        if (!last)
        {
            error = EINVAL;
            return -1;
        }

        String dir_name{path, (size_t)(last - path)};
        BaseDirectory *dir = (BaseDirectory*)data->mounts.lopen(dir_name.c_str(), O_WRONLY, 0);

        if (!dir)
            return -1;

        int res = dir->remove(last + 1);
        dealloc(dir);
        return res;
    }

    int VFS::stat(int fd, struct ::stat *buf)
    {
        BaseFile *file = data->fd_manager.get_fd(fd);
        if (!file)
            return -1;

        int ret = file->stat(buf);
        return ret;
    }

    int VFS::lstat(const char *path, struct ::stat *buf)
    {
        BaseFile *file = data->mounts.lopen(path, O_RDONLY, 0);
        if (!file)
            return -1;

        int ret = file->stat(buf);
        dealloc(file);
        return ret;
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

    char *VFS::get_target(const char *path)
    {
        BaseFile *file = data->mounts.lopen(path, O_RDONLY, 0);
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

    int VFS::set_target(const char *path, const char *target)
    {
        if (!path || !target)
        {
            error = EINVAL;
            return -1;
        }

        BaseFile *file = data->mounts.lopen(path, O_WRONLY, 0);
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
            if (flags & O_NOFOLLOW)
            {
                dealloc(new_file);
                error = ELOOP;
                return -1;
            }
            BaseSymlink *link = (BaseSymlink *)new_file;
            char *target = link->get_target();
            dealloc(link);

            if (!target)
            {
                error = ENOENT;
                return -1;
            }

            int fd = openat(dir, target, flags, mode);
            dealloc(target);
            return fd;
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
        return open(path, flags | O_CREAT | O_EXCL, mode);
    }

    int VFS::mkfileat(int dir, const char *path, int flags, int mode)
    {
        return openat(dir, path, flags | O_CREAT | O_EXCL, mode);
    }

    int VFS::mkdir(const char *path, int flags, int mode)
    {
        return open(path, flags | O_CREAT | O_EXCL | O_DIRECTORY, mode);
    }

    int VFS::mkdirat(int dir, const char *path, int flags, int mode)
    {
        return openat(dir, path, flags | O_CREAT | O_EXCL | O_DIRECTORY, mode);
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

        BaseFile *parent = data->mounts.lopen(parent_path.c_str(), O_RDONLY | O_DIRECTORY, 0);
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

            BaseFile *parent = data->mounts.lopen(parent_path.c_str(), O_RDONLY | O_DIRECTORY, 0, (BaseDirectory *)cloned_file);
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

        BaseFile *parent = data->mounts.lopen(parent_path.c_str(), O_RDONLY | O_DIRECTORY, 0);
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

        BaseFile *parent = data->mounts.lopen(parent_path.c_str(), O_RDONLY | O_DIRECTORY, 0, (BaseDirectory *)cloned_file);
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
        int fd = mkfile(path, O_RDONLY, mode);
        if (fd < 0)
        {
            return -1;
        }
        close(fd);
        return 0;
    }
    
    int VFS::mkfileat(int dir_fd, const char *path, int mode)
    {
        int fd = mkfileat(dir_fd, path, O_RDONLY, mode);
        if (fd < 0)
        {
            return -1;
        }
        close(fd);
        return 0;
    }

    int VFS::mkdir(const char *path, int mode)
    {
        int fd = mkdir(path, O_RDONLY, mode);
        if (fd < 0)
        {
            return -1;
        }
        close(fd);
        return 0;
    }

    int VFS::mkdirat(int dir_fd, const char *path, int mode)
    {
        int fd = mkdirat(dir_fd, path, O_RDONLY, mode);
        if (fd < 0)
        {
            return -1;
        }
        close(fd);
        return 0;
    }

    int VFS::mksfile(const char *path, BaseSpecialDriver *driver, int mode)
    {
        int fd = mksfile(path, O_RDONLY, driver, mode);
        if (fd < 0)
        {
            return -1;
        }
        close(fd);
        return 0;
    }

    int VFS::mksfileat(int dir_fd, const char *path, BaseSpecialDriver *driver, int mode)
    {
        int fd = mksfileat(dir_fd, path, O_RDONLY, driver, mode);
        if (fd < 0)
        {
            return -1;
        }
        close(fd);
        return 0;
    }

    int VFS::isatty(int fd)
    {
        BaseFile *file = data->fd_manager.get_fd(fd);
        if (!file)
            return -1;

        if (file->type() != FileType::Special)
            return 0;
        
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
        if (handle->special_type() != SpecialFileType::CharacterDevice)
            return 0; // Not a character device
        BaseCharacterDeviceHandle *char_handle = (BaseCharacterDeviceHandle *)handle;
        return char_handle->isatty();
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
} // namespace Hamster
