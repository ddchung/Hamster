// Hamster task

#pragma once

#include <riscv/riscv_emulator.hpp>
#include <memory/stl_sequential.hpp>
#include <filesystem/vfs.hpp>
#include <cstdint>

namespace Hamster
{
    template <typename T>
    struct TaskMember
    {
        T obj;
        uint32_t refcount;
    };

    struct UserFD
    {
        int vfs_fd;
        int flags;
    };

    struct FDTable
    {
        Vector<UserFD> fds;
    };

    struct Filesystem
    {
        VFS vfs;
    };

    struct Process
    {
        uint32_t pid; // Process ID
        uint32_t ppid; // Parent Process ID
        uint32_t uid, euid;
        uint32_t gid, egid;

        uint32_t pgid; // Process Group ID
        uint32_t sid;
    };

    struct Task
    {
        TaskMember<EmulatorMemory> *memory;
        TaskMember<FDTable> *fd_table;
        TaskMember<Filesystem> *filesystem;
        TaskMember<Process> *process;

        uint32_t tid;
        RiscVEmulator emulator;
    };
} // namespace Hamster

