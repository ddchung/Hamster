// System call data structures

#pragma once

#include <abi/values.hpp>
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
        uint64_t _pad1;
        int64_t size;
        int32_t blksize;
        int32_t _pad2;
        int64_t blocks;
        int32_t atime;
        uint32_t atime_nsec;
        int32_t mtime;
        uint32_t mtime_nsec;
        int32_t ctime;
        uint32_t ctime_nsec;
        uint32_t _unused4, _unused5;
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
        uint16_t _spare0[1];
        uint64_t ino;
        uint64_t size;
        uint64_t blocks;
        uint64_t attr_mask;

        struct {
            int64_t sec;
            uint32_t nsec;
            int32_t _reserved;
        } atime, btime, ctime, mtime;
        uint32_t rdev_major;
        uint32_t rdev_minor;
        uint32_t dev_major;
        uint32_t dev_minor;
        uint64_t mnt_id;
        uint32_t dio_memalign;
        uint32_t dio_offsetalign;
        uint64_t _spare3[12];
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

    struct sys_termios
    {
        uint32_t iflag;
        uint32_t oflag;
        uint32_t cflag;
        uint32_t lflag;
        uint8_t line;
        uint8_t cc[19];
    };

    struct sys_winsize
    {
        uint16_t row;
        uint16_t col;
        uint16_t xpixel;
        uint16_t ypixel;
    };

    struct sys_sigset
    {
        uint32_t sig[2];
    };

    struct sys_sigaction
    {
        uint32_t handler;
        uint32_t flags;
        uint32_t restorer;
        sys_sigset mask;
    };

    struct sys_user_regs_struct
    {
        uint32_t pc;
        uint32_t regs[31];
    };

    union sys_fp_state
    {
        struct
        {
            uint32_t f[32];
            uint32_t fcsr;
        } f;

        struct
        {
            uint64_t f[32];
            uint32_t fcsr;
        } d;

        struct
        {
            uint64_t f[64] __attribute__((aligned(16)));
            uint32_t fcsr;
            uint32_t _reserved[3];
        } q;
    };

    struct sys_sigcontext
    {
        sys_user_regs_struct regs;
        sys_fp_state fpstate;
    };

    struct sys_ucontext
    {
        uint32_t flags;
        uint32_t link;
        struct {
            uint32_t _unused[3];
        } _unused_stack;
        sys_sigset sigmask;
        uint8_t _sigmask_reserved[1024 / 8 - sizeof(sys_sigset)];
        sys_sigcontext context;
    };

    struct sys_utsname
    {
        char sysname[65];
        char nodename[65];
        char release[65];
        char version[65];
        char machine[65];
        char domainname[65];
    };

    struct sys_pselect6_time64_sigset
    {
        uint32_t sigset_loc;
        uint32_t sigset_size;
    };

    struct sys_sigaltstack
    {
        uint32_t stack_loc;
        int32_t flags;
        uint32_t size;
    };

    struct sys_iovec
    {
        uint32_t data;
        uint32_t size;
    };

    struct sys_robust_list
    {
        uint32_t next;
    };

    struct sys_robust_list_head
    {
        sys_robust_list list;
        int32_t futex_offset;
        uint32_t list_op_pending_loc;
    };

    struct sys_pollfd
    {
        int32_t fd;
        int16_t events;
        int16_t revents;
    };

    using sys_sa_family_t = uint16_t;
    using sys_socklen_t = uint32_t;
    using sys_in_port_t = uint16_t;
    using sys_in_addr_t = uint32_t;

    struct sys_sockaddr
    {
        sys_sa_family_t family;
        char data[14];
    };

    struct sys_sockaddr_un
    {
        sys_sa_family_t family;
        char path[108];
    };

    struct sys_in_addr
    {
        sys_in_addr_t addr;
    };

    struct sys_sockaddr_in {
        sys_sa_family_t family;
        sys_in_port_t port;
        sys_in_addr addr;
        uint8_t zero[8];
    };

    inline uint64_t timespec_to_systick(const sys_timespec &ts)
    {
        // right now, 1 systick = 1 ms
        return (ts.sec * 1000) + (ts.nsec / 1000000);
    }
    inline sys_timespec systick_to_timespec(uint64_t systick)
    {
        return {
            .sec = static_cast<int64_t>(systick / 1000),
            .nsec = static_cast<int64_t>((systick % 1000) * 1000000)
        };
    }
    inline void systick_to_timespec(uint64_t systick, sys_timespec &ts)
    {
        ts.sec = static_cast<int64_t>(systick / 1000);
        ts.nsec = static_cast<int64_t>((systick % 1000) * 1000000);
    }
    inline sys_siginfo make_kill_siginfo(uint8_t signo, uint32_t uid = 0, uint32_t pid = 0)
    {
        sys_siginfo siginfo = {};
        siginfo.signo = signo;
        siginfo.code = H_SI_USER;
        siginfo.fields.kill.pid = pid;
        siginfo.fields.kill.uid = uid;
        return siginfo;
    }
} // namespace Hamster
