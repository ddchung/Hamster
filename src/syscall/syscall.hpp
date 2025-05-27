// System call datastructure

#pragma once

#include <cstdint>

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
        enum ID : uint16_t
        {
            EXIT = 0,
            CLOSE = 1,
            // environ not supported yet, but skip 2 for
            // future compatibility
            EXECVE = 3,
            FORK = 4,
            FSTAT = 5,
            GETPID = 6,
            ISATTY = 7,
            KILL = 8,
            LINK = 9,
            LSEEK = 10,
            OPEN = 11,
            READ = 12,
            // sbrk is not needed, as processes are free
            // to write to any memory they want, for now
            // but we skip 13 for future compatibility
            TIMES = 14,
            UNLINK = 15,
            WAIT = 16,
            WRITE = 17,
        };
    } // namespace SyscallID
    
} // namespace Hamster

