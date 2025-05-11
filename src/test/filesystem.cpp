// Test filesystem

#include <filesystem/vfs.hpp>
#include <filesystem/ramfs.hpp>
#include <memory/allocator.hpp>
#include <cassert>
#include <cstring>

using namespace Hamster;

void test_filesystem()
{
    RamFs *fs = alloc<RamFs>(1);
    VFS vfs;

    assert(vfs.mount("/", fs) == 0);

    int fd = vfs.mkfile("/test.txt", O_RDWR | O_CREAT, 0777);
    assert(fd >= 0);
    int i = vfs.close(fd);
    assert(i == 0);

    fd = vfs.open("/test.txt", O_RDWR);
    assert(fd >= 0);

    i = vfs.write(fd, (const uint8_t *)"Hello, World!", 13);
    assert(i == 13);
    i = vfs.seek(fd, 0, SEEK_SET);
    assert(i == 0);
    uint8_t buf[32];
    i = vfs.read(fd, buf, 32);
    assert(i == 13);
    buf[i] = '\0';
    assert(strcmp((char *)buf, "Hello, World!") == 0);
    i = vfs.close(fd);
    assert(i == 0);
}
