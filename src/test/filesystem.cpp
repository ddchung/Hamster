// Test filesystem

#include <filesystem/vfs.hpp>
#include <filesystem/ramfs.hpp>
#include <filesystem/device_manager.hpp>
#include <memory/allocator.hpp>
#include <memory/stl_sequential.hpp>
#include <memory/memory_space.hpp>
#include <errno/errno.h>
#include <cassert>
#include <cstring>
#include <abi/values.hpp>

using namespace Hamster;

#ifndef NDEBUG

void test_filesystem()
{
    // Allocate VFS and filesystem using custom allocator
    VFS *vfs = alloc<VFS>(1);
    BaseFilesystem *fs = alloc<RamFs>(1);

    // Mount filesystem at root
    assert(vfs->mount("/", fs) == 0);

    // 1. File creation and open/write/read/close
    const char *path = "/file.txt";
    int fd = vfs->mkfile(path, OPEN_RDWR | OPEN_CREAT, 0644);
    assert(fd >= 0);

    const char *text = "Hello, VFS!";
    ssize_t written = vfs->write(fd, (const uint8_t*)text, strlen(text));
    assert(written == (ssize_t)strlen(text));

    // Seek back and read
    assert(vfs->seek(fd, 0, H_SEEK_SET) == 0);
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
    assert(vfs->stat(vfs->open(path, OPEN_RDONLY), &st) == 0);
    assert(is_regular_file(st.mode));
    vfs->close(fd);

    assert(vfs->lstat(path, &st) == 0);

    // 3. Mode, flags, ownership
    fd = vfs->open(path, OPEN_RDWR);
    assert(vfs->get_flags(fd) & OPEN_RDWR);
    assert(vfs->chmod(fd, 0600) == 0);
    assert((vfs->get_mode(fd) & 0777) == 0600);
    assert(vfs->chown(fd, 1000, 1000) == 0);
    assert(vfs->get_uid(fd) == 1000);
    assert(vfs->get_gid(fd) == 1000);

    vfs->close(fd);

    // 4. Directories, openat, mkfileat, mkdir, mkdirat, list
    const char *dpath = "/dir";
    int dfd = vfs->mkdir(dpath, OPEN_RDONLY, 0755);
    assert(dfd >= 0);

    // Create file inside via openat
    int fd2 = vfs->openat(dfd, "inner.txt", OPEN_RDWR | OPEN_CREAT, 0644);
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
    int sub = vfs->mkdirat(dfd, "subdir", OPEN_RDONLY, 0700);
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
    int dirfd2 = vfs->mkdir("/linkdir", OPEN_RDONLY, 0755);
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

        int ioctl(int flags, IoctlArg arg) override
        {
            error = ENOTTY;
            return -1;
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
    device_manager.register_device({1, 1}, driver);
    int special_fd = vfs->mknod(special_path, OPEN_RDWR | OPEN_CREAT, {1, 1}, 0777);
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

    int mmap_fd = vfs->open("/file.txt", OPEN_RDWR | OPEN_CREAT, 0644);
    assert(mmap_fd >= 0);
    

    MemorySpace mem_space;

    // Map 64 bytes starting at virtual address 10
    assert(mem_space.mmap(10, 64, 07, MAP_SHARED, mmap_fd, 0) == 0);

    // Write to mapped memory
    const char *mmap_text = "Mapped Memory! 1234567890abcdefghijklmnopqrstuvwxyz";
    assert(mem_space.memcpy(10, mmap_text, strlen(mmap_text)) == 0);

    fd = vfs->open("/file.txt", OPEN_RDWR);
    assert(fd >= 0);

    assert(vfs->seek(fd, 0, H_SEEK_SET) == 0);
    assert(vfs->read(fd, buf, strlen(mmap_text)) == (ssize_t)strlen(mmap_text));
    assert(strncmp(buf, mmap_text, strlen(mmap_text)) == 0);

    assert(vfs->seek(fd, 0, H_SEEK_SET) == 0);

    const char *new_text = "New Text! blah blah blah";
    const char *expected = "New Text! blah blah blah0abcdefghijklmnopqrstuvwxyz";

    assert(vfs->write(fd, new_text, strlen(new_text)) == (ssize_t)strlen(new_text));
    assert(mem_space.memcpy(buf, 10, strlen(expected)) == 0);

    assert(strncmp(buf, expected, strlen(expected)) == 0);

    assert(mem_space.munmap(15, 10) == 0); // partially unmap the memory

    new_text = "Partially Unmapping Memory!";
    expected = "Parti__________ping Memory!cdefghijklmnopqrstuvwxyz";

    assert(vfs->seek(fd, 0, H_SEEK_SET) == 0);
    assert(vfs->write(fd, new_text, strlen(new_text)) == (ssize_t)strlen(new_text));
    assert(mem_space.memcpy(buf, 10, strlen(expected)) == 0);
    assert(strncmp(buf, expected, 5) == 0);
    assert(strncmp(buf + 15, expected + 15, strlen(expected) - 15) == 0);

    assert(vfs->close(fd) == 0);
    assert(vfs->unmount("/") == 0);

    // Remount new ramfs for additional tests
    fs = alloc<RamFs>(1);
    vfs = alloc<VFS>(1);
    assert(vfs->mount("/", fs) == 0);

    // --- Additional Regular File Tests ---
    // Test zero-length write
    fd = vfs->open("/zerowrite.txt", OPEN_RDWR | OPEN_CREAT, 0644);
    assert(fd >= 0);
    assert(vfs->write(fd, "", 0) == 0);
    assert(vfs->size(fd) == 0);
    vfs->close(fd);

    // Test large file write/read
    fd = vfs->open("/largefile.bin", OPEN_RDWR | OPEN_CREAT, 0644);
    assert(fd >= 0);
    const size_t bigsize = 4096 * 4;
    char *bigbuf = alloc<char>(bigsize, 'A');
    assert(vfs->write(fd, bigbuf, bigsize) == (ssize_t)bigsize);
    assert(vfs->seek(fd, 0, H_SEEK_SET) == 0);
    char *readbuf = alloc<char>(bigsize);
    assert(vfs->read(fd, readbuf, bigsize) == (ssize_t)bigsize);
    for (size_t i = 0; i < bigsize; ++i) assert(readbuf[i] == 'A');
    dealloc(bigbuf); dealloc(readbuf);
    vfs->close(fd);

    // Test invalid seek
    fd = vfs->open("/file.txt", OPEN_RDWR);
    assert(vfs->seek(fd, -100, H_SEEK_SET) < 0);
    vfs->close(fd);

    // Test permission error
    fd = vfs->open("/perm.txt", OPEN_RDWR | OPEN_CREAT, 0000);
    assert(fd >= 0);
    assert(vfs->write(fd, "fail", 4) == 4);
    vfs->chmod(fd, 0000);
    // assert(vfs->read(fd, buf, 4) < 0);
    vfs->close(fd);

    // --- Directory Edge Cases ---
    dfd = vfs->mkdir("/deep", OPEN_RDONLY, 0755);
    assert(dfd >= 0);
    int sub1 = vfs->mkdirat(dfd, "sub1", OPEN_RDONLY, 0755);
    int sub2 = vfs->mkdirat(dfd, "sub2", OPEN_RDONLY, 0755);
    assert(sub1 >= 0 && sub2 >= 0);
    // Try to remove non-empty dir
    assert(vfs->remove("/deep") < 0);
    vfs->close(sub1); vfs->close(sub2); vfs->close(dfd);
    // Remove children then parent
    assert(vfs->remove("/deep/sub1") == 0);
    assert(vfs->remove("/deep/sub2") == 0);
    assert(vfs->remove("/deep") == 0);

    // Duplicate name
    dfd = vfs->mkdir("/dupdir", OPEN_RDONLY, 0755);
    assert(dfd >= 0);
    assert(vfs->mkdirat(dfd, "dup", OPEN_RDONLY, 0755) >= 0);
    assert(vfs->mkdirat(dfd, "dup", OPEN_RDONLY, 0755) < 0);
    vfs->close(dfd);

    // --- Symlink Edge Cases ---
    assert(vfs->symlink("/loop1", "/loop2") == 0);
    assert(vfs->symlink("/loop2", "/loop1") == 0);
    char *tgt = vfs->get_target("/loop1");
    assert(tgt && strcmp(tgt, "/loop2") == 0);
    dealloc<char>(tgt);
    // Broken symlink
    assert(vfs->symlink("/broken", "/noexist") == 0);
    assert(vfs->lstat("/broken", &st) == 0);
    assert(Hamster::is_symbolic_link(st.mode));
    // Symlink permission
    fd = vfs->open("/broken", OPEN_RDWR);
    assert(fd < 0);

    // --- Special File Edge Cases ---
    // Device ID retrieval and error

    // first, make the directory for the special file
    assert(vfs->mkdir("/dev", 0755) == 0);

    int sfd = vfs->mknod("/dev/test", OPEN_RDWR | OPEN_CREAT, {1, 1}, 0666);
    assert(sfd >= 0);
    // Try to open with wrong flags
    assert(vfs->open("/dev/test", OPEN_DIRECTORY) < 0);
    vfs->close(sfd);

    // --- Rename and Remove ---
    fd = vfs->open("/torm.txt", OPEN_RDWR | OPEN_CREAT, 0644);
    assert(fd >= 0);
    assert(vfs->rename("/torm.txt", "/torm2.txt") == 0);
    assert(vfs->remove("/torm2.txt") == 0);
    vfs->close(fd);

    // --- Memory Mapping Edge Cases ---
    fd = vfs->open("/mmapfile", OPEN_RDWR | OPEN_CREAT, 0644);
    assert(fd >= 0);
    MemorySpace ms;
    // Overlapping mapping
    assert(ms.mmap(0x1000, 0x100, 07, MAP_SHARED, fd, 0) == 0);
    assert(ms.mmap(0x1000, 0x100, 07, MAP_SHARED, fd, 0) < 0);
    // Partial unmap
    assert(ms.munmap(0x1000, 0x80) == 0);
    // Permission check
    assert(ms.check_permissions(0x1000, PROT_READ));
    vfs->close(fd);

    // --- More Regular File Edge Cases ---
    // Test file overwrite
    fd = vfs->open("/overwrite.txt", OPEN_RDWR | OPEN_CREAT, 0644);
    assert(fd >= 0);
    assert(vfs->write(fd, "abc", 3) == 3);
    assert(vfs->seek(fd, 0, H_SEEK_SET) == 0);
    assert(vfs->write(fd, "XYZ", 3) == 3);
    assert(vfs->seek(fd, 0, H_SEEK_SET) == 0);
    char obuf[4] = {0};
    assert(vfs->read(fd, obuf, 3) == 3);
    assert(strcmp(obuf, "XYZ") == 0);
    vfs->close(fd);

    // Test file truncation to zero
    fd = vfs->open("/trunc.txt", OPEN_RDWR | OPEN_CREAT, 0644);
    assert(fd >= 0);
    assert(vfs->write(fd, "123456", 6) == 6);
    assert(vfs->truncate(fd, 0) == 0);
    assert(vfs->size(fd) == 0);
    vfs->close(fd);

    // --- Directory/Listing Edge Cases ---
    // List empty directory
    dfd = vfs->mkdir("/emptydir", OPEN_RDONLY, 0755);
    assert(dfd >= 0);
    char * const *empty_entries = vfs->list(dfd);
    assert(empty_entries != nullptr);
    assert(strcmp(empty_entries[0], ".") == 0 || strcmp(empty_entries[0], "..") == 0);
    assert(strcmp(empty_entries[1], ".") == 0 || strcmp(empty_entries[1], "..") == 0);
    assert(empty_entries[2] == nullptr);
    dealloc(empty_entries[0]);
    dealloc(empty_entries[1]);
    dealloc(empty_entries);
    vfs->close(dfd);

    // --- Symlink Error Cases ---
    // Symlink to itself
    assert(vfs->symlink("/selflink", "/selflink") == 0);
    char *selftgt = vfs->get_target("/selflink");
    assert(selftgt && strcmp(selftgt, "/selflink") == 0);
    dealloc<char>(selftgt);
    // Symlink loop detection (should fail to open)
    fd = vfs->open("/selflink", OPEN_RDWR);
    assert(fd < 0);

    // --- Rename/Remove Error Cases ---
    // Rename non-existent file
    assert(vfs->rename("/noexist.txt", "/shouldnotexist.txt") < 0);
    // Remove non-existent file
    assert(vfs->remove("/noexist.txt") < 0);

    // --- Memory Mapping Error Cases ---
    // Map with invalid fd
    assert(ms.mmap(0x2000, 0x100, 07, MAP_SHARED, -1, 0) < 0);
    // Unmap region not mapped (should succeed or no-op)
    assert(ms.munmap(0x3000, 0x100) == 0);

    dealloc(vfs);
}

#endif // NDEBUG
