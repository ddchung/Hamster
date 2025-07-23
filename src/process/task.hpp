// Hamster task

#pragma once

#include <riscv/riscv_emulator.hpp>
#include <memory/stl_sequential.hpp>
#include <memory/allocator.hpp>
#include <memory/stl_map.hpp>
#include <abi/structs.hpp>
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
    
    /**
     * @brief Create a new task member
     * @param old The old task member to copy from, or nullptr to create a new one
     * @param copy If true, the old task member will be copied, otherwise it will be reused and its reference count will be incremented
     * @return A pointer to the new task member
     */
    template <typename T>
    TaskMember<T> *make_task_member(TaskMember<T> *old = nullptr, bool copy = false)
    {
        if (old)
        {
            if (copy)
            {
                auto res = alloc<TaskMember<T>>(1, *old);
                res->refcount = 1;
                return res;
            }
            else
            {
                old->refcount++;
                return old;
            }
        }
        else
        {
            auto res = alloc<TaskMember<T>>(1);
            res->refcount = 1;
            return res;
        }
    }

    /**
     * @brief Destroy a pointer to task member
     * @param ptr The pointer to the task member to destroy
     * @note This will decrement the reference count of the task member, and if it reaches zero, it will deallocate the task member
     */
    template <typename T>
    void destroy_task_member(TaskMember<T> *ptr)
    {
        if (!ptr)
            return;

        ptr->refcount--;
        if (ptr->refcount == 0)
        {
            dealloc(ptr);
        }
    }

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

        // VFS-level file descriptor reference count
        // Used for dup and cloning
        UnorderedMap<int, size_t> fd_refcount;
    };

    using SignalHandler = void (*)(struct Task *, sys_siginfo *);

    struct SignalHandlers
    {
        SignalHandler sig_handlers[32];
    };

    struct FSInfo
    {
        // The starting point of the filesystem, as seen by the process
        String root_path;

        // The current working directory of the process
        // This is relative to the absolute root path, not the process root path
        String cwd_path;

        int umask;
    };
    
    class Session
    {
    public:
        Vector<uint32_t> pgroups;
        uint32_t sid;
    };

    class ProcessGroup
    {
    public:
        /**
         * @brief Get the session ID of the process group
         * @return The session ID of the process group, or 0 if the process group is not associated with a session
         */
        uint32_t get_sid();

        TaskMember<Session> *session;
        Vector<uint32_t> processes;
        uint32_t pgid; // Process Group ID
    };

    struct ZombieProcess
    {
        uint16_t exit_code; // (code << 8) | (status & 0xFF)
    };

    class Process
    {
    public:

        /**
         * @brief Get the session ID of the process
         * @return The session ID of the process, or 0 if the process is not associated with a session
         */
        uint32_t get_sid();

        /**
         * @brief Get the process group ID of the process
         * @return The process group ID of the process, or 0 if the process is not associated with a process group
         */
        uint32_t get_pgid();

        /**
         * @brief Populate the default set of signal handlers
         * This is called when the process is created.
         * @return 0 on success, -1 on failure and set `error`
         */
        int set_default_signal_handlers();

        /**
         * @brief Send a signal to the process
         * @param signo The signal number to send
         * @param uid The user ID of the sender, or 0 if the sender is the kernel
         * @param pid The process ID of the sender, or 0 if the sender is the kernel
         * @return 0 on success, -1 on failure and set `error`
         */
        int send_signal(int signo, uint32_t uid = 0, uint32_t pid = 0);

        /**
         * @brief Send a signal to the process, with a signal info structure
         * @param siginfo The signal info structure to send
         * @return 0 on success, -1 on failure and set `error`
         */
        int send_signal(const sys_siginfo &siginfo);

        /**
         * @brief Set a signal handler for the process
         * @param signo The signal number to set the handler for
         * @param handler The signal handler to set, or nullptr to reset the handler to the default handler
         * @return 0 on success, -1 on failure and set `error`
         * @note A possible failiure could be trying to set a handler for an unblockable signal, such as SIGKILL, SIGSTOP, or SIGCONT
         */
        int set_signal_handler(int signo, SignalHandler handler);

        /**
         * @brief Load an ELF executable into the process memory space
         * @param fd The (vfs) file descriptor of the ELF executable
         * @param argv The command line arguments for the ELF executable
         * @param envp The environment variables for the ELF executable
         * @return 0 on success, -1 on failure and set `error`
         * @note This will kill all threads, and replace the memory space
         */
        int exec_elf(int fd, const char *const *argv, const char *const *envp);

        /**
         * @brief Load an executable into the process memory space
         * @param fd The (vfs) file descriptor of the executable
         * @param argv The command line arguments for the executable
         * @param envp The environment variables for the executable
         * @return 0 on success, -1 on failure and set `error`
         * @note This will kill all threads, and replace the memory space
         * @note This can forward to `load_elf` if the executable is an ELF file, or run an
         *     * interpreter if the executable is a script
         */
        int exec(int fd, const char *const *argv, const char *const *envp);

        /**
         * @brief Get a VFS-level file descriptor to use for relative lookup preprocessing
         * @param path The path to use for the relative lookup
         * @param at_fd The VFS file descriptor to use as the base for relative lookups, or -100 to use the current working directory
         * @return The VFS-level file descriptor on success, -1 on failure and set `error`
         * @note This will resolve the path relative to the current working directory, or
         *     * the specified file descriptor, and return a VFS-level file descriptor that can
         *     * be used for further relative operations
         * @note The returned file descriptor targets:
         *     * - `cwd_path if `at_fd` is -100 and path is not absolute
         *     * - `root_path` if path is absolute (atfd does not matter in this case)
         *     * - `at_fd` if path is relative and `at_fd` is not -100
         */
        int get_relative_fd(const char *path, int at_fd = -100);

        TaskMember<ProcessGroup> *pg;
        TaskMember<SignalHandlers> *signal_handlers;
        TaskMember<FSInfo> *fs_info;
        Vector<uint32_t> tasks;
        Deque<sys_siginfo> shared_sig_queue;
        Map<uint32_t, ZombieProcess> zombies;

        uint32_t pid; // Process ID
        uint32_t ppid; // Parent Process ID
        uint32_t uid, euid;
        uint32_t gid, egid;
    };

    enum class BlockingOperation : uint8_t
    {
        NONE,
        IO_READ,
        IO_WRITE,
        WAIT,
    };

    class Task
    {
    public:

        /**
         * @brief Get the session ID of the task
         * @return The session ID of the task, or 0 if the task is not associated with a session
         */
        uint32_t get_sid();

        /**
         * @brief Get the process group ID of the task
         * @return The process group ID of the task, or 0 if the task is not associated with a process group
         */
        uint32_t get_pgid();

        /**
         * @brief Get the process ID of the task
         * @return The process ID of the task
         */
        uint32_t get_pid();

        /**
         * @brief Poll the blocking operation
         * @return -1 on an error, 0 otherwise
         * @note Forwards to one of: poll_read, poll_write, or poll_wait
         */
        int poll_block();

        /**
         * @brief Poll a blocking read operation
         * @return -1 on an error, 0 otherwise
         */
        int poll_read();

        /**
         * @brief Poll a blocking write operation
         * @return -1 on an error, 0 otherwise
         */
        int poll_write();

        /**
         * @brief Poll a blocking wait operation
         * @return -1 on an error, 0 otherwise
         */
        int poll_wait();

        /**
         * @brief Do the exit routine for the task
         * @param exit_code The exit code of the task, as (code << 8) | (status & 0xFF)
         * @return 0 on success, -1 on failure and set `error`
         * @note This can do things like marking the task as dead, possibly sending signals to parent processes, cleaning up
         *     * resources, etc.
         */
        int exit(uint16_t exit_code);

        /**
         * @brief Dereference a userspace file descriptor to a VFS-level file descriptor
         * @param fd The userspace file descriptor to dereference
         * @return The VFS-level file descriptor on success, -1 on failure and set `error`
         * @note This will dereference the userspace file descriptor to a VFS-level file descriptor,
         *     * and return the VFS-level file descriptor that can be used for VFS operations
         */
        int get_vfs_fd(int fd);

        /**
         * @brief Get the next unused file descriptor
         * @param start Searches for unused fd's >= start
         * @return The index of an unused file descriptor
         * @note This can make a new file descriptor if none are found
         */
        size_t get_unused_fd_index(int start = 0);

        /**
         * @brief Make a new task
         * @param clone_flags The flags controlling which parts to reference or copy
         * @return A newly allocated task, or nullptr on failure and set `error`
         * @note This will allocate a new task, and copy or reference the necessary parts from
         *     * the current task, depending on the `clone_flags` provided.
         * @note The `clone_flags` can be a combination of the `H_CLONE_*` flags defined in `abi/values.hpp`
         */
        Task *clone(uint32_t clone_flags);

        TaskMember<EmulatorMemory> *memory;
        TaskMember<FDTable> *fd_table;
        TaskMember<Filesystem> *filesystem;
        TaskMember<Process> *process;
        Deque<sys_siginfo> sig_queue;
        RiscVEmulator emulator;

        uint32_t tid;
        uint32_t ptid;
        
        // Signal bitmask, bits calculated as (1 << (signo - 1))
        uint32_t sig_mask;
        
        int io_block_fd;

        // (code << 8) | (status & 0xFF)
        uint16_t exit_code;
        
        // Sent to the parent process on exit, if we are the leader of the task group
        uint8_t exit_signal;
        BlockingOperation blocking_operation;
        bool is_paused : 1;
        bool is_dead : 1;
    };
} // namespace Hamster

