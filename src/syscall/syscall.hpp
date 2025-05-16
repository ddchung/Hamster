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
        enum ID : uint16_t
        {
            EXIT,

            // Minimal filesystem
            OPEN,
            READ,
            WRITE,
            SEEK,
            CLOSE,
        };
    } // namespace SyscallID
    
} // namespace Hamster

