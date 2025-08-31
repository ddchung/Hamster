// Native version

#if !defined(ARDUINO) && 1

#include <platform/platform.hpp>
#include <filesystem/vfs.hpp>
#include <filesystem/ramfs.hpp>
#include <filesystem/device_manager.hpp>
#include <driver/base_tty.hpp>
#include <abi/values.hpp>
#include <memory/allocator.hpp>
#include <errno/errno.h>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#include <dirent.h>
#include <cstdarg>
#include <queue>
#include <string>
#include <time.h>

using namespace Hamster;

#define HAMSTER_NATIVE_FS_ROOT "/home/tin/hamster_rootfs"

namespace
{
    void swap_error()
    {
        // Swap the error code with the global error code
        int err = Hamster::error;
        Hamster::error = errno;
        errno = err;
    }

    class NativeFileHandle
    {
    public:
        NativeFileHandle(int fd, BaseFilesystem *fs)
            : fd(fd), filesystem(fs) {}
        
        ~NativeFileHandle()
        {
            if (fd >= 0)
            {
                close(fd);
            }
            fd = -1;
            filesystem = nullptr;
        }

        BaseFilesystem *get_filesystem()
        { return filesystem; }

        int get_id() const
        {
            if (fd < 0)
            {
                error = EBADF;
                return -1;
            }

            struct stat st;
            if (fstat(fd, &st) < 0)
            {
                swap_error();
                return -1;
            }

            return st.st_ino; // Return inode number as ID
        }

        int stat(sys_stat *buf)
        {
            if (fd < 0)
            {
                error = EBADF;
                return -1;
            }

            struct stat st;
            if (fstat(fd, &st) < 0)
            {
                swap_error();
                return -1;
            }

            buf->dev = st.st_dev;
            buf->ino = st.st_ino;
            buf->mode = st.st_mode & 07777;
            buf->nlink = st.st_nlink;
            buf->uid = st.st_uid;
            buf->gid = st.st_gid;
            buf->size = st.st_size;
            buf->atime = st.st_atime;
            buf->mtime = st.st_mtime;
            buf->ctime = st.st_ctime;

            if (S_ISREG(st.st_mode))
            {
                buf->mode |= STAT_IFREG;
            }
            else if (S_ISDIR(st.st_mode))
            {
                buf->mode |= STAT_IFDIR;
            }
            // Skip Character and Block devices
            else if (S_ISFIFO(st.st_mode))
            {
                buf->mode |= STAT_IFIFO;
            }
            else if (S_ISLNK(st.st_mode))
            {
                buf->mode |= STAT_IFLNK;
            }
            else if (S_ISSOCK(st.st_mode))
            {
                buf->mode |= STAT_IFSOCK;
            }

            return 0; // Success
        }

        int get_mode()
        {
            sys_stat st;
            if (stat(&st) < 0)
            {
                return -1;
            }
            return st.mode;
        }

        int get_flags()
        {
            if (fd < 0)
            {
                error = EBADF;
                return -1;
            }

            int flags = fcntl(fd, F_GETFL);
            if (flags < 0)
            {
                swap_error();
                return -1;
            }
            return flags;
        }

        int get_uid()
        {
            sys_stat st;
            if (stat(&st) < 0)
            {
                return -1;
            }
            return st.uid;
        }

        int get_gid()
        {
            sys_stat st;
            if (stat(&st) < 0)
            {
                return -1;
            }
            return st.gid;
        }

        int chmod(int mode)
        {
            if (fd < 0)
            {
                error = EBADF;
                return -1;
            }

            if (fchmod(fd, mode) < 0)
            {
                swap_error();
                return -1;
            }
            return 0; // Success
        }

        int chown(int uid, int gid)
        {
            if (fd < 0)
            {
                error = EBADF;
                return -1;
            }

            if (fchown(fd, uid, gid) < 0)
            {
                swap_error();
                return -1;
            }
            return 0; // Success
        }

        int set_flags(int flags)
        {
            if (fd < 0)
            {
                error = EBADF;
                return -1;
            }

            if (fcntl(fd, F_SETFL, flags) < 0)
            {
                swap_error();
                return -1;
            }
            return 0; // Success
        }

        // not actually from basefile, but used to get the protected fd
        int get_fd() const
        {
            return fd;
        }

    protected:
        int fd;
        BaseFilesystem *filesystem;
    };

    class NativeRegularFileHandle : public BaseRegularFile, public NativeFileHandle
    {
    public:
        using NativeFileHandle::NativeFileHandle;

        // Common to all file types

        BaseFile *clone() override
        {
            int new_fd = dup(fd);
            if (new_fd < 0)
            {
                swap_error();
                return nullptr;
            }
            return alloc<NativeRegularFileHandle>(1, new_fd, filesystem);
        }

        BaseFilesystem *get_filesystem() override { return NativeFileHandle::get_filesystem(); }
        int get_id() const override { return NativeFileHandle::get_id(); }
        int stat(sys_stat *buf) override { return NativeFileHandle::stat(buf); }
        int get_mode() override { return NativeFileHandle::get_mode(); }
        int get_flags() override { return NativeFileHandle::get_flags(); }
        int get_uid() override { return NativeFileHandle::get_uid(); }
        int get_gid() override { return NativeFileHandle::get_gid(); }
        int chmod(int mode) override { return NativeFileHandle::chmod(mode); }
        int chown(int uid, int gid) override { return NativeFileHandle::chown(uid, gid); }
        int set_flags(int flags) override { return NativeFileHandle::set_flags(flags); }

        ssize_t read(uint8_t *buf, size_t size) override
        {
            ssize_t ret = ::read(fd, buf, size);
            if (ret < 0)
            {
                swap_error();
                return -1;
            }
            return ret;
        }

        ssize_t write(const uint8_t *buf, size_t size) override
        {
            ssize_t ret = ::write(fd, buf, size);
            if (ret < 0)
            {
                swap_error();
                return -1;
            }
            return ret;
        }

        int64_t seek(int64_t offset, int whence) override
        {
            off_t ret = lseek(fd, offset, whence);
            if (ret < 0)
            {
                swap_error();
                return -1;
            }
            return ret;
        }

        int64_t tell() override
        {
            off_t ret = lseek(fd, 0, SEEK_CUR);
            if (ret < 0)
            {
                swap_error();
                return -1;
            }
            return ret;
        }

        int truncate(int64_t size) override
        {
            if (ftruncate(fd, size) < 0)
            {
                swap_error();
                return -1;
            }
            return 0; // Success
        }

        int64_t size() override
        {
            sys_stat st;
            if (stat(&st) < 0)
            {
                return -1;
            }
            return st.size;
        }
    };

    class NativeSpecialFileHandle : public BaseSpecialFile, public NativeFileHandle
    {
    public:
        using NativeFileHandle::NativeFileHandle;

        BaseFile *clone() override
        {
            int new_fd = dup(fd);
            if (new_fd < 0)
            {
                swap_error();
                return nullptr;
            }
            return alloc<NativeSpecialFileHandle>(1, new_fd, filesystem);
        }

        BaseFilesystem *get_filesystem() override { return NativeFileHandle::get_filesystem(); }
        int get_id() const override { return NativeFileHandle::get_id(); }
        int stat(sys_stat *buf) override { return NativeFileHandle::stat(buf); }
        int get_mode() override { return NativeFileHandle::get_mode(); }
        int get_flags() override { return NativeFileHandle::get_flags(); }
        int get_uid() override { return NativeFileHandle::get_uid(); }
        int get_gid() override { return NativeFileHandle::get_gid(); }
        int chmod(int mode) override { return NativeFileHandle::chmod(mode); }
        int chown(int uid, int gid) override { return NativeFileHandle::chown(uid, gid); }
        int set_flags(int flags) override { return NativeFileHandle::set_flags(flags); }

        DeviceID get_device_id() override
        {
            sys_stat st = {};
            if (stat(&st) < 0)
            {
                return {0, 0}; // Return invalid device ID on error
            }
            return {static_cast<uint32_t>(st.rdev >> 20), static_cast<uint32_t>(st.rdev & 0xFFFFF)};
        }
    };

    class NativeSymlinkHandle : public BaseSymlink, public NativeFileHandle
    {
    public:
        using NativeFileHandle::NativeFileHandle;

        BaseFile *clone() override
        {
            int new_fd = dup(fd);
            if (new_fd < 0)
            {
                swap_error();
                return nullptr;
            }
            return alloc<NativeSymlinkHandle>(1, new_fd, filesystem);
        }

        BaseFilesystem *get_filesystem() override { return NativeFileHandle::get_filesystem(); }
        int get_id() const override { return NativeFileHandle::get_id(); }
        int stat(sys_stat *buf) override { return NativeFileHandle::stat(buf); }
        int get_mode() override { return NativeFileHandle::get_mode(); }
        int get_flags() override { return NativeFileHandle::get_flags(); }
        int get_uid() override { return NativeFileHandle::get_uid(); }
        int get_gid() override { return NativeFileHandle::get_gid(); }
        int chmod(int mode) override { return NativeFileHandle::chmod(mode); }
        int chown(int uid, int gid) override { return NativeFileHandle::chown(uid, gid); }
        int set_flags(int flags) override { return NativeFileHandle::set_flags(flags); }

        char *get_target() override
        {
            if (fd < 0)
            {
                error = EBADF;
                return nullptr;
            }

            char target[PATH_MAX];
            ssize_t len = readlinkat(fd, "", target, sizeof(target) - 1);
            if (len < 0)
            {
                swap_error();
                return nullptr;
            }

            target[len] = '\0'; // Null-terminate the string
            char *result = alloc<char>(len + 1);
            strcpy(result, target);
            return result;
        }

        int set_target(const char *target) override
        {
            // TODO: change the target of the symlink
            error = ENOSYS;
            return -1; // Not implemented
        }
    };

    class NativeDirectoryHandle : public BaseDirectory, public NativeFileHandle
    {
    public:
        using NativeFileHandle::NativeFileHandle;

        BaseFile *clone() override
        {
            int new_fd = dup(fd);
            if (new_fd < 0)
            {
                swap_error();
                return nullptr;
            }
            return alloc<NativeDirectoryHandle>(1, new_fd, filesystem);
        }

        BaseFilesystem *get_filesystem() override { return NativeFileHandle::get_filesystem(); }
        int get_id() const override { return NativeFileHandle::get_id(); }
        int stat(sys_stat *buf) override { return NativeFileHandle::stat(buf); }
        int get_mode() override { return NativeFileHandle::get_mode(); }
        int get_flags() override { return NativeFileHandle::get_flags(); }
        int get_uid() override { return NativeFileHandle::get_uid(); }
        int get_gid() override { return NativeFileHandle::get_gid(); }
        int chmod(int mode) override { return NativeFileHandle::chmod(mode); }
        int chown(int uid, int gid) override { return NativeFileHandle::chown(uid, gid); }
        int set_flags(int flags) override { return NativeFileHandle::set_flags(flags); }

        char * const *list(size_t count) override
        {
            if (fd < 0)
            {
                error = EBADF;
                return nullptr;
            }

            int dup_fd = dup(fd);
            DIR *dir = fdopendir(dup_fd);
            if (!dir)
            {
                swap_error();
                return nullptr;
            }

            rewinddir(dir); // Reset the directory stream

            std::queue<char *> entries;
            struct dirent *entry;

            // Go to position in directory
            for (int64_t i = 0; i < pos && (entry = readdir(dir)) != nullptr; ++i)
            {
                // Just read entries until we reach the desired position
            }

            while ((entry = readdir(dir)) != nullptr)
            {
                if (count == 0)
                    break;
                const char *name = entry->d_name;
                size_t len = strlen(name);
                char *name_copy = alloc<char>(len + 1);
                strcpy(name_copy, name);
                entries.push(name_copy);

                ++pos;
                --count;
            }

            closedir(dir);

            char **result = alloc<char *>(entries.size() + 1);
            result[entries.size()] = nullptr; // Null-terminate the array

            size_t i = 0;
            while (!entries.empty())
            {
                result[i++] = entries.front();
                entries.pop();
            }

            return result;
        }

        int64_t seek(int64_t offset, int whence) override
        {
            switch (whence)
            {
            case H_SEEK_SET:
                if (offset < 0)
                {
                    error = EINVAL;
                    return -1; // Invalid offset
                }
                pos = offset;
                break;
            case H_SEEK_CUR:
                if (pos + offset < 0)
                {
                    error = EINVAL;
                    return -1; // Invalid offset
                }
                pos += offset;
                break;
            case H_SEEK_END:
                error = ENOTSUP;
                return -1; // Not supported for directories
            default:
                error = EINVAL;
                return -1; // Invalid whence
            }

            return pos;
        }

        int64_t tell() override
        {
            return pos;
        }

        BaseFile *get(const char *name, int flags, int mode) override
        {
            if (fd < 0)
            {
                error = EBADF;
                return nullptr;
            }

            int new_fd = openat(fd, name, O_RDWR | O_NONBLOCK | O_NOFOLLOW |
                                          (flags & OPEN_APPEND ? O_APPEND : 0) |
                                          (flags & OPEN_TRUNC ? O_TRUNC : 0));
            if (new_fd < 0)
            {
                if (errno == ENOENT && (flags & OPEN_CREAT))
                {
                    return flags & OPEN_DIRECTORY ?
                        (BaseFile*)mkdir(name, O_RDWR | O_NONBLOCK, mode) :
                        (BaseFile*)mkfile(name, O_RDWR | O_NONBLOCK, mode);
                }
                else if (errno == ELOOP)
                {
                    new_fd = openat(fd, name, O_PATH | O_NOFOLLOW);
                    if (new_fd < 0)
                    {
                        swap_error();
                        return nullptr;
                    }
                }
                else if (errno == EISDIR)
                {
                    new_fd = openat(fd, name, O_RDONLY | O_DIRECTORY);
                    if (new_fd < 0)
                    {
                        swap_error();
                        return nullptr;
                    }
                }
                else
                {
                    swap_error();
                    return nullptr;
                }
            }
            struct stat st;
            if (fstat(new_fd, &st) < 0)
            {
                swap_error();
                close(new_fd);
                return nullptr;
            }

            BaseFile *file = nullptr;

            if ((flags & OPEN_DIRECTORY) && !S_ISDIR(st.st_mode))
            {
                error = ENOTDIR;
                return nullptr;
            }

            if (S_ISREG(st.st_mode))
            {
                file = alloc<NativeRegularFileHandle>(1, new_fd, filesystem);
            }
            else if (S_ISDIR(st.st_mode))
            {
                file = alloc<NativeDirectoryHandle>(1, new_fd, filesystem);
            }
            else if (S_ISLNK(st.st_mode))
            {
                file = alloc<NativeSymlinkHandle>(1, new_fd, filesystem);
            }
            else if (S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode))
            {
                file = alloc<NativeSpecialFileHandle>(1, new_fd, filesystem);
            }
            else
            {
                close(new_fd);
                error = ENOSYS; // Unsupported file type
                return nullptr;
            }

            return file;
        }

        BaseRegularFile *mkfile(const char *name, int flags, int mode) override
        {
            if (fd < 0)
            {
                error = EBADF;
                return nullptr;
            }

            int new_fd = openat(fd, name, O_RDWR | O_NONBLOCK | O_CREAT | O_EXCL, mode);

            if (new_fd < 0)
            {
                swap_error();
                return nullptr;
            }
            
            return alloc<NativeRegularFileHandle>(1, new_fd, filesystem);
        }

        BaseDirectory *mkdir(const char *name, int flags, int mode) override
        {
            if (fd < 0)
            {
                error = EBADF;
                return nullptr;
            }

            if (mkdirat(fd, name, mode) < 0)
            {
                swap_error();
                return nullptr;
            }

            int new_fd = openat(fd, name, O_RDONLY | O_DIRECTORY, mode);
            if (new_fd < 0)
            {
                swap_error();
                return nullptr;
            }

            return alloc<NativeDirectoryHandle>(1, new_fd, filesystem);
        }

        BaseSymlink *mksym(const char *name, const char *target) override
        {
            if (fd < 0)
            {
                error = EBADF;
                return nullptr;
            }

            int ret = symlinkat(target, fd, name);
            if (ret < 0)
            {
                swap_error();
                return nullptr;
            }

            int new_fd = openat(fd, name, O_PATH | O_NOFOLLOW);
            if (new_fd < 0)
            {
                swap_error();
                return nullptr;
            }

            return alloc<NativeSymlinkHandle>(1, new_fd, filesystem);
        }

        BaseSpecialFile *mksfile(const char *name, int flags, DeviceID device_id, int mode) override
        {
            if (fd < 0)
            {
                error = EBADF;
                return nullptr;
            }

            // Create a special file (character or block device)
            int new_fd = mknodat(fd, name, mode, device_id.major << 20 | (device_id.minor & 0xFFFFF));
            if (new_fd < 0)
            {
                swap_error();
                return nullptr;
            }

            return alloc<NativeSpecialFileHandle>(1, new_fd, filesystem);
        }

        int link(BaseFile *file, const char *name) override
        {
            if (fd < 0)
            {
                error = EBADF;
                return -1;
            }

            if (get_filesystem() != file->get_filesystem())
            {
                error = EXDEV; // Cross-device link not permitted
                return -1;
            }

            int file_fd = -1;
            switch (file->type())
            {
            case FileType::Regular:
                file_fd = ((NativeRegularFileHandle *)file)->get_fd();
                break;
            case FileType::Directory:
                file_fd = ((NativeDirectoryHandle *)file)->get_fd();
                break;
            case FileType::Symlink:
                file_fd = ((NativeSymlinkHandle *)file)->get_fd();
                break;
            case FileType::Special:
                file_fd = ((NativeSpecialFileHandle *)file)->get_fd();
                break;
            default:
                error = EBADF; // Invalid file type for linking
                return -1;
            }

            if (file_fd < 0)
            {
                error = EBADF; // Bad file descriptor
                return -1;
            }

            int ret = linkat(file_fd, "", fd, name, AT_EMPTY_PATH);

            if (ret < 0)
            {
                swap_error();
                return -1;
            }

            return 0; // Success
        }

        int remove(const char *name) override
        {
            if (fd < 0)
            {
                error = EBADF;
                return -1;
            }

            if (unlinkat(fd, name, 0) == 0)
                return 0;
            if (unlinkat(fd, name, AT_REMOVEDIR) == 0)
                return 0;
            swap_error();
            return -1;
        }
    private:
        off_t pos = 0;
    };

    class NativeFilesystem : public BaseFilesystem
    {
    public:
        const char *root_start;

        BaseDirectory *open_root(int flags) override
        {
            int fd = open(root_start, O_RDONLY | O_DIRECTORY);
            if (fd < 0)
            {
                swap_error();
                return nullptr;
            }

            return alloc<NativeDirectoryHandle>(1, fd, this);
        }
    };
} // namespace

namespace
{
#ifndef NTRACE
    FILE *trace_file = nullptr;

    void trace_atexit_handler()
    {
        if (trace_file)
        {
            fclose(trace_file);
            trace_file = nullptr;
        }
    }
#endif

    // Console device

    class ConsoleTTYBackend
    {
    public:
        ConsoleTTYBackend()
        {
            if (tcgetattr(STDIN_FILENO, &old_termios) == 0)
            {
                struct termios new_termios = old_termios;
                new_termios.c_lflag &= ~(ICANON | ECHO | ISIG); // Disable canonical mode, echo, and signals
                new_termios.c_iflag &= ~(IXON | ICRNL); // Disable flow control and CR to NL translation
                new_termios.c_oflag &= ~(OPOST); // Disable output processing
                new_termios.c_cflag |= (CS8 | CREAD); // 8-bit characters and enable receiver
                new_termios.c_cc[VMIN] = 1;
                new_termios.c_cc[VTIME] = 0; 
                tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
            }
            fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK); // Set non-blocking mode
            fcntl(STDOUT_FILENO, F_SETFL, O_NONBLOCK); // Set non-blocking mode
        }
        ~ConsoleTTYBackend()
        {
            tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);
        }

        ssize_t read(void *buf, size_t size)
        {
            ssize_t ret = ::read(STDIN_FILENO, buf, size);
            if (ret < 0)
            {
                swap_error();
                return -1;
            }
            return ret;
        }

        ssize_t write(const void *buf, size_t size)
        {
            ssize_t ret = ::write(STDOUT_FILENO, buf, size);
            if (ret < 0)
            {
                swap_error();
                return -1;
            }
            return ret;
        }
        int get_win_sz(sys_winsize *ws)
        {
            struct winsize w;
            if (ioctl(STDIN_FILENO, TIOCGWINSZ, &w) < 0)
            {
                swap_error();
                return -1;
            }

            ws->row = w.ws_row;
            ws->col = w.ws_col;
            ws->xpixel = w.ws_xpixel;
            ws->ypixel = w.ws_ypixel;
            return 0; // Success
        }
    private:
        struct ::termios old_termios;
    };

    using ConsoleCharDevice = BaseTTYDriver<ConsoleTTYBackend>;
}

int Hamster::_init_platform()
{
#ifndef NTRACE
    char trace_file_name[256];
    printf("Enter trace file name (or leave empty to disable tracing): ");
    fgets(trace_file_name, sizeof(trace_file_name), stdin);
    trace_file_name[strcspn(trace_file_name, "\n")] = 0;
    if (trace_file_name[0] != '\0')
    {
        trace_file = fopen(trace_file_name, "w");
        if (!trace_file)
        {
            perror("Failed to open trace file");
            return -1;
        }
        atexit(trace_atexit_handler);
    }
    else
    {
        trace_file = nullptr;
    }
#endif

    return 0;
}

#ifndef NTRACE
void Hamster::_trace(const char *fmt, ...)
{
    if (!trace_file)
        return;
    
    va_list args;
    va_start(args, fmt);
    vfprintf(trace_file, fmt, args);
    va_end(args);
}
#endif

int Hamster::_mount_rootfs()
{
    auto fs = Hamster::alloc<NativeFilesystem>();
    fs->root_start = HAMSTER_NATIVE_FS_ROOT;
    Hamster::vfs.mount("/", fs) == 0 ? (void)0 : Hamster::dealloc(fs);

    Hamster::vfs.mkdir("/dev", 0755);
    Hamster::vfs.mkdir("/tmp", 0755);
    BaseFilesystem *ramfs = Hamster::alloc<Hamster::RamFs>();
    Hamster::vfs.mount("/dev", ramfs) == 0 ? (void)0 : Hamster::dealloc(ramfs);
    ramfs = Hamster::alloc<Hamster::RamFs>();
    Hamster::vfs.mount("/tmp", ramfs) == 0 ? (void)0 : Hamster::dealloc(ramfs);
    auto console_device = Hamster::alloc<ConsoleCharDevice>();
    Hamster::device_manager.register_device({5, 1}, console_device);
    Hamster::vfs.mknod("/dev/console", {5, 1}, 0666);
    Hamster::vfs.symlink("/dev/tty", "/dev/console");

    return 0;
}

void *Hamster::_malloc(size_t size)
{
    void *mem = malloc(size);
    return mem;
}

int Hamster::_free(void *ptr)
{
    free(ptr);
    return 0;
}

size_t Hamster::_get_free_memory()
{
    // Since we are on a host, we can't get actual free memory
    // but we can do this to always trick the page manager into thinking
    // there's still memory available, which for the most part is mostly true.
    return HAMSTER_TARGET_FREE_RAM + 1;
}

int Hamster::_log(const char *msg)
{
    int out = printf("%s", msg);
    fflush(stdout);
    return out;
}

int Hamster::_log(char c)
{
    int out = printf("%c", c);
    fflush(stdout);
    return out;
}

uint64_t Hamster::_get_sys_time()
{
    struct timespec ts;

    // Fall back to CLOCK_REALTIME if CLOCK_MONOTONIC is not available
    if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0 &&
        clock_gettime(CLOCK_REALTIME, &ts) < 0)
    {
        swap_error();
        return 0; // Error, return 0
    }

    // Convert to milliseconds
    uint64_t time_ms = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    return time_ms;
}

#endif
