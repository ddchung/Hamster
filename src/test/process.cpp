// Test TaskFDTable

#include <process/task_fd_table.hpp>
#include <process/task_base_fd.hpp>
#include <process/task_fs_info.hpp>
#include <memory/allocator.hpp>
#include <errno/errno.h>
#include <cassert>
#include <cstring>

using namespace Hamster;

#ifndef NDEBUG

// Dummy BaseTaskFD for testing
class DummyFD : public BaseTaskFD
{
public:
    int value;
    int flags;
    DummyFD(int v, int f = 0) : value(v), flags(f) {}
    int get_flags() override { return flags; }
    int set_flags(int f) override
    {
        flags = f;
        return 0;
    }
    ssize_t read(void *buf, size_t size)
    {
        error = H_ENOTSUP;
        return -1;
    };
    ssize_t write(const void *buf, size_t size)
    {
        error = H_ENOTSUP;
        return -1;
    };
    int64_t seek(int64_t off, int whence)
    {
        error = H_ENOTSUP;
        return -1;
    };
    int stat(sys_stat *buf)
    {
        error = H_ENOTSUP;
        return -1;
    };
    int truncate(int64_t size)
    {
        error = H_ENOTSUP;
        return -1;
    };
    int64_t size()
    {
        error = H_ENOTSUP;
        return -1;
    };
    int ioctl(int req, IoctlArg arg = IoctlArg())
    {
        error = H_ENOTSUP;
        return -1;
    };
    int poll(int op)
    {
        error = H_ENOTSUP;
        return -1;
    };
    int sync(int fd)
    {
        error = H_ENOTSUP;
        return -1;
    };
    int datasync(int fd)
    {
        error = H_ENOTSUP;
        return -1;
    };
    int get_vfs_fd()
    {
        error = H_EINVAL;
        return -1;
    }
};

void test_process()
{
    // Test FD table
    {
        TaskFDTable table;

        // Set and get
        DummyFD *fd1 = alloc<DummyFD>(1, 1, 123);

        // Allocate first one at 0
        assert(table.set_fd(fd1, -1) == 0);
        BaseTaskFD *out = table.get_fd(0);
        assert(out != nullptr);
        assert(static_cast<DummyFD *>(out)->value == 1);
        assert(static_cast<DummyFD *>(out)->flags == 123);

        // Duplicate (dup)
        assert(table.allocate_fd() == 1);
        assert(table.dup(0, 1) == 0);
        BaseTaskFD *out2 = table.get_fd(1);
        assert(out2 != nullptr);
        assert(static_cast<DummyFD *>(out2)->value == 1);
        assert(out2 == out);

        // Close
        assert(table.close(0) == 0);
        assert(table.get_fd(0) == nullptr);
        assert(table.get_fd(1) != nullptr);

        // Allocate fd
        int slot = table.allocate_fd();
        assert(slot >= 0);
        DummyFD *fd2 = alloc<DummyFD>(1, 2);
        assert(table.set_fd(fd2, slot) == 0);
        assert(static_cast<DummyFD *>(table.get_fd(slot))->value == 2);

        // close_cloexec
        DummyFD *fd3 = alloc<DummyFD>(1, 3, OPEN_CLOEXEC); // OPEN_CLOEXEC
        assert(table.allocate_fd() == 2);
        assert(table.set_fd(fd3, 2) == 0);
        table.close_cloexec();
        assert(table.get_fd(2) == nullptr);

        // clear
        table.clear();
        assert(table.get_fd(1) == nullptr);
        assert(table.get_fd(slot) == nullptr);
    }

    {
        Hamster::TaskFSInfo fsinfo;

        // Initial CWD and root should be "/"
        char *cwd = fsinfo.getcwd();
        assert(strcmp(cwd, "/") == 0);
        dealloc(cwd);
        char *abs_cwd = fsinfo.get_abs_cwd();
        assert(strcmp(abs_cwd, "/") == 0);
        dealloc(abs_cwd);

        // chdir to a relative path
        assert(fsinfo.chdir("tmp") == 0);
        cwd = fsinfo.getcwd();
        assert(strcmp(cwd, "/tmp") == 0);
        dealloc(cwd);

        // chdir to an absolute path
        assert(fsinfo.chdir("/var/log") == 0);
        cwd = fsinfo.getcwd();
        assert(strcmp(cwd, "/var/log") == 0);
        dealloc(cwd);

        // chroot and check cwd
        assert(fsinfo.chroot("/var") == 0);
        cwd = fsinfo.getcwd();
        assert(strcmp(cwd, "/log") == 0); // CWD relative to new root
        dealloc(cwd);
        abs_cwd = fsinfo.get_abs_cwd();
        assert(strcmp(abs_cwd, "/var/log") == 0);
        dealloc(abs_cwd);

        // chdir after chroot
        assert(fsinfo.chdir("/etc") == 0);
        cwd = fsinfo.getcwd();
        assert(strcmp(cwd, "/etc") == 0);
        dealloc(cwd);
        abs_cwd = fsinfo.get_abs_cwd();
        assert(strcmp(abs_cwd, "/var/etc") == 0);
        dealloc(abs_cwd);
    }
}

#endif // NDEBUG
