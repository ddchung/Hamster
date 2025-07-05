// Test filesystem

#include <filesystem/vfs.hpp>
#include <filesystem/ramfs.hpp>
#include <memory/allocator.hpp>
#include <memory/stl_sequential.hpp>
#include <memory/memory_space.hpp>
#include <errno/errno.h>
#include <cassert>
#include <cstring>

using namespace Hamster;

void test_filesystem()
{
    // Allocate VFS and filesystem using custom allocator
    VFS *vfs = alloc<VFS>(1);
    BaseFilesystem *fs = alloc<RamFs>(1);

    // Mount filesystem at root
    assert(vfs->mount("/", fs) == 0);

    // 1. File creation and open/write/read/close
    const char *path = "/file.txt";
    int fd = vfs->mkfile(path, O_RDWR | O_CREAT, 0644);
    assert(fd >= 0);

    const char *text = "Hello, VFS!";
    ssize_t written = vfs->write(fd, (const uint8_t*)text, strlen(text));
    assert(written == (ssize_t)strlen(text));

    // Seek back and read
    assert(vfs->seek(fd, 0, SEEK_SET) == 0);
    char buf[64] = {0};
    ssize_t readn = vfs->read(fd, (uint8_t*)buf, sizeof(buf));
    assert(readn == written);
    assert(strcmp(buf, text) == 0);

    // Test tell and size/truncate
    int64_t pos = vfs->tell(fd);
    assert(pos == written);
    int64_t fsize = vfs->size(fd);
    assert(fsize == written);

    // Truncate file
    assert(vfs->truncate(fd, 5) == 0);
    assert(vfs->size(fd) == 5);

    vfs->close(fd);

    // 2. Stat and lstat
    sys_stat st;
    assert(vfs->stat(vfs->open(path, O_RDONLY), &st) == 0);
    assert(is_regular_file(st.mode));
    vfs->close(fd);

    assert(vfs->lstat(path, &st) == 0);

    // 3. Mode, flags, ownership
    fd = vfs->open(path, O_RDWR);
    assert(vfs->get_flags(fd) & O_RDWR);
    assert(vfs->chmod(fd, 0600) == 0);
    assert((vfs->get_mode(fd) & 0777) == 0600);
    assert(vfs->chown(fd, 1000, 1000) == 0);
    assert(vfs->get_uid(fd) == 1000);
    assert(vfs->get_gid(fd) == 1000);

    vfs->close(fd);

    // 4. Directories, openat, mkfileat, mkdir, mkdirat, list
    const char *dpath = "/dir";
    int dfd = vfs->mkdir(dpath, O_RDONLY, 0755);
    assert(dfd >= 0);

    // Create file inside via openat
    int fd2 = vfs->openat(dfd, "inner.txt", O_RDWR | O_CREAT, 0644);
    assert(fd2 >= 0);
    vfs->close(fd2);

    // List directory
    char * const *entries = vfs->list(dfd);
    bool found = false;
    for (size_t i = 0; entries[i]; ++i) {
        if (strcmp(entries[i], "inner.txt") == 0) {
            found = true;
        }
        dealloc<char>(entries[i]);
    }
    assert(found);
    dealloc(entries);

    // Make subdirectory with mkdirat
    int sub = vfs->mkdirat(dfd, "subdir", O_RDONLY, 0700);
    assert(sub >= 0);
    vfs->close(sub);

    vfs->close(dfd);

    // 5. Symlinks

    const char *link = "/link";
    assert(vfs->symlink(link, path) == 0);
    char *target = vfs->get_target(link);
    assert(target != nullptr);
    assert(strcmp(target, path) == 0);
    dealloc<char>(target);

    // Change symlink target
    assert(vfs->set_target(link, "/other") == 0);
    target = vfs->get_target(link);
    assert(strcmp(target, "/other") == 0);
    dealloc<char>(target);

    // symlinkat
    int dirfd2 = vfs->mkdir("/linkdir", O_RDONLY, 0755);
    assert(dirfd2 >= 0);
    assert(vfs->symlinkat(dirfd2, path, "inside") == 0);
    vfs->close(dirfd2);

    // 6. Removal
    assert((vfs->remove("/file.txt") == 0));

    // Unmount and cleanup
    assert(vfs->unmount("/") == 0);
    dealloc<VFS>(vfs);

    // New VFS for testing special files

    vfs = alloc<VFS>(1);
    fs = alloc<RamFs>(1);

    assert(vfs->mount("/", fs) == 0);

    // 8. Special files
    const char *special_path = "/special";

    Deque<int> deque;

    class TestSpecialDriverHandle : public BaseCharacterDeviceHandle
    {
    public:
        TestSpecialDriverHandle(Deque<int> &deque, int flags) : deque(deque), flags(flags) {}

        ssize_t read(uint8_t *buf, size_t count) override
        {
            if (deque.empty())
            {
                error = EAGAIN;
                return -1;
            }
            while (count --> 0)
            {
                if (deque.empty())
                    return count;
                *buf++ = deque.front();
                deque.pop_front();
            }
            return count;
        }

        ssize_t write(const uint8_t *buf, size_t count) override
        {
            for (size_t i = 0; i < count; ++i)
                deque.push_back(buf[i]);
            return count;
        }

        int get_flags() override
        {
            return flags;
        }

        int set_flags(int flags) override
        {
            this->flags = flags;
            return 0;
        }
    private:
        Deque<int> &deque;
        int flags;
    };

    class TestSpecialDriver : public BaseSpecialDriver
    {
    public:
        TestSpecialDriver(Deque<int> &deque) : deque(deque) {}

        BaseSpecialDriverHandle *create_handle(int flags) override
        {
            return alloc<TestSpecialDriverHandle>(1, deque, flags);
        }

    private:
        Deque<int> &deque;
    };

    TestSpecialDriver *driver = alloc<TestSpecialDriver>(1, deque);
    int special_fd = vfs->mksfile(special_path, O_RDWR | O_CREAT, driver, 0777);
    assert(special_fd >= 0);

    // Write to special file
    const char data[] = "Hello, Special!";
    written = vfs->write(special_fd, (const uint8_t*)data, strlen(data));
    assert(written == (ssize_t)strlen(data));
    assert(deque.size() == strlen(data));

    for (size_t i = 0; i < strlen(data); ++i)
    {
        assert(deque.front() == data[i]);
        deque.pop_front();
    }
    assert(deque.empty());

    // cleanup
    vfs->close(special_fd);
    assert(vfs->unmount("/") == 0);

    dealloc(vfs);

    vfs = &Hamster::vfs; // Use global VFS instance
    fs = alloc<RamFs>(1);
    assert(vfs->mount("/", fs) == 0);

    // Test memory mapping

    int mmap_fd = vfs->open("/file.txt", O_RDWR | O_CREAT, 0644);
    assert(mmap_fd >= 0);
    

    MemorySpace mem_space;

    // Map 64 bytes starting at virtual address 10
    assert(mem_space.mmap(10, 64, 07, MAP_SHARED, mmap_fd, 0) == 0);

    // Write to mapped memory
    const char *mmap_text = "Mapped Memory! 1234567890abcdefghijklmnopqrstuvwxyz";
    assert(mem_space.memcpy(10, mmap_text, strlen(mmap_text)) == 0);

    fd = vfs->open("/file.txt", O_RDWR);
    assert(fd >= 0);

    assert(vfs->seek(fd, 0, SEEK_SET) == 0);
    assert(vfs->read(fd, buf, strlen(mmap_text)) == (ssize_t)strlen(mmap_text));
    assert(strncmp(buf, mmap_text, strlen(mmap_text)) == 0);

    assert(vfs->seek(fd, 0, SEEK_SET) == 0);

    const char *new_text = "New Text! blah blah blah";
    const char *expected = "New Text! blah blah blah0abcdefghijklmnopqrstuvwxyz";

    assert(vfs->write(fd, new_text, strlen(new_text)) == (ssize_t)strlen(new_text));
    assert(mem_space.memcpy(buf, 10, strlen(expected)) == 0);

    assert(strncmp(buf, expected, strlen(expected)) == 0);

    assert(mem_space.munmap(15, 10) == 0); // partially unmap the memory

    new_text = "Partially Unmapping Memory!";
    expected = "Parti__________ping Memory!cdefghijklmnopqrstuvwxyz";

    assert(vfs->seek(fd, 0, SEEK_SET) == 0);
    assert(vfs->write(fd, new_text, strlen(new_text)) == (ssize_t)strlen(new_text));
    assert(mem_space.memcpy(buf, 10, strlen(expected)) == 0);
    assert(strncmp(buf, expected, 5) == 0);
    assert(strncmp(buf + 15, expected + 15, strlen(expected) - 15) == 0);

    assert(vfs->close(fd) == 0);
    assert(vfs->unmount("/") == 0);
}
