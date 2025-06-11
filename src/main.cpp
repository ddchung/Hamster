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
    Hamster::_init_platform();

    Hamster::_log("Testing Platform...\n");
    test_platform();
    Hamster::_log("Done\n");

    Hamster::_log("Testing Memory...\n");
    test_memory();
    Hamster::_log("Done\n");

    Hamster::_log("Testing Filesystem...\n");
    test_filesystem();
    Hamster::_log("Done\n");

    // create /dev and /dev/console

    Hamster::vfs.mkdir("/dev", 0755);
    auto ramfs = Hamster::alloc<Hamster::RamFs>();
    Hamster::vfs.mount("/dev", ramfs) == 0 ? (void)0 : Hamster::dealloc(ramfs);
    auto console_device = Hamster::alloc<ConsoleCharDevice>();
    Hamster::vfs.mksfile("/dev/console", console_device, 0666) == 0 ? (void)0 : Hamster::dealloc(console_device);
    
    const char *progname[] {"program", nullptr};
    Hamster::scheduler.make_process_elf("/a.out", progname);

    // Run the program
    while (true)
    {
        if (Hamster::scheduler.tick() == 0)
        {
            // All processes have finished
            break;
        }
    }
}
