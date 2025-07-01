// Hamster system call IDs

#pragma once

#include <cstdint>
#include <fcntl.h>
#include <sys/stat.h>

namespace Hamster
{
    struct Syscall
    {
        uint64_t syscall_num;
        uint64_t arg1;
        uint64_t arg2;
        uint64_t arg3;
        uint64_t arg4;
        uint64_t arg5;
        uint64_t arg6;
    };

    namespace SyscallID
    {
        // Newlib C system calls
        // Note that ones with RISC-V Linux equivelants use
        // that ID, but ones that don't have such an equivalent
        // use an ID starting from 512
        //
        // Ones without an equivelant are:
        // FORK FSTAT ISATTY LINK LSEEK OPEN UNLINK WAIT
        enum ID : uint16_t
        {
            EXIT = 93,
            CLOSE = 57,
            // environ not supported yet, but skip 2 for
            // future compatibility
            EXECVE = 221,
            FORK = 512,
            FSTAT = 513,
            GETPID = 172,
            ISATTY = 514,
            KILL = 129,
            LINK = 515,
            LSEEK = 516,
            OPEN = 517,
            READ = 63,
            // sbrk is not needed, as processes are free
            // to write to any memory they want, for now
            // but we skip 13 for future compatibility
            TIMES = 153,
            UNLINK = 518,
            WAIT = 519,
            WRITE = 64,
            RENAME = 520,
        };
    } // namespace SyscallID

    // Syscall ABI's

    struct Sys_stat
    {
        uint32_t dev;
        uint32_t ino;
        uint32_t mode;
        uint32_t nlink;
        uint32_t uid;
        uint32_t gid;
        uint32_t rdev;
        int64_t size;
        uint64_t atime;
        uint64_t mtime;
        uint64_t ctime;
        uint64_t blksize;
        uint64_t blocks;
    };

    inline struct ::stat map_sys_to_posix_stat(struct Sys_stat sys_stat)
    {
        struct ::stat statbuf;
        statbuf.st_dev = sys_stat.dev;
        statbuf.st_ino = sys_stat.ino;
        statbuf.st_mode = sys_stat.mode;
        statbuf.st_nlink = sys_stat.nlink;
        statbuf.st_uid = sys_stat.uid;
        statbuf.st_gid = sys_stat.gid;
        statbuf.st_rdev = sys_stat.rdev;
        statbuf.st_size = sys_stat.size;
        statbuf.st_atime = sys_stat.atime;
        statbuf.st_mtime = sys_stat.mtime;
        statbuf.st_ctime = sys_stat.ctime;
        statbuf.st_blksize = sys_stat.blksize;
        statbuf.st_blocks = sys_stat.blocks;
        return statbuf;
    }

    inline struct Sys_stat map_posix_to_sys_stat(struct ::stat stat_buf)
    {
        Sys_stat sys_stat_buf;
        sys_stat_buf.dev = stat_buf.st_dev;
        sys_stat_buf.ino = stat_buf.st_ino;
        sys_stat_buf.mode = stat_buf.st_mode;
        sys_stat_buf.nlink = stat_buf.st_nlink;
        sys_stat_buf.uid = stat_buf.st_uid;
        sys_stat_buf.gid = stat_buf.st_gid;
        sys_stat_buf.rdev = stat_buf.st_rdev;
        sys_stat_buf.size = stat_buf.st_size;
        sys_stat_buf.atime = stat_buf.st_atime;
        sys_stat_buf.mtime = stat_buf.st_mtime;
        sys_stat_buf.ctime = stat_buf.st_ctime;
        sys_stat_buf.blksize = stat_buf.st_blksize;
        sys_stat_buf.blocks = stat_buf.st_blocks;
        return sys_stat_buf;
    }

    enum Sys_o_macro
    {
        Sys_O_RDONLY = 0x0000,
        Sys_O_WRONLY = 0x0001,
        Sys_O_RDWR = 0x0002,
        Sys_O_ACCMODE = 0x0003,

        Sys_O_CREAT = 0x0100,
        Sys_O_EXCL = 0x0200,
        Sys_O_NOCTTY = 0x0400,
        Sys_O_TRUNC = 0x0800,

        Sys_O_APPEND = 0x1000,
        Sys_O_NONBLOCK = 0x2000,
        Sys_O_DSYNC = 0x4000,

        Sys_O_SYNC = 0x101000,
        Sys_O_RSYNC = 0x401000,

        Sys_O_DIRECTORY = 0x10000,
        Sys_O_NOFOLLOW = 0x20000,
        Sys_O_TMPFILE = 0x410000,
        Sys_O_ASYNC = 0x2000,
    };

    // Helper
    inline uint32_t map_posix_to_sys_flags(int posix_flags)
    {
        uint32_t sys_flags = 0;

        // Access mode
        switch (posix_flags & O_ACCMODE)
        {
        case O_RDONLY:
            sys_flags |= Sys_O_RDONLY;
            break;
        case O_WRONLY:
            sys_flags |= Sys_O_WRONLY;
            break;
        case O_RDWR:
            sys_flags |= Sys_O_RDWR;
            break;
        }

        // Creation and file status flags
        if (posix_flags & O_CREAT)
            sys_flags |= Sys_O_CREAT;
        if (posix_flags & O_EXCL)
            sys_flags |= Sys_O_EXCL;
        if (posix_flags & O_NOCTTY)
            sys_flags |= Sys_O_NOCTTY;
        if (posix_flags & O_TRUNC)
            sys_flags |= Sys_O_TRUNC;
        if (posix_flags & O_APPEND)
            sys_flags |= Sys_O_APPEND;
        if (posix_flags & O_NONBLOCK)
            sys_flags |= Sys_O_NONBLOCK;
#ifdef O_DSYNC
        if (posix_flags & O_DSYNC)
            sys_flags |= Sys_O_DSYNC;
#endif
#ifdef O_SYNC
        if (posix_flags & O_SYNC)
            sys_flags |= Sys_O_SYNC;
#endif
#ifdef O_RSYNC
        if (posix_flags & O_RSYNC)
            sys_flags |= Sys_O_RSYNC;
#endif
#ifdef O_ASYNC
        if (posix_flags & O_ASYNC)
            sys_flags |= Sys_O_ASYNC;
#endif

#ifdef O_DIRECTORY
        if (posix_flags & O_DIRECTORY)
            sys_flags |= Sys_O_DIRECTORY;
#endif
#ifdef O_NOFOLLOW
        if (posix_flags & O_NOFOLLOW)
            sys_flags |= Sys_O_NOFOLLOW;
#endif
#ifdef O_TMPFILE
        if (posix_flags & O_TMPFILE)
            sys_flags |= Sys_O_TMPFILE;
#endif

        return sys_flags;
    }

    inline int map_sys_to_posix_flags(uint32_t sys_flags)
    {
        int posix_flags = 0;

        // Access mode
        switch (sys_flags & Sys_O_ACCMODE)
        {
        case Sys_O_RDONLY:
            posix_flags |= O_RDONLY;
            break;
        case Sys_O_WRONLY:
            posix_flags |= O_WRONLY;
            break;
        case Sys_O_RDWR:
            posix_flags |= O_RDWR;
            break;
        }

        // Creation and status flags
        if (sys_flags & Sys_O_CREAT)
            posix_flags |= O_CREAT;
        if (sys_flags & Sys_O_EXCL)
            posix_flags |= O_EXCL;
        if (sys_flags & Sys_O_NOCTTY)
            posix_flags |= O_NOCTTY;
        if (sys_flags & Sys_O_TRUNC)
            posix_flags |= O_TRUNC;
        if (sys_flags & Sys_O_APPEND)
            posix_flags |= O_APPEND;
        if (sys_flags & Sys_O_NONBLOCK)
            posix_flags |= O_NONBLOCK;
#ifdef O_DSYNC
        if (sys_flags & Sys_O_DSYNC)
            posix_flags |= O_DSYNC;
#endif
#ifdef O_SYNC
        if (sys_flags & Sys_O_SYNC)
            posix_flags |= O_SYNC;
#endif
#ifdef O_RSYNC
        if (sys_flags & Sys_O_RSYNC)
            posix_flags |= O_RSYNC;
#endif
#ifdef O_ASYNC
        if (sys_flags & Sys_O_ASYNC)
            posix_flags |= O_ASYNC;
#endif

#ifdef O_DIRECTORY
        if (sys_flags & Sys_O_DIRECTORY)
            posix_flags |= O_DIRECTORY;
#endif
#ifdef O_NOFOLLOW
        if (sys_flags & Sys_O_NOFOLLOW)
            posix_flags |= O_NOFOLLOW;
#endif
#ifdef O_TMPFILE
        if (sys_flags & Sys_O_TMPFILE)
            posix_flags |= O_TMPFILE;
#endif

        return posix_flags;
    }
} // namespace Hamster
