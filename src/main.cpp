#include <platform/platform.hpp>

#include <elf/elf_loader.hpp>
#include <filesystem/vfs.hpp>
#include <filesystem/ramfs.hpp>
#include <memory/allocator.hpp>
#include <process/scheduler.hpp>
#include <errno/errno.h>
#include <cstring>

#ifdef __STDC_HOSTED__
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#endif

void test_platform();
void test_memory();
void test_filesystem();

namespace
{
    // Logging formatters
    void log_operation(const char *operation)
    {
        Hamster::_log("[ ... ]\t");
        Hamster::_log(operation);
    }

    void log_operation_status(const char *status = "OK")
    {
        size_t len = strlen(status);
        size_t lpad = (5 - len) / 2;
        size_t rpad = 5 - lpad - len;
        Hamster::_log("\r[");
        for (size_t i = 0; i < lpad; ++i)
            Hamster::_log(" ");
        Hamster::_log(status);
        for (size_t i = 0; i < rpad; ++i)
            Hamster::_log(" ");
        Hamster::_log("]");
        Hamster::_log("\r\n");
    }
}

int main()
{
    if (Hamster::_init_platform() != 0)
    {
        // Don't log here, as we don't know if log would work
        return -1;
    }

#ifndef NDEBUG
    log_operation("Testing platform...");
    test_platform();
    log_operation_status();

    log_operation("Testing Memory...");
    test_memory();
    log_operation_status();

    log_operation("Testing Filesystem...");
    test_filesystem();
    log_operation_status();

    Hamster::error = 0; // Reset error after tests
#endif // NDEBUG

    log_operation("Mounting root filesystem...");

    if (Hamster::_mount_rootfs() != 0)
    {
        log_operation_status("FAIL");
        return -1;
    }
    log_operation_status("OK");

    Hamster::scheduler.spawn("/usr/bin/init");

    // Run the program
    while (true)
    {
        Hamster::scheduler.tick();

        if (Hamster::scheduler.num_tasks() == 0)
        {
            break;
        }
    }

    Hamster::_log("\n\n========== [ END USERSPACE OUTPUT  ] ==========\n");
    Hamster::_log("All threads ended\n");
}
