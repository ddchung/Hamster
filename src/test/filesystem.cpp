// test filesystem

#include <filesystem/mounts.hpp>
#include <filesystem/file.hpp>
#include <filesystem/ramfs.hpp>
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

    int stat(struct ::stat *) override { return 0; }
    int get_mode() override { return 0777; }
    int get_uid() override { return 0; }
    int get_gid() override { return 0; }
    int chmod(int) override { return 0; }
    int chown(int, int) override { return 0; }
    const char *name() override { return "TestFile"; }


    ssize_t read(uint8_t *buf, size_t size) override { memset(buf, 0, size); return size; }
    ssize_t write(const uint8_t *buf, size_t size) override { return size; }

    int64_t seek(int64_t offset, int whence) override { return 0; }
    int64_t tell() override { return 0; }
    int truncate(int64_t) override { return 0; }
    int64_t size() override { return 0; }
};

class TestFilesystem : public Hamster::BaseFilesystem
{
public:
    TestFilesystem() = default;
    ~TestFilesystem() override = default;
    
    using BaseFilesystem::open;
    Hamster::BaseFile *open(const char *path, int flags, va_list args) override
    {
        return Hamster::alloc<TestFile>();
    }

    int rename(const char *old_path, const char *new_path) override { return 0; }
    int unlink(const char *path) override { return 0; }
    int link(const char *old_path, const char *new_path) override { return 0; }
    int stat(const char *path, struct ::stat *buf) override { return 0; }
    Hamster::BaseRegularFile *mkfile(const char *name, int flags, int mode) override { return (Hamster::BaseRegularFile*)open(0, 0); }
    Hamster::BaseDirectory *mkdir(const char *name, int flags, int mode) override { return nullptr; }
    Hamster::BaseSymbolicFile *mksfile(const char *name, int flags, Hamster::FileType type, int mode) override { return nullptr; }
    int symlink(const char *old_path, const char *new_path) override { return 0; }
    int readlink(const char *path, char *buf, size_t buf_size) override { return 0; }
    char *readlink(const char *path) override { return nullptr; }
    Hamster::FileType type(const char *path) override { return Hamster::FileType::Regular; }
    int lstat(const char *path, struct ::stat *buf) override { return 0; }
};

void test_filesystem()
{
    int i;

    Hamster::Mounts &mounts = Hamster::mounts;
    
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

    // Test open
    Hamster::BaseFile *file = mounts.open("/mnt/test1/file.txt", 0);
    assert(file != nullptr);
    assert(file->type() == Hamster::FileType::Regular);
    i = file->remove();
    assert(i == 0);
    i = file->rename("new_file.txt");
    assert(i == 0);
    Hamster::dealloc<Hamster::BaseFile>(file);



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



    // Test ramfs
    Hamster::RamFs *ramfs = Hamster::alloc<Hamster::RamFs>();
    i = mounts.mount(ramfs, "/");
    assert(i == 0);
    i = mounts.is_mounted("/");
    assert(i == true);
    i = mounts.is_mounted("/mnt/test1");
    assert(i == false);
    i = mounts.is_mounted("/mnt/test2");
    assert(i == false);

    // mkdir
    Hamster::BaseDirectory *dir = mounts.mkdir("/mnt", O_RDWR, 0777);
    assert(dir != nullptr);
    assert(dir->type() == Hamster::FileType::Directory);
    Hamster::dealloc(dir);

    dir = mounts.mkdir("/mnt/test1", O_RDWR, 0777);
    assert(dir != nullptr);
    assert(dir->type() == Hamster::FileType::Directory);
    Hamster::dealloc(dir);

    // Test mkfile
    Hamster::BaseRegularFile *f2 = mounts.mkfile("/mnt/test1/file.txt", O_RDWR, 0777);
    assert(f2 != nullptr);

    assert(f2->type() == Hamster::FileType::Regular);
    i = f2->write((const uint8_t *)"Hello World", 11);
    assert(i == 11);
    i = f2->seek(0, 0);
    assert(i == 0);
    uint8_t buf[12];
    i = f2->read(buf, 11);
    assert(i == 11);
    buf[11] = 0;
    i = strcmp((const char *)buf, "Hello World");
    assert(i == 0);
    i = f2->remove();
    assert(i == 0);
    Hamster::dealloc(f2);


    const char* testData = "Hello, World!";
    size_t len = std::strlen(testData);

    // Create a new file
    f2 = mounts.mkfile("/mnt/test1/file.txt", O_RDWR, 0777);
    assert(f2 != nullptr);

    // Write data
    int w = f2->write(reinterpret_cast<const uint8_t*>(testData), len);
    assert(w >= 0 && static_cast<size_t>(w) == len);

    // Seek to beginning
    int64_t s = f2->seek(0, SEEK_SET);
    assert(s >= 0);

    // Read back data
    uint8_t buffer[32] = {0};
    int r = f2->read(buffer, len);
    assert(r >= 0 && static_cast<size_t>(r) == len);
    assert(std::memcmp(buffer, testData, len) == 0);

    // Seek to offset 100
    s = f2->seek(100, SEEK_SET);
    assert(s == 100);

    // Write single byte 'X'
    const char x = 'X';
    w = f2->write(reinterpret_cast<const uint8_t*>(&x), 1);
    assert(w == 1);

    // Seek to 100 and read it
    s = f2->seek(100, SEEK_SET);
    assert(s == 100);
    uint8_t readX = 0;
    r = f2->read(&readX, 1);
    assert(r == 1);
    assert(readX == 'X');

    // Reset file by removing and creating it again
    i = f2->remove();
    assert(i == 0);
    f2 = mounts.mkfile("/mnt/test1/file.txt", O_RDWR, 0777);
    assert(f2 != nullptr);

    // Write "abcdef" at start
    s = f2->seek(0, SEEK_SET);
    assert(s == 0);
    const char data2[] = "abcdef";
    w = f2->write(reinterpret_cast<const uint8_t*>(data2), 6);
    assert(w == 6);

    // Seek to -2 from end and read
    s = f2->seek(-2, SEEK_END);
    assert(s >= 0);
    uint8_t ch = 0;
    r = f2->read(&ch, 1);
    assert(r == 1);
    assert(ch == 'e');

    // Seek -1 from current and read again
    s = f2->seek(-1, SEEK_CUR);
    assert(s >= 0);
    r = f2->read(&ch, 1);
    assert(r == 1);
    assert(ch == 'e');

    mounts.umount("/");
}
