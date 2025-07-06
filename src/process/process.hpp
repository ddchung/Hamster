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
         * @param argv Arguments to pass to the program. Note that by convention, the first argument is
         *      * the program name
         * @param envp Environment variables to pass to the program
         * @param dirfd Path is relative to this, or if not set, to the root directory
         * @warning This will kill all threads and overwrite the memory space, and create
         *        * a single new thread with the entry point of the ELF file
         * @note If `fds` is empty, it will open `/dev/console` 3 times for stdin, stdout, and stderr
         */
        int load_elf(const char *path, const char *const *argv = 0, const char *const *envp = 0, int dirfd = -1);
        
        MemorySpace memory_space;
        UnorderedMap<uint32_t, size_t> reserved_mem; // For `lr` and `sc` instructions
        List<Thread> threads;
        Vector<ProcessFd> fds;

        String cwd;

        uint32_t pid = 0;
        uint32_t ppid = 0;
        uint32_t pgid = 0;
        uint32_t sid = 0;
        uint32_t uid = 0;
        uint32_t gid = 0;
        uint32_t euid = 0;
        uint32_t egid = 0;

        int exit_status = 0;
    };

    extern UnorderedMap<int, unsigned int> fd_refcount;
} // namespace Hamster
