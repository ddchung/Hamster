#include <filesystem/vfs_mounts.hpp>
#include <memory/allocator.hpp>
#include <memory/stl_sequential.hpp>
#include <cstring>
#include <errno/errno.h>
#include <cassert>

namespace Hamster
{
    namespace
    {
        int check_access(int fsuid, int fsgid, int fsmode, int uid, int *groups, size_t numgroups, int mode)
        {
            bool ok = false;

            if (fsuid == uid)
            {
                ok = !(mode & ~((fsmode >> 6) & 0b111));
            }
            else
            {
                bool found = false;
                for (size_t i = 0; i < numgroups; ++i)
                {
                    if (fsgid == groups[i])
                    {
                        found = true;

                        ok = !(mode & ~((fsmode >> 3) & 0b111));
                        break;
                    }
                }

                if (!found)
                    ok = !(mode & ~((fsmode) & 0b111));
            }

            if (ok)
                return 0;
            error = H_EACCES;
            return -1;
        }
    } // namespace
    

    MountPoint::MountPoint(BaseFilesystem *fs)
        : fs(fs), children(0), parent(0)
    {
    }
    MountPoint::MountPoint(MountPoint &&other)
        : fs(other.fs), children(other.children), parent(other.parent)
    {
        other.fs = nullptr;
        other.children = 0;
    }
    MountPoint &MountPoint::operator=(MountPoint &&other)
    {
        if (this == &other)
            return *this;
        dealloc(fs);
        fs = other.fs;
        children = other.children;
        parent = other.parent;
        other.children = 0;
        other.parent = nullptr;
        other.fs = nullptr;
        return *this;
    }
    MountPoint::~MountPoint()
    {
        dealloc(fs);
        fs = nullptr;
    }

    Mounts::Mounts()
        : root_mount(nullptr)
    {
    }

    Mounts::Mounts(Mounts &&other) 
        : mounts(std::move(other.mounts)), root_mount(other.root_mount)
    {
        other.mounts.clear();
        other.root_mount = nullptr;
    }

    Mounts &Mounts::operator=(Mounts &&other)
    {
        if (this == &other)
            return *this;
        dealloc(root_mount);
        root_mount = other.root_mount;
        other.root_mount = nullptr;
        mounts.clear();
        mounts = std::move(other.mounts);
        other.mounts.clear();
        return *this;
    }
    Mounts::~Mounts()
    {
        dealloc(root_mount);
        root_mount = nullptr;
        mounts.clear();
    }

    BaseDirectory *Mounts::resolve_mount(BaseFile *file)
    {
        if (!file || file->type() != FileType::Directory)
            return nullptr;
        int id = file->get_id();
        auto it = mounts.find(id);
        if (it == mounts.end())
        {
            // Not a mount point
            return (BaseDirectory*)file;
        }
        MountPoint &mount = it->second;
        if (!mount.fs)
        {
            error = H_ENOENT;
            dealloc(file);
            return nullptr;
        }

        BaseDirectory *dir = mount.fs->open_root(file->get_flags());
        dealloc(file);
        return dir;
    }

    BaseFile *Mounts::resolve_symlink(BaseSymlink *link, int flags, BaseDirectory *dir)
    {
        if (!link)
        {
            error = H_EBADF;
            return nullptr;
        }
        char *target = link->get_target();
        int link_id = link->get_id();
        dealloc(link);
        if (!target)
            return nullptr;
        if (target[0] == '/')
        {
            // If the path is absolute, use root directory
            dealloc(dir);
            dir = nullptr;
        }
        BaseFile *file = lopen(target, (flags & ~OPEN_DIRECTORY & ~OPEN_CREAT & ~OPEN_EXCL) | OPEN_NOFOLLOW, 0, dir);
        dealloc(target);

        if (!file)
            return nullptr;
        
        int file_id = file->get_id();

        if (file_id != -1 && link_id != -1 && file_id == link_id)
        {
            // Self-targeting symlink
            error = H_ELOOP;
            dealloc(file);
            return nullptr;
        }

        if (file->type() == FileType::Symlink)
            return resolve_symlink((BaseSymlink *)file, flags);

        if (file->type() != FileType::Directory && (flags & OPEN_DIRECTORY))
        {
            error = H_ENOTDIR;
            dealloc(file);
            return nullptr;
        }
        return file;
    }

    BaseFile *Mounts::lopen(const char *path, int flags, int mode, BaseDirectory *dir)
    {
        if (!path)
        {
            error = H_EINVAL;
            dealloc(dir);
            return nullptr;
        }
        if (!dir)
        {
            if (!root_mount)
            {
                error = H_ENOENT;
                return nullptr;
            }
            dir = root_mount->fs->open_root(flags);
            if (!dir)
                return nullptr;
        }
        while (*path == '/')
            ++path;
        if (*path == '\0')
            return dir;
        const char *next = strchr(path, '/');
        if (!next)
        {
            int get_flags = flags;
            if ((flags & OPEN_CREAT) == 0) get_flags &= ~OPEN_DIRECTORY;
            BaseFile *file = dir->get(path, get_flags, mode);
            if (!file)
            {
                dealloc(dir);
                return nullptr;
            }
            if (file->type() == FileType::Symlink && !(flags & OPEN_NOFOLLOW))
            {
                file = resolve_symlink((BaseSymlink *)file, flags, dir);
                dir = nullptr;
                if (!file)
                {
                    return nullptr;
                }
            }
            dealloc(dir);
            if (file->type() != FileType::Directory && (flags & OPEN_DIRECTORY))
            {
                dealloc(file);
                error = H_ENOTDIR;
                return nullptr;
            }
            return file->type() == FileType::Directory ? resolve_mount(file) : file;
        }
        else
        {
            String next_name(path, next - path);
            BaseFile *next_file = dir->get(next_name.c_str(), (flags & ~OPEN_CREAT & ~OPEN_EXCL & ~OPEN_DIRECTORY));
            if (!next_file)
            {
                dealloc(dir);
                return nullptr;
            }
            switch (next_file->type())
            {
            case FileType::Symlink:
                next_file = resolve_symlink((BaseSymlink *)next_file, (flags & ~OPEN_CREAT & ~OPEN_EXCL) | OPEN_DIRECTORY, dir);
                dir = nullptr;
                if (!next_file)
                {
                    return nullptr;
                }
                // fallthrough to directory handling
            [[fallthrough]];
            case FileType::Directory:
            {
                dealloc(dir);
                BaseDirectory *next_dir = (BaseDirectory *)next_file;
                next_dir = resolve_mount(next_dir);
                if (!next_dir)
                {
                    dealloc(next_file);
                    error = H_ENOENT;
                    return nullptr;
                }
                BaseFile *file = lopen(next, flags, mode, next_dir);
                return file;
            }
            default:
                break;
            }
            // non-directory in middle of path
            dealloc(next_file);
            dealloc(dir);
            error = H_ENOTDIR;
            return nullptr;
        }
    }

    int Mounts::access(const char *path, int uid, int *groups, size_t numgroups, int mode, BaseDirectory *dir, int flags)
    {
        if (!path)
        {
            error = H_EINVAL;
            dealloc(dir);
            return -1;
        }
        if (!dir)
        {
            if (!root_mount)
            {
                error = H_ENOENT;
                return -1;
            }
            dir = root_mount->fs->open_root(OPEN_RDONLY);
            if (!dir)
                return -1;
        }
        int fsmode = dir->get_mode();
        int fsuid = dir->get_uid();
        int fsgid = dir->get_gid();
        if (fsmode < 0 || fsuid < 0 || fsgid < 0 ||
            // Check with search permission
            check_access(fsuid, fsgid, fsmode, uid, groups, numgroups, 0b001) < 0)
        {
            dealloc(dir);
            return -1;
        }
        while (*path == '/')
            ++path;
        if (*path == '\0')
        {
            // Already checked
            dealloc(dir);
            return 0;
        }
        const char *next = strchr(path, '/');
        if (!next)
        {
            BaseFile *file = dir->get(path, OPEN_RDONLY);
            if (file && file->type() == FileType::Directory)
                file = resolve_mount(file);
            if (!file)
            {
                dealloc(dir);
                return -1;
            }
            if (file->type() == FileType::Symlink && !(flags & H_AT_SYMLINK_NOFOLLOW))
            {
                file = resolve_symlink((BaseSymlink *)file, OPEN_RDONLY, dir);
                dir = nullptr;
                if (!file)
                {
                    return -1;
                }
            }
            dealloc(dir);

            int fsmode = file->get_mode();
            int fsuid = file->get_uid();
            int fsgid = file->get_gid();
            dealloc(file);
            if (fsmode < 0 || fsuid < 0 || fsgid < 0) return -1;
            return check_access(fsuid, fsgid, fsmode, uid, groups, numgroups, mode);
        }
        else
        {
            String next_name(path, next - path);
            BaseFile *next_file = dir->get(next_name.c_str(), OPEN_RDONLY);
            if (!next_file)
            {
                dealloc(dir);
                return -1;
            }
            switch (next_file->type())
            {
            case FileType::Symlink:
                next_file = resolve_symlink((BaseSymlink *)next_file, OPEN_RDONLY | OPEN_DIRECTORY, dir);
                dir = nullptr;
                if (!next_file)
                    return -1;
                // fallthrough to directory handling
            [[fallthrough]];
            case FileType::Directory:
            {
                dealloc(dir);
                BaseDirectory *next_dir = (BaseDirectory *)next_file;
                next_dir = resolve_mount(next_dir);
                if (!next_dir)
                {
                    dealloc(next_file);
                    error = H_ENOENT;
                    return -1;
                }

                return access(next, uid, groups, numgroups, mode, next_dir, flags);
            }
            default:
                break;
            }
            // non-directory in middle of path
            dealloc(next_file);
            dealloc(dir);
            error = H_ENOTDIR;
            return -1;
        }
    }

    int Mounts::mount(const char *path, BaseFilesystem *fs)
    {
        if (!path || !fs)
        {
            error = H_EINVAL;
            return -1;
        }
        if (mounts.size() >= 0xFFFF)
        {
            error = H_ENOSPC;
            return -1;
        }
        BaseFile *file = lopen(path, OPEN_RDONLY | OPEN_DIRECTORY, 0);
        if (!file)
            return -1;
        assert(file->type() == FileType::Directory);

        int id = file->get_id();
        if (id == -1)
        {
            dealloc(file);
            return -1;
        }

        auto it = mounts.find(id);
        if (it != mounts.end())
        {
            error = H_EBUSY;
            dealloc(file);
            return -1;
        }

        mounts.emplace(id, MountPoint(fs));

        // Increment the children count of the parent mount point
        BaseFilesystem *parent_fs = file->get_filesystem();
        if (parent_fs)
        {
            BaseDirectory *parent_dir = parent_fs->open_root(file->get_flags());
            if (parent_dir)
            {
                int pid = parent_dir->get_id();

                auto pit = mounts.find(pid);
                if (pit != mounts.end())
                {
                    MountPoint &parent_mount = pit->second;
                    parent_mount.children++;

                    it = mounts.find(id);
                    assert(it != mounts.end());
                    it->second.parent = &parent_mount;
                }
            }
            dealloc(parent_dir);
        }

        dealloc(file);
        return 0;
    }

    int Mounts::mount_root(BaseFilesystem *fs)
    {
        if (!fs)
        {
            error = H_EINVAL;
            return -1;
        }
        if (root_mount)
        {
            error = H_EBUSY;
            return -1;
        }
        mounts.clear();
        root_mount = alloc<MountPoint>(1, fs);
        return 0;
    }

    int Mounts::unmount(const char *path)
    {
        if (!path)
        {
            error = H_EINVAL;
            return -1;
        }
        BaseFile *file = lopen(path, OPEN_RDONLY | OPEN_DIRECTORY, 0);
        if (!file)
            return -1;
        assert(file->type() == FileType::Directory);
        int id = file->get_id();
        dealloc(file);
        if (id == -1)
        {
            return -1;
        }

        auto it = mounts.find(id);
        if (it == mounts.end())
        {
            error = H_ENOENT;
            return -1;
        }

        if (it->second.children > 0)
        {
            error = H_EBUSY;
            return -1;
        }

        it->second.parent->children--;

        mounts.erase(id);

        return 0;
    }

    int Mounts::unmount_root()
    {
        if (!root_mount)
        {
            error = H_ENOENT;
            return -1;
        }

        if (root_mount->children > 0)
        {
            error = H_EBUSY;
            return -1;
        }

        dealloc(root_mount);
        root_mount = nullptr;

        return 0;
    }

} // namespace Hamster
