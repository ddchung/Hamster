// mounts

#include <filesystem/mounts.hpp>
#include <memory/allocator.hpp>
#include <cstring>

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
} // namespace Hamster

