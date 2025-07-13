#include <platform/platform.hpp>

#include <process/process.hpp>
#include <process/thread.hpp>
#include <elf/elf_loader.hpp>
#include <filesystem/vfs.hpp>
#include <filesystem/ramfs.hpp>
#include <memory/allocator.hpp>
#include <process/scheduler.hpp>
#include <errno/errno.h>

#ifdef __STDC_HOSTED__
#include <unistd.h>
#include <fcntl.h>
#endif

void test_platform();
void test_memory();
void test_filesystem();

// Console device

class ConsoleCharDeviceHandle : public Hamster::BaseCharacterDeviceHandle
{
public:
    ssize_t write(const uint8_t *buf, size_t size) override
    {
        for (size_t i = 0; i < size; ++i)
        {
            Hamster::_log(buf[i]);
        }
        return size;
    }

    ssize_t read(uint8_t *buf, size_t size) override
    {
        // For simplicity, we won't implement reading from the console
        // However, it will work with `read` calls if we are on a hosted environment
#ifdef __STDC_HOSTED__
        ssize_t bytes_read = ::read(STDIN_FILENO, buf, size);
        if (bytes_read < 0)
        {
            Hamster::error = errno;
            errno = 0;
            return -1;
        }
        return bytes_read;
#else
        Hamster::error = ENOSYS; // Not implemented
        return -1;
#endif
    }

    int isatty() override
    {
        return 1; // This is a TTY device
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

int main()
{
    if (Hamster::_init_platform() != 0)
    {
        // Don't log here, as we don't know if log would work
        return -1;
    }

#ifndef NDEBUG
    Hamster::_log("Testing Platform...\n");
    test_platform();
    Hamster::_log("Done\n");

    Hamster::_log("Testing Memory...\n");
    test_memory();
    Hamster::_log("Done\n");

    Hamster::_log("Testing Filesystem...\n");
    test_filesystem();
    Hamster::_log("Done\n");

    Hamster::error = 0; // Reset error after tests
#endif // NDEBUG

    if (Hamster::_mount_rootfs() != 0)
    {
        Hamster::_log("Failed to mount root filesystem\n");
        return -1;
    }

    // create /dev and /dev/console

    Hamster::vfs.mkdir("/dev", 0755);
    Hamster::vfs.mkdir("/tmp", 0755);
    auto ramfs = Hamster::alloc<Hamster::RamFs>();
    Hamster::vfs.mount("/dev", ramfs) == 0 ? (void)0 : Hamster::dealloc(ramfs);
    ramfs = Hamster::alloc<Hamster::RamFs>();
    Hamster::vfs.mount("/tmp", ramfs) == 0 ? (void)0 : Hamster::dealloc(ramfs);
    auto console_device = Hamster::alloc<ConsoleCharDevice>();
    Hamster::vfs.mksfile("/dev/console", console_device, 0666) == 0 ? (void)0 : Hamster::dealloc(console_device);
    
    Hamster::scheduler.make_process_elf("/usr/bin/init");

    Hamster::_log("Starting userspace...\n");
    Hamster::_log("========== [ BEGIN USERSPACE OUTPUT ] ==========\n");

    // Run the program
    while (true)
    {
        Hamster::scheduler.tick();

        // Check if there are any processes left
        bool has_processes = false;
        for (const auto &process : Hamster::scheduler.get_processes())
        {
            if (process && !process->threads.empty())
            {
                has_processes = true;
                break;
            }
        }

        if (!has_processes)
        {
            Hamster::_log("\n=========== [ END USERSPACE OUTPUT ] ===========\n");
            Hamster::_log("No more processes left, exiting...\n");
            break; // Exit the loop if no processes are left
        }
    }
}
