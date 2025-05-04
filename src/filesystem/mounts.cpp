// mounts

#include <filesystem/mounts.hpp>
#include <memory/allocator.hpp>
#include <errno/errno.h>
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

    BaseFile *Mounts::open(const char *path, int flags, va_list args)
    {
        Path mount = get_mount(path);
        if (mount.fs)
        {
            return mount.fs->open(mount.path, flags, args);
        }
        return nullptr;
    }

    int Mounts::rename(const char *old_path, const char *new_path)
    {
        Path old_mount = get_mount(old_path);
        Path new_mount = get_mount(new_path);
        if (!old_mount.fs || !new_mount.fs)
            return -1;
        if (old_mount.fs == new_mount.fs)
        {
            return old_mount.fs->rename(old_path, new_path);
        }

        // Different mounts
        
        struct ::stat old_stat;
        if (old_mount.fs->lstat(old_mount.path, &old_stat) < 0)
            return -1;
        
        if (S_ISLNK(old_stat.st_mode))
        {
            char *target = old_mount.fs->readlink(old_mount.path);
            if (!target)
                return -1;
            if (new_mount.fs->symlink(target, new_mount.path) < 0)
            {
                dealloc(target);
                return -1;
            }
            dealloc(target);
            return old_mount.fs->unlink(old_mount.path);
        }
        if (S_ISREG(old_stat.st_mode))
        {
            BaseRegularFile *new_file = new_mount.fs->mkfile(new_mount.path, O_RDWR, old_stat.st_mode);
            if (!new_file)
                return -1;
            BaseFile *old_file = old_mount.fs->open(old_mount.path, O_RDWR);
            if (!old_file)
            {
                dealloc(new_file);
                return -1;
            }
            assert(old_file->type() == FileType::Regular);
            BaseRegularFile *old_reg_file = (BaseRegularFile *)old_file;

            char buf[4096];
            ssize_t bytes_read;
            while ((bytes_read = old_reg_file->read((uint8_t *)buf, sizeof(buf))) > 0)
            {
                if (new_file->write((const uint8_t *)buf, bytes_read) < 0)
                {
                    dealloc(new_file);
                    dealloc(old_file);
                    return -1;
                }
            }
            if (bytes_read < 0)
            {
                dealloc(new_file);
                dealloc(old_file);
                return -1;
            }
            dealloc(new_file);
            dealloc(old_file);
            return old_mount.fs->unlink(old_mount.path);
        }
        if (S_ISDIR(old_stat.st_mode))
        {
            BaseDirectory *new_dir = new_mount.fs->mkdir(new_mount.path, O_RDWR, old_stat.st_mode);
            if (!new_dir)
                return -1;
            BaseDirectory *old_dir = (BaseDirectory*)old_mount.fs->open(old_mount.path, O_RDWR | O_DIRECTORY);
            if (!old_dir)
            {
                dealloc(new_dir);
                return -1;
            }

            char * const *list = old_dir->list();
            if (!list)
            {
                dealloc(new_dir);
                dealloc(old_dir);
                return -1;
            }

            for (size_t i = 0; list[i]; ++i)
            {
                String from{old_path};
                String to{new_path};

                from.append("/");
                from.append(list[i]);

                to.append("/");
                to.append(list[i]);

                if (rename(from.c_str(), to.c_str()) < 0)
                {
                    dealloc(new_dir);
                    dealloc(old_dir);
                    for (size_t j = 0; list[j]; ++j)
                        dealloc(list[j]);
                    dealloc(list);
                    return -1;
                }
            }

            for (size_t i = 0; list[i]; ++i)
                dealloc(list[i]);
            dealloc(list);
            dealloc(new_dir);
            dealloc(old_dir);
            return old_mount.fs->unlink(old_mount.path);
        }

        // Not a regular file, directory or symlink
        // Special files are not supported
        error = ENOTSUP;
        return -1;
    }

    int Mounts::unlink(const char *path)
    {
        Path mount = get_mount(path);
        if (mount.fs)
        {
            return mount.fs->unlink(mount.path);
        }
        return -1;
    }

    int Mounts::link(const char *old_path, const char *new_path)
    {
        Path old_mount = get_mount(old_path);
        Path new_mount = get_mount(new_path);
        if (!old_mount.fs || !new_mount.fs)
            return -1;
        if (old_mount.fs == new_mount.fs)
        {
            return old_mount.fs->link(old_path, new_path);
        }
        // Hard links crossing mountpoints are not supported
        error = ENOTSUP;
        return -1;
    }

    int Mounts::stat(const char *path, struct ::stat *buf)
    {
        Path mount = get_mount(path);
        if (mount.fs)
        {
            return mount.fs->stat(mount.path, buf);
        }
        return -1;
    }

    int Mounts::lstat(const char *path, struct ::stat *buf)
    {
        Path mount = get_mount(path);
        if (mount.fs)
        {
            return mount.fs->lstat(mount.path, buf);
        }
        return -1;
    }

    int Mounts::symlink(const char *old_path, const char *new_path)
    {
        Path mount = get_mount(new_path);
        if (mount.fs)
        {
            return mount.fs->symlink(old_path, mount.path);
        }
        return -1;
    }

    int Mounts::readlink(const char *path, char *buf, size_t buf_size)
    {
        Path mount = get_mount(path);
        if (mount.fs)
        {
            return mount.fs->readlink(mount.path, buf, buf_size);
        }
        return -1;
    }

    char *Mounts::readlink(const char *path)
    {
        Path mount = get_mount(path);
        if (mount.fs)
        {
            return mount.fs->readlink(mount.path);
        }
        return nullptr;
    }

    FileType Mounts::type(const char *path)
    {
        Path mount = get_mount(path);
        if (mount.fs)
        {
            return mount.fs->type(mount.path);
        }
        return FileType::Invalid;
    }

    BaseRegularFile *Mounts::mkfile(const char *name, int flags, int mode)
    {
        Path mount = get_mount(name);
        if (mount.fs)
        {
            return mount.fs->mkfile(mount.path, flags, mode);
        }
        return nullptr;
    }

    BaseDirectory *Mounts::mkdir(const char *name, int flags, int mode)
    {
        Path mount = get_mount(name);
        if (mount.fs)
        {
            return mount.fs->mkdir(mount.path, flags, mode);
        }
        return nullptr;
    }

    BaseSymbolicFile *Mounts::mksfile(const char *name, int flags, FileType type, int mode)
    {
        Path mount = get_mount(name);
        if (mount.fs)
        {
            return mount.fs->mksfile(mount.path, flags, type, mode);
        }
        return nullptr;
    }
} // namespace Hamster

