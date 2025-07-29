// System call data structures

#pragma once

#include <cstdint>

namespace Hamster
{
    struct sys_clone_args
    {
        uint64_t flags;
        uint64_t pidfd;
        uint64_t child_tid;
        uint64_t parent_tid;
        uint64_t exit_signal;
        uint64_t stack;
        uint64_t stack_size;
        uint64_t tls;
        // We will not include `set_tid`, `set_tid_size`, and `cgroup` for now
    };

    struct sys_siginfo
    {
        int32_t signo;
        int32_t errno_value;
        int32_t code;

        union
        {
            struct
            {
                int32_t pid;
                int32_t uid;
            } kill;

            struct
            {
                int32_t pid;
                int32_t uid;
                int32_t status;
                uint32_t utime;
                uint32_t stime;
            } child;

            struct
            {
                uint32_t addr;

                union {
                    int16_t addr_lsb;
                    struct {
                        char _pad[4];
                        uint32_t lower;
                        uint32_t upper;
                    } addr_bnd;

                    // PKUERR and TRAP_PERF aren't used for now
                };
            } fault;

            struct
            {
                uint32_t band;
                int32_t fd;
            } poll;

            struct
            {
                uint32_t addr;
                int32_t syscall;
                uint32_t arch;
            } syscall;
        } fields;
    };

    struct sys_rusage
    {
        struct
        {
            int32_t sec;
            int32_t usec;
        } utime, stime;
        int32_t maxrss;
        int32_t ixrss;
        int32_t idrss;
        int32_t isrss;
        int32_t minflt;
        int32_t majflt;
        int32_t nswap;
        int32_t inblock;
        int32_t oublock;
        int32_t msgsnd;
        int32_t msgrcv;
        int32_t nsignals;
        int32_t nvcsw;
        int32_t nivcsw;
    };

    // From `struct stat64` in linux-headers
    struct sys_stat
    {
        uint64_t dev;
        uint64_t ino;
        uint32_t mode;
        uint32_t nlink;
        uint32_t uid;
        uint32_t gid;
        uint64_t rdev;
        uint64_t __pad1;
        int64_t size;
        int32_t blksize;
        int32_t __pad2;
        int64_t blocks;
        int32_t atime;
        uint32_t atime_nsec;
        int32_t mtime;
        uint32_t mtime_nsec;
        int32_t ctime;
        uint32_t ctime_nsec;
        uint32_t __unused4, __unused5;
    };

    struct sys_statx
    {
        uint32_t mask;
        uint32_t blksize;
        uint64_t attributes;
        uint32_t nlink;
        uint32_t uid;
        uint32_t gid;
        uint16_t mode;
        uint16_t __spare0[1];
        uint64_t ino;
        uint64_t size;
        uint64_t blocks;
        uint64_t attr_mask;

        struct {
            int64_t sec;
            uint32_t nsec;
            int32_t __reserved;
        } atime, btime, ctime, mtime;
        uint32_t rdev_major;
        uint32_t rdev_minor;
        uint32_t dev_major;
        uint32_t dev_minor;
        uint64_t mnt_id;
        uint32_t dio_memalign;
        uint32_t dio_offsetalign;
        uint64_t __spare3[12];
    };

    struct sys_dirent
    {
        uint64_t ino;
        uint64_t offset;
        uint16_t reclen;
        uint8_t type;

        // Acutal size longer, writing name will overflow buffer
        // Please write to this carefully, as to not write to invalid memory
        // We use this to replace a flexible array member
        char name[1];
    };

    struct sys_fd_set
    {
        // 1024 / (8 * sizeof(uint32_t)) = 32
        uint32_t fds_bits[32];
    };

    struct sys_timespec
    {
        int64_t sec;
        int64_t nsec;
    };

    struct sys_statfs
    {
        uint32_t type;
        uint32_t bsize;
        uint64_t blocks;
        uint64_t bfree;
        uint64_t bavail;
        uint64_t files;
        uint64_t ffree;
        uint64_t fsid;
        uint32_t namelen;
        uint32_t frsize;
        uint32_t flags;
        uint32_t spare[4];
    };
} // namespace Hamster
