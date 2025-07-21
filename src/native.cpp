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
#include <cstdarg>
#include <string>

using namespace Hamster;

#define HAMSTER_NATIVE_FS_ROOT "/home/tin/hamster_rootfs"

namespace
{
    void recursive_copy_to_vfs(const char *native_dir, int vfs_dir)
    {
        if (!native_dir || vfs_dir < 0)
            return;

        DIR *dir = opendir(native_dir);
        if (!dir)
        {
            // Handle error opening directory
            return;
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL)
        {
            // Skip current and parent directory entries
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            // Build full path for the current entry
            size_t path_len = strlen(native_dir) + strlen(entry->d_name) + 2;
            char *full_path = alloc<char>(path_len);
            if (!full_path)
            {
                closedir(dir);
                return;
            }
            snprintf(full_path, path_len, "%s/%s", native_dir, entry->d_name);

            struct stat st;
            if (lstat(full_path, &st) == 0)
            {
                if (S_ISREG(st.st_mode))
                {
                    // regular file
                    int file_fd = vfs.openat(vfs_dir, entry->d_name, OPEN_CREAT | OPEN_RDWR | OPEN_EXCL, 0755);
                    if (file_fd < 0)
                    {
                        dealloc(full_path);
                        continue;
                    }

                    // Copy contents

                    // This is *not* reentrant or multi-thread safe, however,
                    // Hamster is single-threaded and it is impossible to re-enter
                    // this function in said single-threaded environment.

                    int native_fd = ::open(full_path, O_RDONLY);
                    if (native_fd < 0)
                    {
                        vfs.close(file_fd);
                        dealloc(full_path);
                        continue;
                    }

                    if (vfs.seek(file_fd, 0, H_SEEK_SET) < 0)
                    {
                        ::close(native_fd);
                        vfs.close(file_fd);
                        dealloc(full_path);
                        continue;
                    }

                    static char buffer[4096];
                    ssize_t bytes_read;
                    while ((bytes_read = ::read(native_fd, buffer, sizeof(buffer))) > 0)
                    {
                        ssize_t bytes_written = vfs.write(file_fd, buffer, bytes_read);
                        if (bytes_written < 0)
                        {
                            break; // Error writing to VFS
                        }
                        while (bytes_written < bytes_read)
                        {
                            ssize_t additional_bytes = vfs.write(file_fd, buffer + bytes_written, bytes_read - bytes_written);
                            if (additional_bytes < 0)
                            {
                                break; // Error writing to VFS
                            }
                            bytes_written += additional_bytes;
                        }
                    }
                    ::close(native_fd);
                    vfs.close(file_fd);
                }
                else if (S_ISDIR(st.st_mode))
                {
                    // directory

                    // open read-write to be able to iterate and create files in it
                    // Note that this is Hamster-specific VFS behavior
                    int new_vfs_dir = vfs.mkdirat(vfs_dir, entry->d_name, OPEN_RDWR, 0755);
                    if (new_vfs_dir < 0)
                    {
                        dealloc(full_path);
                        continue;
                    }

                    // Recursively copy contents of the directory
                    recursive_copy_to_vfs(full_path, new_vfs_dir);

                    vfs.close(new_vfs_dir);
                }
                else if (S_ISLNK(st.st_mode))
                {
                    // symlink
                    char link_target[PATH_MAX];
                    ssize_t len = readlink(full_path, link_target, sizeof(link_target) - 1);
                    if (len >= 0)
                    {
                        link_target[len] = '\0'; // Null-terminate the string
                        vfs.symlinkat(vfs_dir, entry->d_name, link_target);
                    }
                }

                // Other file types (e.g., sockets, FIFOs) are ignored for now
                // Maybe later in the future, we could support them by making a new driver
                // for each one that writes to the real file
            }

            dealloc(full_path);
        }

        closedir(dir);
    }
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
    RamFs *ramfs = alloc<RamFs>();

    if (vfs.mount("/", ramfs) < 0)
    {
        dealloc(ramfs);

        if (errno != EBUSY)
        {
            // We will continue if there is already something on "/"
            return -1;
        }
    }

    int fd = vfs.open("/", OPEN_RDWR | OPEN_DIRECTORY);

    if (fd < 0)
    {
        return -1;
    }

    recursive_copy_to_vfs(HAMSTER_NATIVE_FS_ROOT, fd);

    vfs.close(fd);

    Hamster::vfs.mkdir("/dev", 0755);
    Hamster::vfs.mkdir("/tmp", 0755);
    ramfs = Hamster::alloc<Hamster::RamFs>();
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
