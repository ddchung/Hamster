// Hamster VFS Implementation

#include <filesystem/vfs.hpp>
#include <memory/allocator.hpp>
#include <memory/stl_sequential.hpp>
#include <memory/stl_map.hpp>
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

            const char *get_path() const
            {
                return path;
            }

            BaseFilesystem *get_fs() const
            {
                return fs;
            }

        private:
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
            int mount(const char *path, BaseFilesystem *fs);
            int unmount(const char *path);

            RelativePath resolve(const char *path);

        private:
            List<MountPoint> mounts;
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

            bool is_busy(MountPoint *mount)
            {
                auto it = mount_refcount.find(mount);
                if (it == mount_refcount.end())
                    return false;
                return it->second > 0;
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

