// Native version

#if !defined(ARDUINO) && 1

#include <platform/platform.hpp>
#include <filesystem/vfs.hpp>
#include <filesystem/ramfs.hpp>
#include <memory/allocator.hpp>
#include <cstdio>
#include <cstdlib>

using namespace Hamster;

int Hamster::_init_platform()
{
    // Mount a RAM filesystem at the root
    int ret = vfs.mount("/", alloc<RamFs>());

    // Make a /dev/console
    int fd = vfs.mkdir("/dev", O_RDONLY, 0777);
    if (fd < 0)
    {
        printf("Failed to create /dev directory: %d\n", fd);
        return -1;
    }
    vfs.close(fd);

    class ConsoleHandle : public BaseCharacterDeviceHandle
    {
    public:
        int flags;
        ssize_t write(const uint8_t *buf, size_t size) override
        {
            return fwrite(buf, 1, size, stdout);
        }

        ssize_t read(uint8_t *buf, size_t size) override
        {
            return fread(buf, 1, size, stdin);
        }

        int get_flags() override
        {
            return flags;
        }

        int set_flags(int new_flags) override
        {
            flags = new_flags;
            return 0;
        }
    };

    class ConsoleDriver : public BaseSpecialDriver
    {
    public:
        BaseSpecialDriverHandle *create_handle(int flags) override
        {
            ConsoleHandle *handle = alloc<ConsoleHandle>();
            handle->flags = flags;
            return handle;
        }
    };

    fd = vfs.mksfile("/dev/console", O_RDWR | O_CREAT, alloc<ConsoleDriver>(), 0777);
    if (fd < 0)
    {
        printf("Failed to create /dev/console: %d\n", fd);
        return -1;
    }
    vfs.close(fd);
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
