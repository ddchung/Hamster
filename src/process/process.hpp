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

    class Process
    {
    public:
        Process() = default;
        Process(const Process &);
        Process &operator=(const Process &);
        ~Process();
        /**
         * @brief Load an ELF file into the process
         * @param path The path to the ELF file
         * @warning This will kill all threads and overwrite the memory space, and create
         *        * a single new thread with the entry point of the ELF file
         * @note If `fds` is empty, it will open `/dev/console` 3 times for stdin, stdout, and stderr
         */
        int load_elf(const char *path);
        
        MemorySpace memory_space;
        UnorderedMap<uint32_t, size_t> reserved_mem; // For `lr` and `sc` instructions
        List<Thread> threads;
        Vector<ProcessFd> fds;

        String cwd;

        uint32_t pid;
        uint32_t ppid;
        uint32_t pgid;
        uint32_t sid;
        uint32_t uid;
        uint32_t gid;
        uint32_t euid;
        uint32_t egid;

        uint8_t exit_code;
    };
} // namespace Hamster
