// Test filesystem

#include <filesystem/vfs.hpp>
#include <filesystem/ramfs.hpp>
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
    // Not supported on all systems, so don't assert
    if (vfs->set_target(link, "/other") == 0)
    {
        target = vfs->get_target(link);
        assert(strcmp(target, "/other") == 0);
        dealloc<char>(target);
    }

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

        TestSpecialDriverHandle *clone() override
        {
            return alloc<TestSpecialDriverHandle>(1, deque, flags);
        }

        ssize_t read(uint8_t *buf, size_t count) override
        {
            if (deque.empty())
            {
                error = H_EAGAIN;
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
            error = H_ENOTTY;
            return -1;
        }

        int64_t seek(int64_t offset, int whence) override
        {
            error = H_ESPIPE;
            return -1;
        }

        int64_t tell() override
        {
            error = H_ESPIPE;
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

    int special_fd = vfs->mknod(special_path, OPEN_RDWR | OPEN_CREAT, alloc<TestSpecialDriver>(1, deque), 0777);
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

    int sfd = vfs->mknod("/dev/test", OPEN_RDWR | OPEN_CREAT, alloc<TestSpecialDriver>(1, deque), 0666);
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

    dealloc(vfs);

    vfs = &Hamster::vfs;
    assert(vfs->mount("/", alloc<RamFs>(1)) == 0);

    // --- Memory Mapping ---
    {
        Hamster::MemorySpace ms;
        constexpr uint32_t page_size = HAMSTER_PAGE_SIZE;
        constexpr uint32_t region_size = page_size * 2;
        uint8_t perms = Hamster::PERM_READ | Hamster::PERM_WRITE;

        // Create and write to a file
        const char *mapfile = "/mapped_file.bin";
        int fd = vfs->open(mapfile, OPEN_RDWR | OPEN_CREAT, 0644);
        assert(fd >= 0);
        char filedata[page_size * 2];
        for (uint32_t i = 0; i < sizeof(filedata); ++i) filedata[i] = (char)(i % 256);
        assert(vfs->write(fd, filedata, sizeof(filedata)) == (ssize_t)sizeof(filedata));
        assert(vfs->seek(fd, 0, H_SEEK_SET) == 0);

        // Map the file privately into memory
        assert(ms.map_private_file(0x40000, fd, 0, region_size, perms) == 0);
        // Check mapping
        assert(ms.is_mapped(0x40000, region_size) == 1);
        assert(ms.how_many_mapped(0x40000, region_size) == 2);

        // Read from mapped region and compare to file
        char buf[page_size * 2] = {0};
        assert(ms.memcpy(buf, 0x40000, sizeof(buf)) == 0);
        assert(memcmp(buf, filedata, sizeof(buf)) == 0);

        // Write to mapped region and verify change is private
        for (uint32_t i = 0; i < sizeof(buf); ++i) buf[i] = (char)(255 - (i % 256));
        assert(ms.memcpy(0x40000, buf, sizeof(buf)) == 0);
        // Read back from memory
        char memcheck[page_size * 2] = {0};
        assert(ms.memcpy(memcheck, 0x40000, sizeof(memcheck)) == 0);
        assert(memcmp(memcheck, buf, sizeof(buf)) == 0);
        // Read from file again to verify file is unchanged
        assert(vfs->seek(fd, 0, H_SEEK_SET) == 0);
        char filecheck[page_size * 2] = {0};
        assert(vfs->read(fd, filecheck, sizeof(filecheck)) == (ssize_t)sizeof(filecheck));
        assert(memcmp(filecheck, filedata, sizeof(filedata)) == 0);

        // Unmap and cleanup
        assert(ms.unmap(0x40000, region_size) == 0);
        vfs->close(fd);
        assert(vfs->remove(mapfile) == 0);
    }

    // --- Shared File Mapping Test ---
    {
        Hamster::MemorySpace ms1, ms2;
        constexpr uint32_t page_size = HAMSTER_PAGE_SIZE;
        constexpr uint32_t region_size = page_size * 2;
        uint8_t perms = Hamster::PERM_READ | Hamster::PERM_WRITE;

        const char *sharedfile = "/shared_map.bin";
        int fd = vfs->open(sharedfile, OPEN_RDWR | OPEN_CREAT, 0644);
        assert(fd >= 0);
        char filedata[region_size];
        for (uint32_t i = 0; i < sizeof(filedata); ++i) filedata[i] = (char)(i % 256);
        assert(vfs->write(fd, filedata, sizeof(filedata)) == (ssize_t)sizeof(filedata));
        assert(vfs->seek(fd, 0, H_SEEK_SET) == 0);

        // Map the file shared into ms1
        assert(ms1.map_shared_file(0x50000, fd, 0, region_size, perms) == 0);
        // Write to ms1 mapping
        char newdata[region_size];
        for (uint32_t i = 0; i < sizeof(newdata); ++i) newdata[i] = (char)(255 - (i % 256));
        assert(ms1.memcpy(0x50000, newdata, sizeof(newdata)) == 0);

        // Copy ms1 to ms2 (should share the mapping)
        ms2 = ms1;

        // Read from ms2 and check it sees the new data
        char readback[region_size];
        assert(ms2.memcpy(readback, 0x50000, sizeof(readback)) == 0);
        assert(memcmp(readback, newdata, sizeof(newdata)) == 0);

        // Write different data in ms2
        for (uint32_t i = 0; i < sizeof(newdata); ++i) newdata[i] = (char)((i * 3) % 256);
        assert(ms2.memcpy(0x50000, newdata, sizeof(newdata)) == 0);

        // Read from ms1 and check it sees the new data
        char readback2[region_size];
        assert(ms1.memcpy(readback2, 0x50000, sizeof(readback2)) == 0);
        assert(memcmp(readback2, newdata, sizeof(newdata)) == 0);

        // Read from file and check it sees the new data (shared mapping)
        assert(vfs->seek(fd, 0, H_SEEK_SET) == 0);
        char filecheck[region_size];
        assert(vfs->read(fd, filecheck, sizeof(filecheck)) == (ssize_t)sizeof(filecheck));
        assert(memcmp(filecheck, newdata, sizeof(newdata)) == 0);

        // Cleanup
        assert(ms1.unmap(0x50000, region_size) == 0);
        assert(ms2.unmap(0x50000, region_size) == 0);
        vfs->close(fd);
        assert(vfs->remove(sharedfile) == 0);
    }

    // --- VFS::accessat tests ---
    {
        // Setup: create a directory and a file inside it
        const char *accdir = "/accdir";
        assert(vfs->mkdir(accdir, 0755) == 0);
        int dirfd = vfs->open(accdir, OPEN_RDONLY);
        assert(dirfd >= 0);
        const char *accfile = "accfile.txt";
        int fd = vfs->mkfileat(dirfd, accfile, OPEN_RDWR | OPEN_CREAT, 0640);
        assert(fd >= 0);
        vfs->close(fd);

        // User and group setup
        int owner_uid = 1001;
        int other_uid = 2002;
        int owner_gid = 3003;
        int other_gid = 4004;
        int groups1[] = {owner_gid};
        int groups2[] = {other_gid};


        // Change ownership (use path relative to dirfd)
        assert(vfs->chownat(dirfd, accfile, owner_uid, owner_gid) == 0);


        // Owner should have read/write access
        assert(vfs->accessat(dirfd, accfile, owner_uid, groups1, 1, 4) == 0); // read
        assert(vfs->accessat(dirfd, accfile, owner_uid, groups1, 1, 2) == 0); // write
        assert(vfs->accessat(dirfd, accfile, owner_uid, groups1, 1, 1) != 0); // execute (should fail)

        // Other user, not in group, should have no access
        assert(vfs->accessat(dirfd, accfile, other_uid, groups2, 1, 4) != 0); // read
        assert(vfs->accessat(dirfd, accfile, other_uid, groups2, 1, 2) != 0); // write

        // Other user, but in group, should have read access (since 0640)
        assert(vfs->accessat(dirfd, accfile, other_uid, groups1, 1, 4) == 0); // read
        assert(vfs->accessat(dirfd, accfile, other_uid, groups1, 1, 2) != 0); // write

        // Clean up
        assert(vfs->removeat(dirfd, accfile) == 0);
        vfs->close(dirfd);
        assert(vfs->remove(accdir) == 0);
    }

    // --- VFS::accessat advanced tests: intermediate directory permissions and multiple groups ---
    {
        // Setup: create nested directories and a file
        const char *topdir = "/topdir";
        const char *subdir = "subdir";
        const char *filename = "file.txt";
        assert(vfs->mkdir(topdir, 0755) == 0);
        int topfd = vfs->open(topdir, OPEN_RDONLY);
        assert(topfd >= 0);
        assert(vfs->mkdirat(topfd, subdir, 0750) == 0);
        int subfd = vfs->openat(topfd, subdir, OPEN_RDONLY);
        assert(subfd >= 0);
        int fd = vfs->mkfileat(subfd, filename, OPEN_RDWR | OPEN_CREAT, 0640);
        assert(fd >= 0);
        vfs->close(fd);

        // Set up users and groups
        int owner_uid = 1111;
        int groupA = 2222;
        int groupB = 3333;
        int groupC = 4444;
        int groupsA[] = {groupA};
        int groupsAB[] = {groupA, groupB};
        int groupsBC[] = {groupB, groupC};
        int other_uid = 5555;

        // Set ownerships
        assert(vfs->chownat(topfd, subdir, owner_uid, groupA) == 0);
        assert(vfs->chownat(subfd, filename, owner_uid, groupB) == 0);

        // /topdir
        //         /subdir - owner:A
        //                 /file.txt - owner:B

        // Owner should have access through both dirs
        assert(vfs->accessat(subfd, filename, owner_uid, groupsA, 1, 4) == 0); // read
        // User in groupB but not groupA: should fail due to subdir perms
        assert(vfs->accessat(subfd, filename, other_uid, groupsBC, 2, 4) != 0);
        // User in both groupA and groupB: should succeed
        assert(vfs->accessat(subfd, filename, other_uid, groupsAB, 2, 4) == 0);

        // Remove read/execute from subdir, only owner can access
        assert(vfs->chmod(subfd, 0700) == 0);
        // Now only owner can access
        assert(vfs->accessat(subfd, filename, owner_uid, groupsA, 1, 4) == 0);
        assert(vfs->accessat(subfd, filename, other_uid, groupsAB, 2, 4) != 0);

        // Clean up
        assert(vfs->removeat(subfd, filename) == 0);
        vfs->close(subfd);
        assert(vfs->removeat(topfd, subdir) == 0);
        vfs->close(topfd);
        assert(vfs->remove(topdir) == 0);
    }

    // --- Symlink access / accessat tests (including H_AT_SYMLINK_NOFOLLOW) ---
    {
        int empty[1] = {0};

        // dangling symlink at root
        const char *dang = "/danglink";
        // create a symlink pointing to a non-existent target
        assert(vfs->symlink(dang, "/no/such/target") == 0);

        // By default, access should try to follow the symlink and fail (target missing)
        assert(vfs->access(dang, 0, empty, 1, 4) < 0);

        // With H_AT_SYMLINK_NOFOLLOW, access should check the symlink itself and succeed
        assert(vfs->access(dang, 0, empty, 1, 4, H_AT_SYMLINK_NOFOLLOW) == 0);

        // Cleanup root symlink
        assert(vfs->remove(dang) == 0);

        // Now test accessat with a directory FD and a symlink inside it
        const char *adir = "/ad";
        const char *alinkname = "linkname";
        assert(vfs->mkdir(adir, 0755) == 0);
        int adfd = vfs->open(adir, OPEN_RDONLY);
        assert(adfd >= 0);

        // create dangling symlink relative to adir
        assert(vfs->symlinkat(adfd, alinkname, "/nonexistent") == 0);

        // accessat without NOFOLLOW should fail (follows and fails)
        assert(vfs->accessat(adfd, alinkname, 0, empty, 1, 4) < 0);

        // accessat with NOFOLLOW should succeed (checks symlink itself)
        assert(vfs->accessat(adfd, alinkname, 0, empty, 1, 4, H_AT_SYMLINK_NOFOLLOW) == 0);

        // cleanup
        assert(vfs->removeat(adfd, alinkname) == 0);
        vfs->close(adfd);
        assert(vfs->remove(adir) == 0);
    }

    // --- Symlink follow tests: verify access follows the symlink to target ---
    {
        const char *real = "/real_follow.txt";
        const char *slink = "/slink_follow";
        int empty[1] = {0};
        // create target file with owner-only permissions
        int rfd = vfs->mkfile(real, OPEN_RDWR | OPEN_CREAT, 0600);
        assert(rfd >= 0);
        // set ownership to uid 1001
        int owner_uid = 1001;
        int owner_gid = 1001;
        assert(vfs->chown(rfd, owner_uid, owner_gid) == 0);
        vfs->close(rfd);

        // create symlink pointing to the real file
        assert(vfs->symlink(slink, real) == 0);

        int other_uid = 2002;

        // access without NOFOLLOW should follow and check the target
        // owner should be allowed to read
        assert(vfs->access(slink, owner_uid, empty, 1, 4) == 0);
        // other user should not be allowed (target is 0600)
        assert(vfs->access(slink, other_uid, empty, 1, 4) < 0);

        // Now change symlink ownership to other_uid (lchown) so NOFOLLOW checks the symlink
        assert(vfs->lchown(slink, other_uid, owner_gid) == 0);

        // access with AT_SYMLINK_NOFOLLOW should check the symlink itself (owned by other_uid)
        assert(vfs->access(slink, other_uid, empty, 1, 4, H_AT_SYMLINK_NOFOLLOW) == 0);

        // Also test accessat: create a directory and place a symlink inside
        const char *dname = "/sdir";
        assert(vfs->mkdir(dname, 0755) == 0);
        int dfd2 = vfs->open(dname, OPEN_RDONLY);
        assert(dfd2 >= 0);
        // create symlink inside that points to the real file
        assert(vfs->symlinkat(dfd2, "inner", real) == 0);

        // follow: owner allowed, other not
        assert(vfs->accessat(dfd2, "inner", owner_uid, empty, 1, 4) == 0);
        assert(vfs->accessat(dfd2, "inner", other_uid, empty, 1, 4) < 0);

        // set symlink owner to other_uid via lchownat if available or lchown on path
        // use lchown on full path for simplicity
        assert(vfs->lchown("/slink_follow", other_uid, owner_gid) == 0);

        // nofollow via accessat should now succeed for other_uid
        assert(vfs->accessat(dfd2, "inner", other_uid, empty, 1, 4, H_AT_SYMLINK_NOFOLLOW) == 0);

        // cleanup
        assert(vfs->removeat(dfd2, "inner") == 0);
        vfs->close(dfd2);
        assert(vfs->remove(dname) == 0);
        assert(vfs->remove(slink) == 0);
        assert(vfs->remove(real) == 0);
    }

    assert(vfs->unmount("/") == 0);
}

#endif // NDEBUG
