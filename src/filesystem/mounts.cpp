// mounts

#include <filesystem/mounts.hpp>
#include <memory/allocator.hpp>
#include <cstring>
#include <fcntl.h>

namespace Hamster
{
    Mounts::Mount::Mount(BaseFilesystem *fs, const char *path)
        : fs(fs), path(nullptr)
    {
        this->path = alloc<char>(strlen(path) + 1);
        strcpy(this->path, path);
    }

    Mounts::Mount::Mount(Mount &&mount)
        : fs(mount.fs), path(mount.path)
    {
        mount.fs = nullptr;
        mount.path = nullptr;
    }

    Mounts::Mount &Mounts::Mount::operator=(Mount &&mount)
    {
        if (this != &mount)
        {
            dealloc(this->path);
            this->path = mount.path;
            mount.path = nullptr;

            dealloc(this->fs);
            this->fs = mount.fs;
            mount.fs = nullptr;
        }
        return *this;
    }

    Mounts::Mount::~Mount()
    {
        dealloc(this->path);
        this->path = nullptr;
        dealloc(this->fs);
        this->fs = nullptr;
    }

    int Mounts::mount(BaseFilesystem *fs, const char *path)
    {
        if (!fs || !path)
            return -1;

        if (!is_valid(path))
            return -2;

        if (is_mounted(path))
            return -3;
        
        Path mount = get_mount(path);
        if (mount.fs && mount.path)
        {
            // check if there is a directory there
            BaseFile *file = mount.fs->open(mount.path, O_RDONLY);
            if (!file || file->type() != FileType::Directory)
            {
                // mountpoint is not valid or does not exist
                dealloc(file);
                return -4;
            }
            dealloc(file);
        }

        size_t len = strlen(path);
        
        // insert mountpoints with longest path first
        for (auto it = mounts.begin(); it != mounts.end(); ++it)
        {
            if (strlen(it->path) > len)
                continue;
            mounts.insert(it, Mount(fs, path));
            return 0;   
        }
        
        mounts.push_back(Mount(fs, path));
        return 0;
    }

    int Mounts::umount(const char *path)
    {
        if (!path)
            return -1;
        if (!is_valid(path))
            return -2;
        
        auto it = mounts.begin();
        while (it != mounts.end())
        {
            if (strcmp(it->path, path) == 0)
            {
                mounts.erase(it);
                return 0;
            }
            ++it;
        }
        return -3;
    }

    bool Mounts::is_valid(const char *path)
    {
        if (!path)
            return false;
        if (strlen(path) == 0)
            return false;
        if (path[0] != '/')
            return false;
        return true;
    }

    bool Mounts::is_mounted(const char *path)
    {
        if (!path)
            return false;
        if (!is_valid(path))
            return false;
        
        for (const auto &mount : mounts)
        {
            if (strcmp(mount.path, path) == 0)
                return true;
        }
        return false;
    }

    Mounts::Path Mounts::get_mount(const char *path)
    {
        if (!path)
            return {nullptr, nullptr};
        if (!is_valid(path))
            return {nullptr, nullptr};
        
        for (const auto &mount : mounts)
        {
            size_t len = strlen(mount.path);
            if (strncmp(mount.path, path, len) == 0)
            {
                if (path[len] == '/')
                    return {mount.fs, path + len};
                else
                    return {mount.fs, path + len - (len != 0)};
            }
        }
        return {nullptr, path};
    }

    BaseFile *Mounts::open(const char *path, int flags, ...)
    {
        va_list args;
        va_start(args, flags);
        BaseFile *file = open(path, flags, args);
        va_end(args);
        return file;
    }

    BaseFile *Mounts::open(const char *path, int flags, va_list args)
    {
        if (!path)
            return nullptr;
        Path mount = get_mount(path);
        if (!mount.fs)
            return nullptr;
        return mount.fs->open(mount.path, flags, args);
    }

    int Mounts::rename(const char *old_path, const char *new_path)
    {
        if (!old_path || !new_path)
            return -1;
        Path old_mount = get_mount(old_path);
        Path new_mount = get_mount(new_path);
        if (!old_mount.fs || !new_mount.fs)
            return -1;
        if (old_mount.fs != new_mount.fs)
            return -2;
        return old_mount.fs->rename(old_mount.path, new_mount.path);
    }

    int Mounts::unlink(const char *path)
    {
        if (!path)
            return -1;
        Path mount = get_mount(path);
        if (!mount.fs)
            return -1;
        return mount.fs->unlink(mount.path);
    }

    int Mounts::link(const char *old_path, const char *new_path)
    {
        if (!old_path || !new_path)
            return -1;
        Path old_mount = get_mount(old_path);
        Path new_mount = get_mount(new_path);
        if (!old_mount.fs || !new_mount.fs)
            return -1;
        if (old_mount.fs != new_mount.fs)
            return -2;
        return old_mount.fs->link(old_mount.path, new_mount.path);
    }

    int Mounts::stat(const char *path, struct ::stat *buf)
    {
        if (!path || !buf)
            return -1;
        Path mount = get_mount(path);
        if (!mount.fs)
            return -2;
        return mount.fs->stat(mount.path, buf);
    }

    BaseRegularFile *Mounts::mkfile(const char *name, int flags, int mode)
    {
        if (!name)
            return nullptr;
        Path mount = get_mount(name);
        if (!mount.fs)
            return nullptr;
        return mount.fs->mkfile(mount.path, flags, mode);
    }

    BaseDirectory *Mounts::mkdir(const char *name, int flags, int mode)
    {
        if (!name)
            return nullptr;
        Path mount = get_mount(name);
        if (!mount.fs)
            return nullptr;
        return mount.fs->mkdir(mount.path, flags, mode);
    }

    BaseFifoFile *Mounts::mkfifo(const char *name, int flags, int mode)
    {
        if (!name)
            return nullptr;
        Path mount = get_mount(name);
        if (!mount.fs)
            return nullptr;
        return mount.fs->mkfifo(mount.path, flags, mode);
    }
} // namespace Hamster

