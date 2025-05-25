// Hamster process

#pragma once

#include <process/thread.hpp>
#include <memory/stl_map.hpp>
#include <memory/stl_sequential.hpp>
#include <memory/memory_space.hpp>


namespace Hamster
{
    struct ProcessFd
    {
        int fd;
        int fd_flags; // FD_CLOEXEC, but int to allow for future flags
    };

    struct Process
    {
        MemorySpace memory_space;
        UnorderedMap<uint32_t, size_t> reserved_mem; // For `lr` and `sc` instructions
        List<Thread> threads;

        Vector<ProcessFd> fds;

        uint32_t pid;
        uint32_t ppid;
        uint32_t pgid;
        uint32_t sid;
        uint32_t uid;
    };
} // namespace Hamster
