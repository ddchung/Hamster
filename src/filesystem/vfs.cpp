// Hamster VFS Implementation

#include <filesystem/vfs.hpp>
#include <memory/allocator.hpp>
#include <memory/stl_sequential.hpp>
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
        public:
            int add_fd(BaseFile *file);
            int remove_fd(int fd);
            BaseFile *get_fd(int fd);
            void close_all();
            bool is_busy();
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

