// Native version

#if !defined(ARDUINO) && 1

#include <platform/native/native_fs.hpp>
#include <platform/platform.hpp>
#include <filesystem/vfs.hpp>
#include <filesystem/ramfs.hpp>
#include <filesystem/device_manager.hpp>
#include <driver/base_tty.hpp>
#include <abi/values.hpp>
#include <errno/errno.h>
#include <cstdio>
#include <cstdlib>
#include <termios.h>
#include <sys/ioctl.h>
#include <cstdarg>
#include <time.h>

using namespace Hamster;

namespace
{
    void swap_error()
    {
        // Swap the error code with the global error code
        int err = Hamster::error;
        Hamster::error = errno;
        errno = err;
    }

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
