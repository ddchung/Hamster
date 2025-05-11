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

                // Unmount the filesystem
                dealloc(mounts[mount_id]);
                mounts[mount_id] = nullptr;

                // Clear flags
                file->set_vfs_flags(flags & ~FLAG_MOUNTPOINT & ~MOUNT_ID_MASK);

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
            ~FDManager()
            {
                close_all();
            }

            // Takes ownership of file, but not mount
            // Mount is just for reference counting
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
            UnorderedMap<int, Entry> fds;
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
        dealloc(file);
        return ret;
    }

    int VFS::rename(const char *old_path, const char *new_path)
    {
        // TODO: Implement this. Note that this must move the file, not just rename it
        // This is a bit tricky as it must also handle moving between different filesystems
        // and mountpoints
        error = ENOTSUP;
        return -1;
    }

    int VFS::remove(int fd)
    {
        BaseFile *file = data->fd_manager.get_fd(fd);
        if (!file)
            return -1;

        int ret = file->remove();
        dealloc(file);
        return ret;
    }

    int VFS::stat(int fd, struct ::stat *buf)
    {
        BaseFile *file = data->fd_manager.get_fd(fd);
        if (!file)
            return -1;

        int ret = file->stat(buf);
        dealloc(file);
        return ret;
    }

    int VFS::lstat(const char *path, struct ::stat *buf)
    {
        BaseFile *file = data->mounts.lopen(path, O_RDONLY | O_DIRECTORY, 0);
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
        dealloc(file);
        return ret;
    }

    int VFS::get_flags(int fd)
    {
        BaseFile *file = data->fd_manager.get_fd(fd);
        if (!file)
            return -1;

        int ret = file->get_flags();
        dealloc(file);
        return ret;
    }

    int VFS::get_uid(int fd)
    {
        BaseFile *file = data->fd_manager.get_fd(fd);
        if (!file)
            return -1;

        int ret = file->get_uid();
        dealloc(file);
        return ret;
    }

    int VFS::get_gid(int fd)
    {
        BaseFile *file = data->fd_manager.get_fd(fd);
        if (!file)
            return -1;

        int ret = file->get_gid();
        dealloc(file);
        return ret;
    }

    int VFS::chmod(int fd, int mode)
    {
        BaseFile *file = data->fd_manager.get_fd(fd);
        if (!file)
            return -1;

        int ret = file->chmod(mode);
        dealloc(file);
        return ret;
    }

    int VFS::chown(int fd, int uid, int gid)
    {
        BaseFile *file = data->fd_manager.get_fd(fd);
        if (!file)
            return -1;

        int ret = file->chown(uid, gid);
        dealloc(file);
        return ret;
    }

    char *VFS::basename(int fd)
    {
        BaseFile *file = data->fd_manager.get_fd(fd);
        if (!file)
            return nullptr;

        char *ret = file->basename();
        dealloc(file);
        return ret;
    }

    ssize_t VFS::read(int fd, uint8_t *buf, size_t size)
    {
        BaseFile *file = data->fd_manager.get_fd(fd);
        if (!file)
            return -1;

        if (file->type() != FileType::Regular)
        {
            dealloc(file);
            error = EISDIR;
            return -1;
        }
        ssize_t ret = ((BaseRegularFile *)file)->read(buf, size);
        dealloc(file);
        return ret;
    }

    ssize_t VFS::write(int fd, const uint8_t *buf, size_t size)
    {
        BaseFile *file = data->fd_manager.get_fd(fd);
        if (!file)
            return -1;

        if (file->type() != FileType::Regular)
        {
            dealloc(file);
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
            dealloc(file);
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
            dealloc(file);
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
            dealloc(file);
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
            dealloc(file);
            error = EISDIR;
            return -1;
        }
        int64_t ret = ((BaseRegularFile *)file)->size();
        return ret;
    }

    char *VFS::get_target(const char *path)
    {
        BaseFile *file = data->mounts.lopen(path, O_RDONLY | O_DIRECTORY, 0);
        if (!file)
            return nullptr;

        if (file->type() != FileType::Symlink)
        {
            dealloc(file);
            error = EINVAL;
            return nullptr;
        }

        char *ret = ((BaseSymlink *)file)->get_target();
        return ret;
    }

    int VFS::set_target(int fd, const char *target)
    {
        BaseFile *file = data->fd_manager.get_fd(fd);
        if (!file)
            return -1;

        if (file->type() != FileType::Symlink)
        {
            dealloc(file);
            error = EINVAL;
            return -1;
        }

        int ret = ((BaseSymlink *)file)->set_target(target);
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

        BaseFile *new_file = data->mounts.lopen(path, flags, mode, (BaseDirectory *)file);
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
        ((BaseDirectory *)parent)->mksym(last + 1, target);
        dealloc(parent);
        return 0;
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

        const char *last = strrchr(path, '/');
        if (!last)
        {
            error = EINVAL;
            return -1;
        }

        String parent_path(path, last - path);

        BaseFile *parent = data->mounts.lopen(parent_path.c_str(), O_RDONLY | O_DIRECTORY, 0, (BaseDirectory *)dir);
        if (!parent)
            return -1;
        assert(parent->type() == FileType::Directory);
        ((BaseDirectory *)parent)->mksym(last + 1, target);
        dealloc(parent);
        return 0;
    }
} // namespace Hamster
