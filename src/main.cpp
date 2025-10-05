#include <platform/platform.hpp>

#include <elf/elf_loader.hpp>
#include <filesystem/vfs.hpp>
#include <filesystem/ramfs.hpp>
#include <filesystem/device_manager.hpp>
#include <memory/allocator.hpp>
#include <process/scheduler.hpp>
#include <kscheduler/kscheduler.hpp>
#include <riscv/riscv_emulator.hpp>
#include <driver/base_char_device.hpp>
#include <errno/errno.h>
#include <cstring>
#include <cstdlib>


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

    class UserSchedulerTickTask : public Hamster::BaseKTask
    {
    public:
        UserSchedulerTickTask()
        {
            flags = Hamster::KSCHED_AUTO_INTERVAL;
            interval = 0; // Tick as fast as possible
            id = 1; // Fixed ID
            next_tick = 0;
        }

        void run() override
        {
            Hamster::scheduler.tick();
            if (Hamster::scheduler.num_tasks() == 0)
            {
                Hamster::_trace("No tasks left, exiting...\n");
                flags |= Hamster::KSCHED_REMOVE_NOW | Hamster::KSCHED_REMOVE_ALL;
            }
        }
    };

#ifndef NTRACE
    class UserSchedulerPerfMonitorTask : public Hamster::BaseKTask
    {
    public:
        UserSchedulerPerfMonitorTask()
        {
            flags = Hamster::KSCHED_AUTO_INTERVAL;
            interval = 1000; // Every second
            id = 2; // Fixed ID
            next_tick = 0;
        }
        ~UserSchedulerPerfMonitorTask() override = default;

        void run() override
        {
            uint64_t current_tick_count = Hamster::total_instructions_executed;
            Hamster::_trace("Instructions executed in the last %llums: %llu\n", (unsigned long long)interval, (unsigned long long)(current_tick_count - last_tick_count));
            last_tick_count = current_tick_count;
        }
    private:
        uint64_t last_tick_count = 0;
    };
#endif

    // Some devices
    struct NullDevice {};
    struct ZeroDevice {};
    struct FullDevice {};
    struct RandomDevice {};
} // namespace

template <>
ssize_t Hamster::CharacterDeviceImpl<NullDevice>::write(const void *, size_t size)
{
    // Discard
    return size;
}

template <>
ssize_t Hamster::CharacterDeviceImpl<NullDevice>::read(void *, size_t)
{
    // EOF
    return 0;
}

template <>
ssize_t Hamster::CharacterDeviceImpl<ZeroDevice>::write(const void *, size_t size)
{
    return size;
}

template <>
ssize_t Hamster::CharacterDeviceImpl<ZeroDevice>::read(void *buf, size_t size)
{
    memset(buf, 0, size);
    return size;
}

template <>
ssize_t Hamster::CharacterDeviceImpl<FullDevice>::write(const void *, size_t)
{
    // Error with ENOSPC
    Hamster::error = H_ENOSPC;
    return -1;
}

template <>
ssize_t Hamster::CharacterDeviceImpl<FullDevice>::read(void *buf, size_t size)
{
    memset(buf, 0, size);
    return size;
}

template <>
int64_t Hamster::CharacterDeviceImpl<FullDevice>::seek(int64_t offset, int whence)
{
    return 0;
}

template <>
ssize_t Hamster::CharacterDeviceImpl<RandomDevice>::write(const void *, size_t size)
{
    // discard
    return size;
}

template <>
ssize_t Hamster::CharacterDeviceImpl<RandomDevice>::read(void *buf, size_t size)
{
    for (size_t i = 0; i < size; ++i)
    {
        ((uint8_t *)buf)[i] = rand() % 256;
    }
    return size;
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

    // Create some devices in /dev/
    Hamster::device_manager.register_device({1, 3}, Hamster::alloc<Hamster::CharacterDevice<NullDevice>>());
    Hamster::device_manager.register_device({1, 5}, Hamster::alloc<Hamster::CharacterDevice<ZeroDevice>>());
    Hamster::device_manager.register_device({1, 7}, Hamster::alloc<Hamster::CharacterDevice<FullDevice>>());
    Hamster::device_manager.register_device({1, 8}, Hamster::alloc<Hamster::CharacterDevice<RandomDevice>>());
    Hamster::device_manager.register_device({1, 9}, Hamster::alloc<Hamster::CharacterDevice<RandomDevice>>());

    Hamster::vfs.mknod("/dev/null", {1, 3}, 0666);
    Hamster::vfs.mknod("/dev/zero", {1, 5}, 0666);
    Hamster::vfs.mknod("/dev/full", {1, 7}, 0666);
    Hamster::vfs.mknod("/dev/random", {1, 8}, 0666);
    Hamster::vfs.mknod("/dev/urandom", {1, 9}, 0666);
    Hamster::vfs.mkdir("/dev/shm", 0777);
    Hamster::vfs.mount("/dev/shm", Hamster::alloc<Hamster::RamFs>());

    Hamster::scheduler.spawn("/usr/bin/init");

    // Add the user scheduler tick task
    Hamster::kscheduler.add_task(Hamster::alloc<UserSchedulerTickTask>());
#ifndef NTRACE
    Hamster::kscheduler.add_task(Hamster::alloc<UserSchedulerPerfMonitorTask>());
#endif

    // Run the program
    while (true)
    {
        Hamster::kscheduler.tick();
        if (!Hamster::kscheduler.has_tasks())
            break;
    }
}
