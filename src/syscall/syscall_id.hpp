// Hamster system call IDs

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
        };
    } // namespace SyscallID
    
} // namespace Hamster

