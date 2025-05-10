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
                : path(alloc<char>(strlen(path))), fs(fs)
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
                if (next[0] == '.' && next[1] == '.' && (next[2] == '/' || next[2] == '\0'))
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
                        case FileType::Directory:
                            BaseDirectory *next_dir = (BaseDirectory *)next;

                            if (next_dir->get_vfs_flags() & FLAG_MOUNTPOINT)
                            {
                                next_dir = resolve_mount(next_dir);
                            }

                            BaseFile *file = lopen(next_name.c_str(), flags, mode, next_dir);
                            dealloc(next_dir);
                            return file;
                        default:
                            dealloc(next);
                            error = ENOTDIR;
                            return nullptr;
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

                // Mount the filesystem
                MountPoint *mount = alloc<MountPoint>(1, path, fs);
                mounts[mount_id] = mount;

                // Set flags
                file->set_vfs_flags(flags | FLAG_MOUNTPOINT | (mount_id << 16));

                // OK
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

                // Unmount the filesystem
                dealloc(mounts[mount_id]);
                mounts[mount_id] = nullptr;

                // Clear flags
                file->set_vfs_flags(flags & ~FLAG_MOUNTPOINT & ~MOUNT_ID_MASK);

                // OK
                dealloc(file);
                return 0;
            }

        private:
            Vector<MountPoint *> mounts;

            // Set this to identify as a mountpoint
            static constexpr uint32_t FLAG_MOUNTPOINT = 1 << 0;

            // Upper 16 bits are the mount id
            static constexpr uint32_t MOUNT_ID_MASK = 0xFFFF << 16;
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
                MountPoint *mount;
            };

        public:
            ~FDManager()
            {
                close_all();
            }
            
            // Takes ownership of file, but not mount
            // Mount is just for reference counting
            int add_fd(BaseFile *file, MountPoint *mount)
            {
                if (file == nullptr || mount == nullptr)
                    return -1;

                // handle overflow
                if (next_fd < 0)
                    next_fd = 0;

                // handle overflow
                while (fds.find(next_fd) != fds.end())
                    ++next_fd;

                fds[next_fd] = {file, mount};
                mount_refcount[mount]++;
                return next_fd++;
            }

            int remove_fd(int fd)
            {
                auto it = fds.find(fd);
                if (it == fds.end())
                    return -1;
                auto &entry = it->second;

                --mount_refcount[entry.mount];
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
                mount_refcount.clear();
                next_fd = 0;
            }

            bool is_busy()
            {
                for (auto &ref : mount_refcount)
                {
                    if (ref.second > 0)
                        return true;
                }
                return false;
            }

            size_t get_refcount(MountPoint *mount)
            {
                auto it = mount_refcount.find(mount);
                if (it == mount_refcount.end())
                    return 0;
                return it->second;
            }

        private:
            UnorderedMap<int, Entry> fds;
            UnorderedMap<MountPoint *, size_t> mount_refcount;
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
} // namespace Hamster
