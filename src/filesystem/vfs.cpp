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
                    return nullptr;

                if (!dir)
                {
                    // Open the root directory
                    MountPoint *root_mnt = nullptr;
                    for (MountPoint *mp : mounts)
                    {
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

                    BaseFile *next = dir->get(next_name.c_str(), (flags & ~O_CREAT & ~O_EXCL));
                    dealloc(dir);
                    if (!next)
                        return nullptr;

                    switch (next->type())
                    {
                    case FileType::Symlink:
                    {
                        BaseSymlink *link = (BaseSymlink *)next;
                        char *target = link->get_target();
                        dealloc(link);
                        if (!target)
                        {
                            error = ENOENT;
                            return nullptr;
                        }

                        BaseFile *f = lopen(target, flags, mode);
                        dealloc(target);
                        return f;
                    }
                    case FileType::Directory:
                    {
                        BaseDirectory *next_dir = (BaseDirectory *)next;

                        if (next_dir->get_vfs_flags() & FLAG_MOUNTPOINT)
                        {
                            next_dir = resolve_mount(next_dir);
                        }

                        BaseFile *file = lopen(next_name.c_str(), flags, mode, next_dir);
                        return file;
                    }
                    default:
                    {
                        dealloc(next);
                        error = ENOTDIR;
                        return nullptr;
                    }
                    }

                    // unreachable
                    dealloc(next);
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

                if (mounts.size() > 0)
                {
                    error = EIO;
                    return -1;
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
                next_fd = other.next_fd;
                other.fds = Map<int, Entry>();
            }

            FDManager &operator=(FDManager &&other)
            {
                if (this == &other)
                    return *this;
                std::swap(fds, other.fds);
                std::swap(next_fd, other.next_fd);
                return *this;
            }

            // Takes ownership of file
            int add_fd(BaseFile *file)
            {
                if (file == nullptr)
                    return -1;

                // handle overflow
                if (next_fd < 0)
                    next_fd = 0;

                // handle overflow
                while (fds.find(next_fd) != fds.end())
                    ++next_fd;

                fds[next_fd] = {file};
                return next_fd++;
            }

            int remove_fd(int fd)
            {
                auto it = fds.find(fd);
                if (it == fds.end())
                    return -1;

                dealloc(it->second.file);

                fds.erase(it);

                return 0;
            }

            BaseFile *get_fd(int fd)
            {
                auto it = fds.find(fd);
                if (it == fds.end())
                    return nullptr;
                return it->second.file;
            }

            void close_all()
            {
                for (auto &fd : fds)
                {
                    dealloc(fd.second.file);
                }
                fds.clear();
                next_fd = 0;
            }

        private:
            Map<int, Entry> fds;
            int next_fd = 0;
        };
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

    int VFS::rename(int fd, const char *new_name)
    {
        BaseFile *file = data->fd_manager.get_fd(fd);
        if (!file)
            return -1;

        int ret = file->rename(new_name);
        return ret;
    }

    int VFS::rename(const char *old_path, const char *new_path)
    {
        if (!old_path || !new_path)
        {
            error = EINVAL;
            return -1;
        }

        BaseFile *old_file = data->mounts.lopen(old_path, O_RDONLY, 0);
        if (!old_file)
            return -1;

        // Ensure that the new path does not exist
        BaseFile *new_file = data->mounts.lopen(new_path, O_WRONLY | O_CREAT | O_EXCL, 0);
        if (!new_file)
        {
            dealloc(old_file);
            return -1;
        }
        new_file->remove();
        dealloc(new_file);
        new_file = nullptr;

        switch (old_file->type())
        {
        case FileType::Regular:
        {
            BaseRegularFile *old_reg_file = (BaseRegularFile *)old_file;
            BaseRegularFile *new_reg_file =
                (BaseRegularFile *)data->mounts.lopen(
                    new_path, O_WRONLY | O_CREAT | O_EXCL,
                    old_file->get_mode());
            if (!new_reg_file)
            {
                dealloc(old_file);
                return -1;
            }

            // Copy contents
            static constexpr size_t BUF_SIZE = 512;
            static uint8_t buf[BUF_SIZE];
            ssize_t bytes_read;
            while ((bytes_read = old_reg_file->read(buf, BUF_SIZE)) > 0)
            {
                new_reg_file->write(buf, bytes_read);
            }

            if (bytes_read >= 0)
                old_file->remove();
            dealloc(old_reg_file);
            dealloc(new_reg_file);
            return bytes_read >= 0 ? 0 : -1;
        }
        case FileType::Symlink:
        {
            BaseSymlink *old_symlink = (BaseSymlink *)old_file;
            char *target = old_symlink->get_target();

            if (!target)
            {
                dealloc(old_symlink);
                return -1;
            }

            int ret = symlink(new_path, target);
            dealloc(target);
            if (ret == 0)
                old_symlink->remove();
            dealloc(old_symlink);
            return ret;
        }
        case FileType::Special:
        {
            BaseSpecialFile *old_special_file = (BaseSpecialFile *)old_file;
            int dev_id = old_special_file->get_device_id();

            const char *last_slash = strrchr(new_path, '/');
            if (!last_slash)
            {
                dealloc(old_special_file);

                error = ENOENT;
                return -1;
            }
            String new_parent(last_slash, last_slash - new_path);
            BaseDirectory *new_parent_file = (BaseDirectory *)data->mounts.lopen(new_parent.c_str(), O_RDWR | O_DIRECTORY, 0);
            if (!new_parent_file)
            {
                dealloc(old_special_file);

                error = ENOENT;
                return -1;
            }

            BaseSpecialFile *new_special_file = new_parent_file->mksfile(
                last_slash + 1, dev_id, old_special_file->get_mode());
            if (new_special_file)
            {
                old_special_file->remove();
                dealloc(old_special_file);
                dealloc(new_special_file);
            }
            dealloc(new_parent_file);
            return new_special_file ? 0 : -1;
        }
        case FileType::Directory:
        {
            // TODO: Recursive move
            error = ENOTSUP;
            dealloc(old_file);
            return -1;
        }
        default:
            dealloc(old_file);
            error = EINVAL;
            return -1;
        }
    }

    int VFS::remove(int fd)
    {
        BaseFile *file = data->fd_manager.get_fd(fd);
        if (!file)
            return -1;

        int ret = file->remove();
        return ret;
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

    char *VFS::basename(int fd)
    {
        BaseFile *file = data->fd_manager.get_fd(fd);
        if (!file)
            return nullptr;

        char *ret = file->basename();
        return ret;
    }

    ssize_t VFS::read(int fd, uint8_t *buf, size_t size)
    {
        BaseFile *file = data->fd_manager.get_fd(fd);
        if (!file)
            return -1;

        if (file->type() != FileType::Regular)
        {
            error = EISDIR;
            return -1;
        }
        ssize_t ret = ((BaseRegularFile *)file)->read(buf, size);
        return ret;
    }

    ssize_t VFS::write(int fd, const uint8_t *buf, size_t size)
    {
        BaseFile *file = data->fd_manager.get_fd(fd);
        if (!file)
            return -1;

        if (file->type() != FileType::Regular)
        {
            error = EISDIR;
            return -1;
        }
        ssize_t ret = ((BaseRegularFile *)file)->write(buf, size);
        return ret;
    }

    int VFS::seek(int fd, int64_t offset, int whence)
    {
        BaseFile *file = data->fd_manager.get_fd(fd);
        if (!file)
            return -1;

        if (file->type() != FileType::Regular)
        {
            error = EISDIR;
            return -1;
        }
        int ret = ((BaseRegularFile *)file)->seek(offset, whence);
        return ret;
    }

    int64_t VFS::tell(int fd)
    {
        BaseFile *file = data->fd_manager.get_fd(fd);
        if (!file)
            return -1;

        if (file->type() != FileType::Regular)
        {
            error = EISDIR;
            return -1;
        }
        int64_t ret = ((BaseRegularFile *)file)->tell();
        return ret;
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

        if (file->type() != FileType::Regular)
        {
            error = EISDIR;
            return -1;
        }
        int64_t ret = ((BaseRegularFile *)file)->size();
        return ret;
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

    char *const *VFS::list(int fd)
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

        char *const *ret = ((BaseDirectory *)file)->list();
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
        if (!last)
        {
            error = EINVAL;
            return -1;
        }

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
} // namespace Hamster
