// Teensy 4 implementation

#if (defined(ARDUINO_TEENSY41) || defined(ARDUINO_TEENSY40)) && 1

#include <Arduino.h>
#include <SD.h>
#include <platform/platform.hpp>
#include <filesystem/vfs.hpp>
#include <filesystem/ramfs.hpp>
#include <memory/allocator.hpp>
#include <filesystem/device_manager.hpp>
#include <driver/base_tty.hpp>
#include <errno/errno.h>
#include <o1heap.h>

using namespace Hamster;

namespace
{
    void recursive_copy_to_vfs(const char *sd_path, int vfs_dir_fd)
    {
        if (!sd_path || vfs_dir_fd < 0)
            return;

        SdFile dir;
        if (!dir.open(sd_path, O_READ))
        {
            // Failed to open SD directory
            return;
        }

        SdFile entry;
        while (entry.openNext(&dir, O_RDONLY))
        {
            static char entry_name[13];
            entry.getName(entry_name, sizeof(entry_name));

            // Skip "." and ".."
            if (strcmp(entry_name, ".") == 0 || strcmp(entry_name, "..") == 0)
            {
                entry.close();
                continue;
            }

            if (entry.isDir())
            {
                // Directory: create in VFS and recurse

                int new_vfs_dir = vfs.mkdirat(vfs_dir_fd, entry_name, OPEN_RDWR, 0755);
                if (new_vfs_dir >= 0)
                {
                    // Construct full path for recursion
                    size_t path_len = strlen(sd_path) + strlen(entry_name) + 2;
                    char *new_sd_path = alloc<char>(path_len);
                    if (new_sd_path)
                    {
                        snprintf(new_sd_path, path_len, "%s/%s", sd_path, entry_name);
                        recursive_copy_to_vfs(new_sd_path, new_vfs_dir);
                        dealloc(new_sd_path);
                    }
                    vfs.close(new_vfs_dir);
                }
            }
            else
            {
                // Symlinks are supported by naming a regular file with the prefix `hsln_`

                if (strncmp(entry_name, "hsln_", 5) == 0)
                {
                    // Read the file data

                    static char buf[64];
                    buf[0] = '\0';
                    entry.seekSet(0);
                    int len = entry.read(buf, sizeof(buf) - 1);
                    if (len > 0)
                    {
                        buf[len] = '\0';
                        _trace("Making symlink '%s' -> '%s'\n", entry_name + 5, buf);
                        vfs.symlinkat(vfs_dir_fd, entry_name + 5, (const char *)buf);
                    }
                }
                else
                {
                    int file_fd = vfs.openat(vfs_dir_fd, entry_name, OPEN_CREAT | OPEN_RDWR | OPEN_EXCL, 0755);
                    if (file_fd >= 0)
                    {
                        // Copy contents from SdFat file to VFS file

                        if (vfs.seek(file_fd, 0, H_SEEK_SET) < 0)
                        {
                            vfs.close(file_fd);
                            entry.close();
                            continue;
                        }

                        static char buffer[4096];
                        int32_t bytes_read;

                        // Read from SdFat file
                        entry.seekSet(0);
                        while ((bytes_read = entry.read(buffer, sizeof(buffer))) > 0)
                        {
                            int32_t bytes_written = vfs.write(file_fd, (uint8_t *)buffer, bytes_read);
                            if (bytes_written < 0)
                                break;
                            // Write remainder if partial write
                            int32_t total_written = bytes_written;
                            while (total_written < bytes_read)
                            {
                                int32_t w = vfs.write(file_fd, (uint8_t *)buffer + total_written, bytes_read - total_written);
                                if (w < 0)
                                    break;
                                total_written += w;
                            }
                            if (total_written < bytes_read)
                                break;
                        }

                        vfs.close(file_fd);
                    }
                }
            }

            entry.close();
        }

        dir.close();
    }

    class TeensyTTYBackend
    {
    public:
        ssize_t read(void *buf, size_t size)
        {
            // Read from Serial
            size_t bytes_read = 0;
            while (bytes_read < size)
            {
                if (!Serial.available())
                {
                    if (bytes_read == 0)
                    {
                        error = EAGAIN;
                        return -1;
                    }
                    break;
                }
                ((uint8_t *)buf)[bytes_read++] = Serial.read();
            }
            return bytes_read;
        }

        ssize_t write(const void *buf, size_t size)
        {
            // Write to serial
            size_t bytes_written = 0;
            while (bytes_written < size)
            {
                if (!Serial.availableForWrite())
                {
                    if (bytes_written == 0)
                    {
                        error = EAGAIN;
                        return -1;
                    }
                    break;
                }
                Serial.write(((const uint8_t *)buf)[bytes_written++]);
            }
            return bytes_written;
        }

        int get_win_sz(sys_winsize *ws)
        {
            // Teensy does not support terminal size, return default
            ws->row = 24;   // Default rows
            ws->col = 80;   // Default columns
            ws->xpixel = 0; // Not applicable
            ws->ypixel = 0; // Not applicable
            return 0;       // Success
        }
    };

    O1HeapInstance *ext_heap;
    O1HeapInstance *ram2_heap;
    EXTMEM uint8_t extmem_buffer[16 * 1024 * 1024];
    DMAMEM uint8_t ram2_buffer[450 * 1024];
} // namespace

void Hamster::_init_allocator()
{
    ext_heap = o1heapInit(extmem_buffer, sizeof(extmem_buffer));
    ram2_heap = o1heapInit(ram2_buffer, sizeof(ram2_buffer));
}

int Hamster::_init_platform()
{
    Serial.begin(115200);
    while (!Serial)
        ;
    auto trace_millis_start = millis();
    while (!SerialUSB1 && millis() - trace_millis_start < 1000)
        ;

    // init sd card, if available
    if (SD.begin(254))
        Serial.println("SD card found");
    else
    {
        // Set color to red
        Serial.print("\e[31m");
        Serial.println("! NO SD CARD FOUND !");
        SD.sdfs.initErrorPrint(&Serial);
        Serial.println("aborting...");
        Serial.print("\e[0m");
        abort();
    }
    return 0;
}

int Hamster::_mount_rootfs()
{
    vfs.mount("/", alloc<RamFs>());

    int rootfd = vfs.open("/", OPEN_RDWR);
    if (rootfd < 0)
        return -1;
    recursive_copy_to_vfs("/rootfs", rootfd);
    vfs.close(rootfd);

    Hamster::vfs.mkdir("/dev", 0755);
    Hamster::vfs.mkdir("/tmp", 0755);
    BaseFilesystem *ramfs = Hamster::alloc<Hamster::RamFs>();
    Hamster::vfs.mount("/dev", ramfs) == 0 ? (void)0 : Hamster::dealloc(ramfs);
    ramfs = Hamster::alloc<Hamster::RamFs>();
    Hamster::vfs.mount("/tmp", ramfs) == 0 ? (void)0 : Hamster::dealloc(ramfs);

    auto console_device = Hamster::alloc<BaseTTYDriver<TeensyTTYBackend>>();
    Hamster::device_manager.register_device({5, 1}, console_device);
    Hamster::vfs.mknod("/dev/console", {5, 1}, 0666);
    Hamster::vfs.symlink("/dev/tty", "/dev/console");

    return 0;
}

void *Hamster::_malloc(size_t size)
{
    void *ptr = nullptr;
    if (!ptr && size < 1024)
        ptr = o1heapAllocate(ram2_heap, size);
    if (!ptr)
        ptr = o1heapAllocate(ext_heap, size);
    return ptr;
}

int Hamster::_free(void *ptr)
{
    if (ptr >= extmem_buffer &&
        ptr < extmem_buffer + sizeof(extmem_buffer))
        o1heapFree(ext_heap, ptr);

    else if (ptr >= ram2_buffer &&
             ptr < ram2_buffer + sizeof(ram2_buffer))
        o1heapFree(ram2_heap, ptr);
    else
        __builtin_unreachable();
    return 0;
}

int Hamster::_log(const char *msg)
{
    Serial.write(msg);
    return 0;
}

int Hamster::_log(char c)
{
    Serial.write(c);
    return 0;
}

uint64_t Hamster::_get_sys_time()
{
    return millis();
}

size_t Hamster::_get_free_memory()
{
    size_t used = 0;
    auto diagnostics = o1heapGetDiagnostics(ram2_heap);
    used += diagnostics.allocated;
    diagnostics = o1heapGetDiagnostics(ext_heap);
    used += diagnostics.allocated;
    return used;
}

void Hamster::_trace(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    SerialUSB1.vprintf(fmt, args);
    va_end(args);
}

#endif // TEENSY41
