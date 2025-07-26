#include <platform/platform.hpp>

#include <elf/elf_loader.hpp>
#include <filesystem/vfs.hpp>
#include <filesystem/ramfs.hpp>
#include <memory/allocator.hpp>
#include <process/scheduler.hpp>
#include <errno/errno.h>

#ifdef __STDC_HOSTED__
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#endif

void test_platform();
void test_memory();
void test_filesystem();


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

    Hamster::scheduler.spawn("/usr/bin/init");

    Hamster::_log("Starting userspace...\n");
    Hamster::_log("========== [ BEGIN USERSPACE OUTPUT ] ==========\n");

    // Run the program
    while (true)
    {
        Hamster::scheduler.tick();
    }
}
