// test filesystem

#include <filesystem/mounts.hpp>
#include <filesystem/file.hpp>
#include <memory/allocator.hpp>
#include <cstring>
#include <cstdlib>
#include <cassert>

class TestFile : public Hamster::BaseRegularFile
{
public:
    TestFile() = default;
    ~TestFile() override = default;

    int rename(const char *name) override { return 0; }
    int remove() override { return 0; }

    int read(uint8_t *buf, size_t size) override { memset(buf, 0, size); return size; }
    int write(const uint8_t *buf, size_t size) override { return size; }

    int64_t seek(int64_t offset, int whence) override { return 0; }
};

class TestFilesystem : public Hamster::BaseFilesystem
{
public:
    TestFilesystem() = default;
    ~TestFilesystem() override = default;

    Hamster::BaseFile *open(const char *path, int flags, ...) override
    {
        return Hamster::alloc<TestFile>();
    }

    int rename(const char *old_path, const char *new_path) override { return 0; }
    int unlink(const char *path) override { return 0; }
    int link(const char *old_path, const char *new_path) override { return 0; }
    int stat(const char *path, struct ::stat *buf) override { return 0; }
    Hamster::BaseFile *mkfile(const char *name, int flags, int mode) override { return open(0, 0); }
    Hamster::BaseFile *mkdir(const char *name, int flags, int mode) override { return open(0, 0); }
    Hamster::BaseFile *mkfifo(const char *name, int flags, int mode) override { return open(0, 0); }
};

void test_filesystem()
{
    int i;

    Hamster::Mounts &mounts = Hamster::Mounts::instance();
    
    TestFilesystem *fs1 = Hamster::alloc<TestFilesystem>();
    TestFilesystem *fs2 = Hamster::alloc<TestFilesystem>();
    TestFilesystem *fs3 = Hamster::alloc<TestFilesystem>();

    // Test mount
    i = mounts.mount(fs1, "/mnt/test1");
    assert(i == 0);
    i = mounts.mount(fs2, "/mnt/test2");
    assert(i == 0);
    i = mounts.mount(fs3, "/");
    assert(i == 0);
    i = mounts.mount(nullptr, "/mnt/test3");
    assert(i != 0);
    i = mounts.mount(fs1, "/mnt/test1");
    assert(i != 0);

    // Test is_mounted
    i = mounts.is_mounted("/mnt/test1");
    assert(i == true);
    i = mounts.is_mounted("/mnt/test2");
    assert(i == true);
    i = mounts.is_mounted("/");
    assert(i == true);
    i = mounts.is_mounted("/mnt/test3");
    assert(i == false);
    i = mounts.is_mounted("/mnt/test4");
    assert(i == false);
    i = mounts.is_mounted(nullptr);
    assert(i == false);
    i = mounts.is_mounted("");
    assert(i == false);

    // Test get_mount
    auto mount = mounts.get_mount("/mnt/test1/file.txt");
    assert(mount.fs == fs1);
    i = strcmp(mount.path, "/file.txt");
    assert(i == 0);

    mount = mounts.get_mount("/mnt/test2/file.txt");
    assert(mount.fs == fs2);
    i = strcmp(mount.path, "/file.txt");
    assert(i == 0);

    mount = mounts.get_mount("/mnt/test3/file.txt");
    assert(mount.fs == fs3);
    i = strcmp(mount.path, "/mnt/test3/file.txt");
    assert(i == 0);

    mount = mounts.get_mount("/mnt/test4/file.txt");
    assert(mount.fs == fs3);
    i = strcmp(mount.path, "/mnt/test4/file.txt");
    assert(i == 0);

    // Test umount
    i = mounts.umount("/");
    assert(i == 0);

    mount = mounts.get_mount("/mnt/test4/file.txt");
    assert(mount.fs == nullptr);
    i = strcmp(mount.path, "/mnt/test4/file.txt");
    assert(i == 0);

    i = mounts.umount("/mnt/test1");
    assert(i == 0);
    i = mounts.umount("/mnt/test2");
    assert(i == 0);
    i = mounts.umount("/mnt/test3");
    assert(i != 0);
    i = mounts.umount("/mnt/test4");
    assert(i != 0);
    i = mounts.umount(nullptr);
    assert(i != 0);

    // umount automatically frees the filesystem
}
