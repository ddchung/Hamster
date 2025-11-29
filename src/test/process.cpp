// Test TaskFDTable

#include <process/task_fd_table.hpp>
#include <process/task_base_fd.hpp>
#include <process/task_fs_info.hpp>
#include <process/task_signal_mask.hpp>
#include <process/task_signal_queue.hpp>
#include <process/task_signal_handlers.hpp>
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
    int sync()
    {
        error = H_ENOTSUP;
        return -1;
    };
    int datasync()
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
        assert(table.set_fd(fd2, slot) == slot);
        assert(static_cast<DummyFD *>(table.get_fd(slot))->value == 2);

        // close_cloexec
        DummyFD *fd3 = alloc<DummyFD>(1, 3);
        assert(table.allocate_fd() == 2);
        assert(table.set_fd(fd3, 2) == 2);
        assert(table.set_fd_flags(2, H_FD_CLOEXEC) == 0);
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

    // Test TaskSignalMask
    {
        Hamster::TaskSignalMask mask;
        // Initially, all signals should be unblocked
        for (uint8_t i = 1; i <= 64; ++i)
        {
            assert(mask.check(i) == 0);
        }
        // Block a signal
        mask.block(2);
        assert(mask.check(2) == 1);
        // Unblock it
        mask.unblock(2);
        assert(mask.check(2) == 0);
        // Block multiple signals
        mask.block(1);
        mask.block(3);
        mask.block(64);
        assert(mask.check(1) == 1);
        assert(mask.check(3) == 1);
        assert(mask.check(64) == 1);
        // Convert to uint64_t
        uint64_t m = mask.convert();
        assert((m & 1) == 1);                       // signal 1
        assert((m & (1ULL << 2)) == (1ULL << 2));   // signal 3
        assert((m & (1ULL << 63)) == (1ULL << 63)); // signal 64
        // Invert
        uint64_t inv = mask.convert(true);
        assert((inv & 1) == 0);
        assert((inv & (1ULL << 2)) == 0);
        assert((inv & (1ULL << 63)) == 0);
        // to_sigset
        sys_sigset set = mask.to_sigset();
        // Should match the mask
        assert(set.sig[0] == (uint32_t)(m & 0xFFFFFFFF));
        assert(set.sig[1] == (uint32_t)(m >> 32));
    }

    // Test TaskSignalQueue
    {
        Hamster::TaskSignalQueue queue;
        Hamster::TaskSignalMask mask;
        Hamster::sys_siginfo s1 = {2, 0, 0, {.kill = {100, 200}}};
        Hamster::sys_siginfo s2 = {3, 0, 1, {.kill = {101, 201}}};
        Hamster::sys_siginfo s3 = {2, 0, 2, {.kill = {102, 202}}};   // same signo as s1
        Hamster::sys_siginfo srt = {35, 0, 0, {.kill = {111, 222}}}; // realtime

        // Initially empty
        assert(queue.size() == 0);
        // Push normal signals
        assert(queue.push(s1) == 0);
        assert(queue.size() == 1);
        assert(queue.push(s2) == 0);
        assert(queue.size() == 2);
        // Push another with same signo (should replace)
        assert(queue.push(s3) == 0);
        assert(queue.size() == 2);
        // Push realtime
        assert(queue.push(srt) == 0);
        assert(queue.size() == 3);

        // Mask signal 2, should see signal 3
        mask.block(2);
        const Hamster::sys_siginfo *peeked = queue.peek(mask);
        assert(peeked && peeked->signo == 3);
        // Unmask signal 2, should see s3
        mask.unblock(2);
        peeked = queue.peek(mask);
        assert(peeked && peeked->signo == 2 && peeked->code == 2);
        // Pop signal 2
        queue.pop(mask);
        assert(queue.size() == 2);
        // Now should see signal 3
        peeked = queue.peek(mask);
        assert(peeked && peeked->signo == 3);
        // Pop signal 3
        queue.pop(mask);
        assert(queue.size() == 1);
        // Now should see realtime
        peeked = queue.peek(mask);
        assert(peeked && peeked->signo == 35);
        // Pop realtime
        queue.pop(mask);
        assert(queue.size() == 0);
        // Peek empty
        assert(queue.peek(mask) == nullptr);
    }

    // Test TaskSignalHandlers
    {
        Hamster::TaskSignalHandlers handlers;

        // static to be able to access in lambda
        static bool called;
        called = false;

        // Dummy handler
        auto my_handler = [](Hamster::Task &, const Hamster::sys_siginfo &, const Hamster::sys_sigaction &)
        {
            *(volatile bool *)&called = true;
        };
        Hamster::sys_sigaction action = {};

        // Initially, all signals should be default
        for (uint8_t i = 1; i <= 64; ++i)
        {
            assert(handlers.is_default(i) == 1);
            assert(handlers.is_handler(i) == 0);
            assert(handlers.is_ignored(i) == 0);
        }
        // Set handler for signal 2
        assert(handlers.set_handler(2, my_handler, action) == 0);
        assert(handlers.is_handler(2) == 1);
        assert(handlers.is_default(2) == 0);
        assert(handlers.is_ignored(2) == 0);
        // Set ignore for signal 3
        assert(handlers.set_ignore(3) == 0);
        assert(handlers.is_ignored(3) == 1);
        assert(handlers.is_default(3) == 0);
        assert(handlers.is_handler(3) == 0);
        // Set default for signal 2
        assert(handlers.set_default(2) == 0);
        assert(handlers.is_default(2) == 1);
        assert(handlers.is_handler(2) == 0);
        assert(handlers.is_ignored(2) == 0);
        // Set handler again and call it
        assert(handlers.set_handler(2, my_handler, action) == 0);

        struct
        {
        } dummy_task;

        Hamster::sys_siginfo siginfo = {2, 0, 0, {.kill = {0, 0}}};
        called = false;

        // Note: cast from dummy struct to Task
        //       we aren't going to use the value anyway, and
        //       properly initializing a full Task will be too costly
        handlers.handle_signal(2, (class Task &)dummy_task, siginfo);
        assert(called);
    }
}

#endif // NDEBUG
