// Root filesystem on SD card

#if defined(ARDUINO) && 1

#include <Arduino.h>
#include <SdFat.h>
#include <platform/platform.hpp>
#include <filesystem/file.hpp>
#include <filesystem/mounts.hpp>
#include <memory/stl_sequential.hpp>
#include <memory/allocator.hpp>
#include <errno/errno.h>
#include <cstring>
#include <cstdarg>
#include <utility>

// The root filesystem directory on the SD card
// All files will be in this directory in the SD card
#define ROOT_DIR "/rootfs"

using namespace Hamster;

namespace
{
    uint32_t dosDateTimeToUnix(uint16_t dosDate, uint16_t dosTime)
    {
        // Days per month for non-leap years
        static const uint16_t monthDays[12] = {
            0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

        // Decode the DOS date and time fields
        uint16_t year = ((dosDate >> 9) & 0x7F) + 1980;
        uint8_t month = (dosDate >> 5) & 0x0F;
        uint8_t day = dosDate & 0x1F;
        uint8_t hour = (dosTime >> 11) & 0x1F;
        uint8_t minute = (dosTime >> 5) & 0x3F;
        uint8_t second = (dosTime & 0x1F) * 2;

        // Calculate the number of days since 1970 (Unix epoch)
        uint32_t yearsSince1970 = year - 1970;
        uint32_t days = yearsSince1970 * 365 + (yearsSince1970 + 1) / 4;

        // Add days for months of the year (adjusting for leap year)
        days += monthDays[month - 1];

        // Adjust for leap year if it's after February
        if (month > 2 && (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)))
        {
            days += 1; // Leap year adjustment
        }

        // Add days from the current month
        days += day - 1;

        // Convert everything to seconds
        uint32_t totalSeconds = days * 86400UL + hour * 3600UL + minute * 60UL + second;

        return totalSeconds;
    }

    class SdCardFile
    {
    public:
        SdCardFile(SdFile &&file, int flags)
            : file(std::move(file)), flags(flags)
        {
        }

        ~SdCardFile()
        {
            file.close();
        }

        int rename(const char *new_name)
        {
            if (!file.isOpen())
            {
                error = EBADF;
                return -1;
            }
            if (!new_name)
            {
                error = EINVAL;
                return -1;
            }
            if ((flags & O_ACCMODE) == O_RDONLY)
            {
                error = EACCES;
                return -1;
            }
            return file.rename(new_name) ? 0 : (error = EIO, -1);
        }

        int remove()
        {
            if (!file.isOpen())
            {
                error = EBADF;
                return -1;
            }
            if ((flags & O_ACCMODE) == O_RDONLY)
            {
                error = EACCES;
                return -1;
            }
            return file.remove() ? 0 : (error = EIO, -1);
        }

        int stat(struct ::stat *buf)
        {
            if (!file.isOpen())
            {
                error = EBADF;
            }

            if (!buf)
            {
                error = EINVAL;
                return -1;
            }

            uint16_t ddate, dtime;

            buf->st_dev = 0;
            buf->st_ino = 0;
            buf->st_mode = file.isDir() ? S_IFDIR : S_IFREG;
            buf->st_nlink = 1;
            buf->st_uid = 0;
            buf->st_gid = 0;
            buf->st_rdev = 0;
            file.getAccessDateTime(&ddate, &dtime);
            buf->st_atime = dosDateTimeToUnix(ddate, dtime);
            file.getModifyDateTime(&ddate, &dtime);
            buf->st_mtime = dosDateTimeToUnix(ddate, dtime);
            file.getCreateDateTime(&ddate, &dtime);
            buf->st_ctime = dosDateTimeToUnix(ddate, dtime);
            buf->st_size = file.fileSize();
            buf->st_blksize = 512;
            buf->st_blocks = (buf->st_size + 511) / 512;
            buf->st_mode |= (file.isDir() ? S_IFDIR : S_IFREG) | 0777;

            return 0;
        }

        int mode()
        {
            if (!file.isOpen())
            {
                error = EBADF;
                return -1;
            }
            return (file.isDir() ? S_IFDIR : S_IFREG) | 0777;
        }

        int chmod(int mode)
        {
            error = ENOTSUP;
            return -1;
        }

        int chown(int uid, int gid)
        {
            error = ENOTSUP;
            return -1;
        }

        const char *name()
        {
            if (!file.isOpen())
            {
                error = EBADF;
                return nullptr;
            }
            name_buf.reserve(64);
            file.getName(name_buf.data(), name_buf.capacity());
            return name_buf.data();
        }

    protected:
        SdFile file;
        int flags;
        Hamster::String name_buf;
    };

    class SdCardRegularFile : public BaseRegularFile, private SdCardFile
    {
    public:
        using SdCardFile::SdCardFile;

        ~SdCardRegularFile() override = default;

        int rename(const char *new_name) override { return SdCardFile::rename(new_name); }
        int remove() override { return SdCardFile::remove(); }
        int stat(struct ::stat *buf) override { return SdCardFile::stat(buf); }
        int mode() override { return SdCardFile::mode(); }
        int chmod(int mode) override { return SdCardFile::chmod(mode); }
        int chown(int uid, int gid) override { return SdCardFile::chown(uid, gid); }
        const char *name() override { return SdCardFile::name(); }

        ssize_t read(uint8_t *buf, size_t size) override
        {
            if ((flags & O_ACCMODE) == O_WRONLY)
            {
                error = EACCES;
                return -1;
            }
            if (!file.isOpen())
            {
                error = EBADF;
                return -1;
            }
            int ret = file.read(buf, size);
            if (ret < 0)
            {
                error = EIO;
                return -1;
            }
            return ret;
        }

        ssize_t write(const uint8_t *buf, size_t size) override
        {
            if ((flags & O_ACCMODE) == O_RDONLY)
            {
                error = EACCES;
                return -1;
            }
            if (!file.isOpen())
            {
                error = EBADF;
                return -1;
            }
            size_t written = file.write(buf, size);
            if (written == 0 && file.getWriteError())
            {
                error = EIO;
                return -1;
            }
            return written;
        }

        int64_t seek(int64_t offset, int whence) override
        {
            if (!file.isOpen())
            {
                error = EBADF;
                return -1;
            }
            if (whence == SEEK_SET)
            {
                file.seekSet(offset);
            }
            else if (whence == SEEK_CUR)
            {
                file.seekCur(offset);
            }
            else if (whence == SEEK_END)
            {
                file.seekEnd(offset);
            }
            else
            {
                error = EINVAL;
                return -1;
            }
            return file.curPosition();
        }

        int64_t tell() override
        {
            if (!file.isOpen())
            {
                error = EBADF;
                return -1;
            }
            return file.curPosition();
        }

        int truncate(int64_t size) override
        {
            if ((flags & O_ACCMODE) == O_RDONLY)
            {
                error = EACCES;
                return -1;
            }
            if (!file.isOpen())
            {
                error = EBADF;
                return -1;
            }
            if (file.truncate(size))
            {
                return 0;
            }
            else
            {
                error = EIO;
                return -1;
            }
        }

        int64_t size() override
        {
            if (!file.isOpen())
            {
                error = EBADF;
                return -1;
            }
            return file.fileSize();
        }
    };

    class SdCardDirectory : public BaseDirectory
    {
    public:
        SdCardDirectory(const Hamster::String &path, int flags)
            : path(path), flags(flags)
        {
        }

        ~SdCardDirectory() override = default;

        int rename(const char *new_name) override
        {
            if ((flags & O_ACCMODE) == O_RDONLY)
            {
                error = EACCES;
                return -1;
            }
            if (!new_name)
            {
                error = EINVAL;
                return -1;
            }
            SdFile dir;
            if (!dir.open(path.c_str(), O_RDWR))
            {
                error = EBADF;
                return -1;
            }
            if (!dir.isDir())
            {
                error = ENOTDIR;
                return -1;
            }
            int ret = dir.rename(new_name) ? 0 : (error = EIO, -1);
            dir.close();
            return ret;
        }

        int remove() override
        {
            if ((flags & O_ACCMODE) == O_RDONLY)
            {
                error = EACCES;
                return -1;
            }
            SdFile dir;
            if (!dir.open(path.c_str(), O_RDWR))
            {
                error = EBADF;
                return -1;
            }
            if (!dir.isDir())
            {
                error = ENOTDIR;
                return -1;
            }
            return dir.remove() ? 0 : (error = EIO, -1);
        }

        int stat(struct ::stat *buf) override
        {
            if (!buf)
            {
                error = EINVAL;
                return -1;
            }
            SdFile dir;
            if (!dir.open(path.c_str(), O_RDWR))
            {
                error = EBADF;
                return -1;
            }
            if (!dir.isDir())
            {
                error = ENOTDIR;
                return -1;
            }
            if (!buf)
            {
                error = EINVAL;
                return -1;
            }

            uint16_t ddate, dtime;

            buf->st_dev = 0;
            buf->st_ino = 0;
            buf->st_mode = dir.isDir() ? S_IFDIR : S_IFREG;
            buf->st_nlink = 1;
            buf->st_uid = 0;
            buf->st_gid = 0;
            buf->st_rdev = 0;
            dir.getAccessDateTime(&ddate, &dtime);
            buf->st_atime = dosDateTimeToUnix(ddate, dtime);
            dir.getModifyDateTime(&ddate, &dtime);
            buf->st_mtime = dosDateTimeToUnix(ddate, dtime);
            dir.getCreateDateTime(&ddate, &dtime);
            buf->st_ctime = dosDateTimeToUnix(ddate, dtime);
            buf->st_size = dir.fileSize();
            buf->st_blksize = 512;
            buf->st_blocks = (buf->st_size + 511) / 512;
            buf->st_mode = S_IFDIR | 0777;

            return 0;
        }

        int mode() override
        {
            return S_IFDIR | 0777;
        }

        int chmod(int mode) override
        {
            error = ENOTSUP;
            return -1;
        }

        int chown(int uid, int gid) override
        {
            error = ENOTSUP;
            return -1;
        }

        const char *name() override
        {
            return path.c_str();
        }

        char *const *list() override
        {
            if ((flags & O_ACCMODE) == O_WRONLY)
            {
                error = EACCES;
                return nullptr;
            }

            SdFile dir;
            if (!dir.open(path.c_str(), O_RDONLY))
            {
                error = EBADF;
                return nullptr;
            }
            if (!dir.isDir())
            {
                error = ENOTDIR;
                return nullptr;
            }

            dir.rewindDirectory();
            Hamster::Vector<char *> names;

            // care should be taken to ensure re-entrency
            // please read directly after writing
            static char name_buf[64];

            SdFile entry;

            while (entry.openNext(&dir, O_RDONLY))
            {
                entry.getName(name_buf, sizeof(name_buf));
                names.push_back(alloc<char>(strlen(name_buf) + 1));
                strcpy(names.back(), name_buf);
                entry.close();
            }

            char **strings = alloc<char *>(names.size() + 1);
            memcpy(strings, names.data(), names.size() * sizeof(char *));
            strings[names.size()] = nullptr;
            return strings;
        }

        BaseFile *get(const char *name, int flags, ...) override
        {
            va_list args;
            va_start(args, flags);
            BaseFile *f = get(name, flags, args);
            va_end(args);
            return f;
        }

        BaseFile *get(const char *name, int flags, va_list args) override
        {
            if (!name)
            {
                error = EINVAL;
                return nullptr;
            }
            if (strchr(name, '/') != nullptr)
            {
                error = EINVAL;
                return nullptr;
            }
            SdFile dir;
            if (!dir.open(path.c_str(), O_RDONLY))
            {
                error = EBADF;
                return nullptr;
            }
            if (!dir.isDir())
            {
                error = ENOTDIR;
                return nullptr;
            }

            SdFile file;

            if (!file.open(&dir, name, flags))
            {
                error = ENOENT;
                return nullptr;
            }

            if (file.isDir())
            {
                file.close();
                return alloc<SdCardDirectory>(1, path + "/" + name, flags);
            }
            else
            {
                return alloc<SdCardRegularFile>(1, std::move(file), flags);
            }

            // should not happen
            return nullptr;
        }

        BaseFile *mkfile(const char *name, int flags, int mode) override
        {
            return get(name, flags | O_CREAT | O_TRUNC, mode);
        }

        BaseFile *mkdir(const char *name, int flags, int mode) override
        {
            if ((flags & O_ACCMODE) == O_RDONLY)
            {
                error = EACCES;
                return nullptr;
            }
            if (!name)
            {
                error = EINVAL;
                return nullptr;
            }
            if (strchr(name, '/') != nullptr)
            {
                error = EINVAL;
                return nullptr;
            }
            SdFile dir;
            if (!dir.open(path.c_str(), O_RDWR))
            {
                error = EBADF;
                return nullptr;
            }
            if (!dir.isDir())
            {
                error = ENOTDIR;
                return nullptr;
            }
            SdFile new_dir;
            if (!new_dir.mkdir(&dir, name))
            {
                error = EIO;
                return nullptr;
            }
            new_dir.close();
            return alloc<SdCardDirectory>(1, path + "/" + name, flags);
        }

        BaseFile *mkfifo(const char *name, int flags, int mode) override
        {
            // FIFO is not supported on SD card
            error = ENOTSUP;
            return nullptr;
        }

        int remove(const char *name) override
        {
            if ((flags & O_ACCMODE) == O_RDONLY)
            {
                error = EACCES;
                return -1;
            }
            if (!name)
            {
                error = EINVAL;
                return -1;
            }
            if (strchr(name, '/') != nullptr)
            {
                error = EINVAL;
                return -1;
            }
            SdFile dir;
            if (!dir.open(path.c_str(), O_RDWR))
            {
                error = EBADF;
                return -1;
            }
            if (!dir.isDir())
            {
                error = ENOTDIR;
                return -1;
            }
            return dir.remove(name) ? 0 : (error = EIO, -1);
        }

    private:
        Hamster::String path;
        int flags;
    };

    class SdCardFilesystem : public BaseFilesystem
    {
    public:
        SdCardFilesystem() = default;
        ~SdCardFilesystem() override = default;

        BaseFile *open(const char *path, int flags, ...) override
        {
            va_list args;
            va_start(args, flags);
            BaseFile *f = open(path, flags, args);
            va_end(args);
            return f;
        }

        BaseFile *open(const char *path, int flags, va_list args) override
        {
            if (!path)
            {
                error = EINVAL;
                return nullptr;
            }
            SdFile file;
            Hamster::String path_str = ROOT_DIR;
            path_str += "/";
            path_str += path;
            if (!file.open(path_str.c_str(), flags))
            {
                error = ENOENT;
                return nullptr;
            }
            if (file.isDir())
            {
                file.close();
                return alloc<SdCardDirectory>(1, path_str, flags);
            }
            else
            {
                return alloc<SdCardRegularFile>(1, std::move(file), flags);
            }
        }

        int rename(const char *old_path, const char *new_path) override
        {
            if (!old_path || !new_path || !*old_path || !*new_path)
            {
                error = EINVAL;
                return -1;
            }
            Hamster::String old_path_str = ROOT_DIR;
            Hamster::String new_path_str = ROOT_DIR;
            old_path_str += "/";
            new_path_str += "/";
            old_path_str += old_path;
            new_path_str += new_path;

            SdFile file;
            if (!file.open(old_path_str.c_str(), O_RDWR))
            {
                error = ENOENT;
                return -1;
            }

            int ret = file.rename(new_path_str.c_str()) ? 0 : (error = EIO, -1);
            file.close();
            return ret;
        }

        int link(const char *old_path, const char *new_path) override
        {
            error = ENOSYS;
            return -1;
        }

        int unlink(const char *path) override
        {
            if (!path)
            {
                error = EINVAL;
                return -1;
            }
            Hamster::String path_str = ROOT_DIR;
            path_str += "/";
            path_str += path;

            SdFile file;
            if (!file.open(path_str.c_str(), O_RDWR))
            {
                error = ENOENT;
                return -1;
            }

            int ret = file.remove() ? 0 : (error = EIO, -1);
            file.close();
            return ret;
        }

        int stat(const char *path, struct ::stat *buf) override
        {
            BaseFile *file = open(path, O_RDONLY);
            if (!file)
                return -1;
            int ret = file->stat(buf);
            dealloc(file);
            return ret;
        }

        BaseDirectory *mkdir(const char *path, int flags, int mode) override
        {
            if (!path)
            {
                error = EINVAL;
                return nullptr;
            }
            const char *last = strrchr(path, '/');
            if (!last)
            {
                error = EINVAL;
                return nullptr;
            }
            Hamster::String dir_path = ROOT_DIR;
            dir_path += "/";
            dir_path += Hamster::String(path, last - path);
            
            SdFile dir;
            if (!dir.open(dir_path.c_str(), O_RDWR))
            {
                error = ENOENT;
                return nullptr;
            }
            if (!dir.isDir())
            {
                error = ENOTDIR;
                return nullptr;
            }

            SdFile new_dir;
            if (!new_dir.mkdir(&dir, last + 1))
            {
                error = EIO;
                return nullptr;
            }
            assert(new_dir.isDir());
            new_dir.close();
            return alloc<SdCardDirectory>(1, dir_path + "/" + (last + 1), flags);
        }

        BaseRegularFile *mkfile(const char *path, int flags, int mode) override
        {
            BaseFile *f = open(path, (flags & ~O_DIRECTORY) | O_CREAT | O_TRUNC, mode);
            if (!f)
                return nullptr;
            assert(f->type() == FileType::Regular);
            return (BaseRegularFile *)f;
        }

        BaseFifoFile *mkfifo(const char *path, int flags, int mode) override
        {
            // FIFO is not supported on SD card
            error = ENOTSUP;
            return nullptr;
        }
    };
}

int Hamster::_mount_rootfs()
{
    return Hamster::Mounts::instance().mount(alloc<SdCardFilesystem>(1), "/");   
}

#endif // ARDUINO
