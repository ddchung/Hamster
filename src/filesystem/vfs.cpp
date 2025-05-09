// Hamster VFS Implementation

#include <filesystem/vfs.hpp>
#include <memory/allocator.hpp>
#include <memory/stl_sequential.hpp>
#include <memory/stl_map.hpp>
#include <errno/errno.h>
#include <cstring>

namespace Hamster
{
    namespace
    {
        // Resolves a path, possibly containing symbolic links, to a normalized absolute path.
        // returns a newly allocated string with the resolved path.
        char *resolve_path(const char *path, VFSData *data);
    }

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
            ~Mounts()
            {
                for (MountPoint *mp : mounts)
                {
                    dealloc(mp);
                }
                mounts.clear();
            }

            // Note: this function takes ownershiop of `fs`
            MountPoint *mount(const char *path, BaseFilesystem *fs)
            {
                for (MountPoint *mp : mounts)
                {
                    if (strcmp(mp->path, path) == 0)
                        return nullptr; // Already mounted
                }

                MountPoint *newMount = alloc<MountPoint>(1, path, fs);
                mounts.push_back(newMount);

                mounts.sort([](MountPoint *a, MountPoint *b)
                            { return strlen(a->path) > strlen(b->path); });

                return newMount;
            }

            int unmount(const char *path)
            {
                for (auto it = mounts.begin(); it != mounts.end(); ++it)
                {
                    if (strcmp((*it)->path, path) == 0)
                    {
                        dealloc(*it);
                        mounts.erase(it);
                        return 0;
                    }
                }
                return -1;
            }

            RelativePath resolve(const char *path)
            {
                for (MountPoint *mp : mounts)
                {
                    size_t len = strlen(mp->path);
                    if (strncmp(path, mp->path, len) == 0 &&
                        (path[len] == '/' || path[len] == '\0'))
                    {
                        const char *rel = path + len;
                        if (*rel == '/')
                            rel++;
                        return {mp, rel};
                    }
                }
                return {nullptr, path};
            }

        private:
            List<MountPoint *> mounts;
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

    VFS::VFS()
        : data(alloc<VFSData>())
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
        if (path == nullptr || path[0] != '/' || fs == nullptr)
        {
            error = EINVAL;
            return -1;
        }

        char *resolved_path = resolve_path(path, data);

        MountPoint *mp = data->mounts.mount(resolved_path, fs);

        dealloc(resolved_path);

        if (!mp)
            return -1;
        return 0;
    }

    int VFS::unmount(const char *path)
    {
        if (path == nullptr || path[0] != '/')
        {
            error = EINVAL;
            return -1;
        }

        char *resolved_path = resolve_path(path, data);

        RelativePath rel = data->mounts.resolve(resolved_path);
        if (rel.mount == nullptr)
        {
            dealloc(resolved_path);
            error = ENOENT;
            return -1;
        }
        if (!rel.path || rel.path[0] != '\0')
        {
            dealloc(resolved_path);
            // Does not exactly match a mount point
            error = EBUSY;
            return -1;
        }
        if (data->fd_manager.get_refcount(rel.mount) > 0)
        {
            dealloc(resolved_path);
            // Still has open file descriptors
            error = EBUSY;
            return -1;
        }

        if (data->mounts.unmount(resolved_path) == -1)
        {
            dealloc(resolved_path);
            error = EIO;
            return -1;
        }

        return 0;
    }
} // namespace Hamster
