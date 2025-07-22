// Native version

#if !defined(ARDUINO) && 1

#include <platform/platform.hpp>
#include <filesystem/vfs.hpp>
#include <filesystem/ramfs.hpp>
#include <filesystem/device_manager.hpp>
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
                errno = EBADF;
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
                errno = EBADF;
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
            buf->mode = st.st_mode;
            buf->nlink = st.st_nlink;
            buf->uid = st.st_uid;
            buf->gid = st.st_gid;
            buf->size = st.st_size;
            buf->atime = st.st_atime;
            buf->mtime = st.st_mtime;
            buf->ctime = st.st_ctime;

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
                errno = EBADF;
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
                errno = EBADF;
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
                errno = EBADF;
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
                errno = EBADF;
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
            sys_stat st;
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
                errno = EBADF;
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
                errno = EBADF;
                return nullptr;
            }

            int dup_fd = dup(fd);

            DIR *dir = fdopendir(dup_fd);
            if (!dir)
            {
                swap_error();
                return nullptr;
            }

            std::queue<char *> entries;
            struct dirent *entry;
            while ((entry = readdir(dir)) != nullptr)
            {
                if (count == 0)
                    break;
                const char *name = entry->d_name;
                size_t len = strlen(name);
                char *name_copy = alloc<char>(len + 1);
                strcpy(name_copy, name);
                entries.push(name_copy);

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
            if (fd < 0)
            {
                errno = EBADF;
                return -1;
            }

            off_t ret = lseek(fd, offset, whence);
            if (ret < 0)
            {
                swap_error();
                return -1;
            }
            return ret;
        }

        BaseFile *get(const char *name, int flags, int mode) override
        {
            if (fd < 0)
            {
                errno = EBADF;
                return nullptr;
            }

            int new_fd = openat(fd, name, O_RDWR | O_NOFOLLOW);
            if (new_fd < 0)
            {
                if (errno == ENOENT && (flags & OPEN_CREAT))
                {
                    return flags & OPEN_DIRECTORY ?
                        (BaseFile*)mkdir(name, O_RDWR, mode) :
                        (BaseFile*)mkfile(name, O_RDWR, mode);
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
                errno = ENOSYS; // Unsupported file type
                return nullptr;
            }

            return file;
        }

        BaseRegularFile *mkfile(const char *name, int flags, int mode) override
        {
            if (fd < 0)
            {
                errno = EBADF;
                return nullptr;
            }

            int new_fd = openat(fd, name, O_RDWR | O_CREAT | O_EXCL, mode);

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
                errno = EBADF;
                return nullptr;
            }

            int new_fd = mkdirat(fd, name, mode);
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
                errno = EBADF;
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
                errno = EBADF;
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
                errno = EBADF;
                return -1;
            }

            if (get_filesystem() != file->get_filesystem())
            {
                errno = EXDEV; // Cross-device link not permitted
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
                errno = EBADF; // Invalid file type for linking
                return -1;
            }

            if (file_fd < 0)
            {
                errno = EBADF; // Bad file descriptor
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
                errno = EBADF;
                return -1;
            }

            if (unlinkat(fd, name, 0) == 0)
                return 0;
            if (unlinkat(fd, name, AT_REMOVEDIR) == 0)
                return 0;
            swap_error();
            return -1;
        }
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
    void tty_atexit(void);
    int tty_reset(void);
    void tty_raw(void);

    struct termios orig_termios; /* TERMinal I/O Structure */
    int ttyfd = STDIN_FILENO;    /* STDIN_FILENO is 0 by default */

    /* exit handler for tty reset */
    void tty_atexit(void) /* NOTE: If the program terminates due to a signal   */
    {                     /* this code will not run.  This is for exit()'s     */
        tty_reset();      /* only.  For resetting the terminal after a signal, */
    } /* a signal handler which calls tty_reset is needed. */

    /* reset tty - useful also for restoring the terminal when this process
       wishes to temporarily relinquish the tty
    */
    int tty_reset(void)
    {
        /* flush and reset */
        if (tcsetattr(ttyfd, TCSAFLUSH, &orig_termios) < 0)
            return -1;
        return 0;
    }

    /* put terminal in raw mode - see termio(7I) for modes */
    void tty_raw(void)
    {
        struct termios raw;

        raw = orig_termios; /* copy original and then modify below */

        // /* input modes - clear indicated ones giving: no break, no CR to NL,
        //    no parity check, no strip char, no start/stop output (sic) control */
        // raw.c_iflag |= ICRNL;

        // /* control modes - set 8 bit chars */
        // raw.c_cflag |= (CS8);

        // /* local modes - clear giving: echoing off, canonical off (no erase with
        //    backspace, ^U,...),  no extended functions, no signal chars (^Z,^C) */
        // raw.c_lflag &= ~(ISIG);

        // /* control chars - set return condition: min number of bytes and timer */
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;

        /* put terminal in raw mode after flushing */
        if (tcsetattr(ttyfd, TCSAFLUSH, &raw) < 0)
        printf("Warning: Can't set TTY to raw mode, skipping.\n");
    }

    FILE *trace_file = nullptr;
    
    std::string trace_write_buf;

    void trace_atexit_handler()
    {
        if (trace_file)
        {
            fclose(trace_file);
            trace_file = nullptr;
        }
    }

    // Console device

    class ConsoleCharDeviceHandle : public Hamster::BaseCharacterDeviceHandle
    {
    public:
        ssize_t write(const uint8_t *buf, size_t size) override
        {
            ssize_t bytes_written = ::write(STDOUT_FILENO, buf, size);
            if (bytes_written < 0)
            {
                Hamster::error = errno;
                errno = 0;
                return -1;
            }

            // trace if tracing is enabled
            if (trace_file)
            {
                trace_write_buf.append(reinterpret_cast<const char *>(buf), bytes_written);
            }

            return bytes_written;
        }

        ssize_t read(uint8_t *buf, size_t size) override
        {
            ssize_t bytes_read = ::read(STDIN_FILENO, buf, size);
            if (bytes_read < 0)
            {
                Hamster::error = errno;
                errno = 0;
                return -1;
            }
            return bytes_read;
        }

        int ioctl(int req, Hamster::IoctlArg args) override
        {
            if (args.p)
                return ::ioctl(STDIN_FILENO, req, args.p);
            else
                return ::ioctl(STDIN_FILENO, req, args.i);
        }

        int get_flags() override
        {
            return flags;
        }

        int set_flags(int new_flags) override
        {
            flags = new_flags;
            return 0; // Success
        }

        int flags = 0;
    };

    class ConsoleCharDevice : public Hamster::BaseSpecialDriver
    {
    public:
        Hamster::BaseSpecialDriverHandle *create_handle(int flags) override
        {
            auto *handle = Hamster::alloc<ConsoleCharDeviceHandle>();
            handle->set_flags(flags);
            return handle;
        }
    };
}

int Hamster::_init_platform()
{
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

    if (isatty(ttyfd))
    {
        /* store current tty settings in orig_termios */
        if (tcgetattr(ttyfd, &orig_termios) < 0)
        {
            printf("Warning: Can't get tty settings, not setting to raw mode\n");
            return 0;
        }

        /* register the tty reset with the exit handler */
        if (atexit(tty_atexit) != 0)
        {
            printf("Error: Cannot register tty reset with atexit, exiting now.\n");
            tty_atexit();
            exit(1);
        }

        tty_raw();  /* put tty in raw mode */
    }
    return 0;
}


void Hamster::_trace(const char *fmt, ...)
{
    if (!trace_file)
        return;
    
    va_list args;
    va_start(args, fmt);
    vfprintf(trace_file, fmt, args);
    va_end(args);
}

void Hamster::_flush_trace()
{
    while (true)
    {
        size_t pos = trace_write_buf.find('\n');
        if (pos == std::string::npos)
            break; // No more complete lines

        std::string line = trace_write_buf.substr(0, pos + 1);
        _trace("[OUTPUT] %s", line.c_str());
        trace_write_buf.erase(0, pos + 1);
    }
}

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

#endif
