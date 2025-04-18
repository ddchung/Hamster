// Root filesystem on SD card

#if defined(ARDUINO) && 1

#include <Arduino.h>
#include <SD.h>
#include <platform/platform.hpp>
#include <filesystem/file.hpp>
#include <filesystem/mounts.hpp>
#include <memory/stl_sequential.hpp>
#include <memory/allocator.hpp>
#include <cstring>
#include <cstdarg>


// The root filesystem's path
#define ROOTFS_PATH "/rootfs"

namespace
{
    class SDCardFile : public Hamster::BaseRegularFile
    {
    public:
        SDCardFile(File file)
            : file(file)
        {
        }

        ~SDCardFile() override
        {
            file.close();
        }

        int rename(const char *new_name) override
        {
            // TODO: implement rename
            // Arduino SD lib does not support rename
            return -1;
        }

        int remove() override
        {
            // TODO: implement remove
            // Arduino SD lib does not support remove
            return -1;
        }

        int64_t seek(int64_t offset, int whence) override
        {
            file.seek(offset, whence);
            return file.position();
        }

        int read(uint8_t *buf, size_t size) override
        {
            return file.read(buf, size);
        }

        int write(const uint8_t *buf, size_t size) override
        {
            return file.write(buf, size);
        }
    private:
        File file;
    };

    class SDCardDirectory : public Hamster::BaseDirectory
    {
    public:
        SDCardDirectory(const char *path)
            : path(path)
        {
        }

        ~SDCardDirectory() override = default;

        int rename(const char *new_name) override
        {
            return SD.rename(path.c_str(), new_name) ? 0 : -1;
        }

        int remove() override
        {
            return SD.rmdir(path.c_str()) ? 0 : -1;
        }

        char * const *list() override
        {
            Hamster::Vector<Hamster::String> files;
            File dir = SD.open(path.c_str());
            if (!dir)
                return nullptr;
            File entry = dir.openNextFile();
            while (entry)
            {
                const char *name = strrchr(entry.name(), '/');
                if (name)
                    name++;
                else
                    name = entry.name();
                files.push_back(name);
                entry = dir.openNextFile();
            }
            dir.close();
            char **list = Hamster::alloc<char *>(files.size() + 1);
            for (size_t i = 0; i < files.size(); i++)
            {
                list[i] = Hamster::alloc<char>(files[i].length() + 1);
                strcpy(list[i], files[i].c_str());
            }
            list[files.size()] = nullptr;
            return list;
        }

        Hamster::BaseFile *get(const char *name, int flags, ...) override
        {
            size_t len = strlen(name);
            size_t path_len = path.length();
            char *full_path = Hamster::alloc<char>(path_len + len + 2);
            snprintf(full_path, path_len + len + 2, "%s/%s", path.c_str(), name);
            
            SDCardFile *file = Hamster::alloc<SDCardFile>(1, SD.open(full_path, flags));
            Hamster::dealloc(full_path);
            return file;
        }

        Hamster::BaseFile *mkfile(const char *name, int flags, int mode) override
        {
            Hamster::String full_path = path + "/" + name;
            char *next = strchr(full_path.data(), '/');
            while (next)
            {
                *next = '\0';
                SD.mkdir(full_path.c_str());
                *next = '/';
                next = strchr(next + 1, '/');
            }

            flags |= O_CREAT | O_EXCL;
            flags &= ~O_DIRECTORY;
            return get(name, flags);
        }

        Hamster::BaseFile *mkdir(const char *name, int flags, int mode) override
        {
            Hamster::String full_path = path + "/" + name;
            char *next = strchr(full_path.data(), '/');
            while (next)
            {
                *next = '\0';
                SD.mkdir(full_path.c_str());
                *next = '/';
                next = strchr(next + 1, '/');
            }

            if (SD.mkdir(full_path.c_str()))
                return Hamster::alloc<SDCardDirectory>(1, full_path.c_str());
            return nullptr;
        }

        Hamster::BaseFile *mkfifo(const char *name, int flags, int mode) override
        {
            // SD card does not support FIFO
            return nullptr;
        }

        int remove(const char *name) override
        {
            size_t len = strlen(name);
            size_t path_len = path.length();
            char *full_path = Hamster::alloc<char>(path_len + len + 2);
            snprintf(full_path, path_len + len + 2, "%s/%s", path.c_str(), name);

            int ret = SD.remove(full_path) ? 0 : -1;
            Hamster::dealloc(full_path);
            return ret;
        }
    private:
        Hamster::String path;
    };

    class SDCardFS : public Hamster::BaseFilesystem
    {
    public:
        SDCardFS() = default;
        ~SDCardFS() override = default;

        Hamster::BaseFile *open(const char *path, int flags, ...) override
        {
            va_list args;
            va_start(args, flags);
            Hamster::BaseFile *file = open(path, flags, args);
            va_end(args);
            return file;
        }

        Hamster::BaseFile *open(const char *path, int flags, va_list args) override
        {
            char *full_path = Hamster::alloc<char>(sizeof(ROOTFS_PATH) + 1 + strlen(path) + 1);
            snprintf(full_path, sizeof(ROOTFS_PATH) + 1 + strlen(path) + 1, "%s/%s", ROOTFS_PATH, path);
            File file = SD.open(full_path, flags);

            if (!file)
            {
                Hamster::dealloc(full_path);
                return nullptr;
            }

            if (file.isDirectory())
            {
                file.close();
                SDCardDirectory *dir = Hamster::alloc<SDCardDirectory>(1, full_path);
                Hamster::dealloc(full_path);
                return dir;
            }
            else
            {
                Hamster::dealloc(full_path);
                return Hamster::alloc<SDCardFile>(1, file);
            }
        }

        int rename(const char *old_path, const char *new_path) override
        {
            char *old_full_path = Hamster::alloc<char>(sizeof(ROOTFS_PATH) + 1 + strlen(old_path) + 1);
            snprintf(old_full_path, sizeof(ROOTFS_PATH) + 1 + strlen(old_path) + 1, "%s/%s", ROOTFS_PATH, old_path);

            char *new_full_path = Hamster::alloc<char>(sizeof(ROOTFS_PATH) + 1 + strlen(new_path) + 1);
            snprintf(new_full_path, sizeof(ROOTFS_PATH) + 1 + strlen(new_path) + 1, "%s/%s", ROOTFS_PATH, new_path);

            int ret = SD.rename(old_full_path, new_full_path) ? 0 : -1;
            Hamster::dealloc(old_full_path);
            Hamster::dealloc(new_full_path);
            return ret;
        }

        int unlink(const char *path) override
        {
            char *full_path = Hamster::alloc<char>(sizeof(ROOTFS_PATH) + 1 + strlen(path) + 1);
            snprintf(full_path, sizeof(ROOTFS_PATH) + 1 + strlen(path) + 1, "%s/%s", ROOTFS_PATH, path);

            int ret = SD.remove(full_path) ? 0 : -1;
            Hamster::dealloc(full_path);
            return ret;
        }

        int link(const char *old_path, const char *new_path) override
        {
            // SD card does not support link
            return -1;
        }

        int stat(const char *path, struct ::stat *buf) override
        {
            char *full_path = Hamster::alloc<char>(sizeof(ROOTFS_PATH) + 1 + strlen(path) + 1);
            snprintf(full_path, sizeof(ROOTFS_PATH) + 1 + strlen(path) + 1, "%s/%s", ROOTFS_PATH, path);
            File file = SD.open(full_path);
            Hamster::dealloc(full_path);
            if (!file)
                return -1;
            
            buf->st_size = file.size();
            buf->st_mode = file.isDirectory() ? S_IFDIR : S_IFREG;
            buf->st_mode |= 0777; // rwxrwxrwx
            buf->st_atime = buf->st_mtime = buf->st_ctime = 0; // not supported
            buf->st_uid = buf->st_gid = 0; // not supported
            buf->st_nlink = 1; // not supported
            buf->st_dev = 0; // not supported
            buf->st_ino = 0; // not supported
            buf->st_rdev = 0; // not supported
            file.close();
            return 0;
        }

        Hamster::BaseRegularFile *mkfile(const char *name, int flags, int mode) override
        {
            char *full_path = Hamster::alloc<char>(sizeof(ROOTFS_PATH) + 1 + strlen(name) + 1);
            snprintf(full_path, sizeof(ROOTFS_PATH) + 1 + strlen(name) + 1, "%s/%s", ROOTFS_PATH, name);

            char *next = strchr(full_path, '/');
            while (next)
            {
                *next = '\0';
                SD.mkdir(full_path);
                *next = '/';
                next = strchr(next + 1, '/');
            }
            flags |= O_CREAT | O_EXCL;
            flags &= ~O_DIRECTORY;

            printf("Creating file %s\n", full_path);

            File file = SD.open(full_path, flags);
            Hamster::dealloc(full_path);
            if (!file)
                return nullptr;
            assert(!file.isDirectory());
            return Hamster::alloc<SDCardFile>(1, file);
        }

        Hamster::BaseDirectory *mkdir(const char *name, int flags, int mode) override
        {
            char *full_path = Hamster::alloc<char>(sizeof(ROOTFS_PATH) + 1 + strlen(name) + 1);
            snprintf(full_path, sizeof(ROOTFS_PATH) + 1 + strlen(name) + 1, "%s/%s", ROOTFS_PATH, name);

            char *next = strchr(full_path, '/');
            while (next)
            {
                *next = '\0';
                SD.mkdir(full_path);
                *next = '/';
                next = strchr(next + 1, '/');
            }
            if (!SD.mkdir(full_path))
            {
                Hamster::dealloc(full_path);
                return nullptr;
            }
            Hamster::BaseDirectory *dir = Hamster::alloc<SDCardDirectory>(1, full_path);
            Hamster::dealloc(full_path);
            return dir;
        }

        Hamster::BaseFifoFile *mkfifo(const char *name, int flags, int mode) override
        {
            // SD card does not support FIFO
            return nullptr;
        }
    };
}

int Hamster::_mount_rootfs()
{
    // Mount the root filesystem
    SDCardFS *fs = Hamster::alloc<SDCardFS>(1);
    if (Hamster::Mounts::instance().mount(fs, "/") < 0)
    {
        Hamster::dealloc(fs);
        return -1;
    }
    return 0;
}

#endif // ARDUINO
