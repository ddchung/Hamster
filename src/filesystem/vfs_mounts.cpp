#include <filesystem/vfs_mounts.hpp>
#include <memory/allocator.hpp>
#include <cstring>
#include <errno/errno.h>
#include <cassert>

namespace Hamster
{

    MountPoint::MountPoint(const char *path, BaseFilesystem *fs)
        : path(alloc<char>(strlen(path) + 1)), fs(fs)
    {
        strcpy(this->path, path);
    }
    MountPoint::MountPoint(MountPoint &&other)
        : path(other.path), fs(other.fs)
    {
        other.path = nullptr;
        other.fs = nullptr;
    }
    MountPoint &MountPoint::operator=(MountPoint &&other)
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
    MountPoint::~MountPoint()
    {
        dealloc(path);
        dealloc(fs);
        path = nullptr;
        fs = nullptr;
    }

    Mounts::Mounts() = default;
    Mounts::Mounts(Mounts &&other) : mounts(std::move(other.mounts)) { other.mounts.clear(); }
    Mounts &Mounts::operator=(Mounts &&other)
    {
        if (this == &other)
            return *this;
        for (MountPoint *mp : mounts)
            dealloc(mp);
        mounts.clear();
        mounts = std::move(other.mounts);
        other.mounts.clear();
        return *this;
    }
    Mounts::~Mounts()
    {
        for (MountPoint *mp : mounts)
            dealloc(mp);
        mounts.clear();
    }

    BaseDirectory *Mounts::resolve_mount(BaseFile *file)
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

    BaseFile *Mounts::lopen(const char *path, int flags, int mode, BaseDirectory *dir)
    {
        if (!path)
        {
            error = EINVAL;
            dealloc(dir);
            return nullptr;
        }
        if (!dir)
        {
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
        const char *next = strchr(path, '/');
        if (!next)
        {
            BaseFile *file = dir->get(path, flags, mode);
            dealloc(dir);
            if (!file)
                return nullptr;
            if (file->type() != FileType::Directory && (flags & OPEN_DIRECTORY))
            {
                dealloc(file);
                error = ENOTDIR;
                return nullptr;
            }
            if (file->type() == FileType::Directory && file->get_vfs_flags() & FLAG_MOUNTPOINT)
                file = resolve_mount(file);
            return file;
        }
        else
        {
            String next_name(path, next - path);
            BaseFile *next_file = dir->get(next_name.c_str(), (flags & ~OPEN_CREAT & ~OPEN_EXCL));
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
                BaseDirectory *ndir = (BaseDirectory *)lopen(target, (flags & ~OPEN_CREAT & ~OPEN_EXCL) | OPEN_DIRECTORY, mode);
                dealloc(target);
                if (!ndir)
                {
                    error = ENOENT;
                    return nullptr;
                }
                if (ndir->get_vfs_flags() & FLAG_MOUNTPOINT)
                    ndir = resolve_mount(ndir);
                BaseFile *file = lopen(next, flags, mode, ndir);
                return file;
            }
            case FileType::Directory:
            {
                BaseDirectory *next_dir = (BaseDirectory *)next_file;
                if (next_dir->get_vfs_flags() & FLAG_MOUNTPOINT)
                    next_dir = resolve_mount(next_dir);
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
            dealloc(next_file);
            return nullptr;
        }
    }

    int Mounts::mount(const char *path, BaseFilesystem *fs)
    {
        if (!path || !fs)
        {
            error = EINVAL;
            return -1;
        }
        if (mounts.size() >= 0xFFFF)
        {
            error = ENOSPC;
            return -1;
        }
        BaseFile *file = lopen(path, OPEN_RDONLY | OPEN_DIRECTORY, 0);
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
            mounts.resize(mount_id + 1);
        MountPoint *mount = alloc<MountPoint>(1, path, fs);
        mounts[mount_id] = mount;
        file->set_vfs_flags(flags | FLAG_MOUNTPOINT | (mount_id << 16));
        dealloc(file);
        return 0;
    }

    int Mounts::mount_root(BaseFilesystem *fs)
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
        BaseFile *file = lopen("/", OPEN_RDONLY | OPEN_DIRECTORY, 0);
        if (!file)
            return -1;
        uint32_t flags = file->get_vfs_flags();
        file->set_vfs_flags(flags | FLAG_MOUNTPOINT | (0 << 16));
        dealloc(file);
        return 0;
    }

    int Mounts::unmount(const char *path)
    {
        if (!path)
        {
            error = EINVAL;
            return -1;
        }
        BaseFile *file = lopen(path, OPEN_RDONLY | OPEN_DIRECTORY, 0);
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
        file->set_vfs_flags(flags & ~FLAG_MOUNTPOINT & ~MOUNT_ID_MASK);
        dealloc(mounts[mount_id]);
        mounts[mount_id] = nullptr;
        dealloc(file);
        return 0;
    }

} // namespace Hamster
