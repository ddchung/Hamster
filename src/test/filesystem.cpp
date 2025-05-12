// Test filesystem

#include <filesystem/vfs.hpp>
#include <filesystem/ramfs.hpp>
#include <memory/allocator.hpp>
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
    struct stat st;
    assert(vfs->stat(vfs->open(path, O_RDONLY), &st) == 0);
    assert(S_ISREG(st.st_mode));
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

    // Basename
    char *name = vfs->basename(fd);
    assert(strcmp(name, "file.txt") == 0);
    dealloc<char>(name);

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
    fd = vfs->open(path, O_RDONLY);
    assert(fd >= 0);
    assert(vfs->remove(fd) == 0);
    vfs->close(fd);

    // 7. Rename by fd
    fd = vfs->mkfile(path, O_RDWR | O_CREAT, 0644);
    assert(fd >= 0);
    assert(vfs->rename(fd, "renamed.txt") == 0);
    vfs->close(fd);

    // Unmount and cleanup
    assert(vfs->unmount("/") == 0);
    dealloc<VFS>(vfs);
}
