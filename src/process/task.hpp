// Hamster task

#pragma once

#include <riscv/riscv_emulator.hpp>
#include <memory/stl_sequential.hpp>
#include <memory/allocator.hpp>
#include <memory/stl_map.hpp>
#include <abi/structs.hpp>
#include <filesystem/file.hpp>
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
     * @brief Copy a task member
     * @param old The old task member to copy from, or nullptr to create a new one
     * @return A pointer to the new task member
     */
    template <typename T>
    TaskMember<T> *copy_task_member(TaskMember<T> *old = nullptr)
    {
        if (old)
        {
            auto res = alloc<TaskMember<T>>(1, *old);
            res->refcount = 1;
            return res;
        }
        else
        {
            auto res = alloc<TaskMember<T>>(1);
            res->refcount = 1;
            return res;
        }
    }

    /**
     * @brief Reference a task member
     * @param old The old task member to reference, or nullptr to create a new one
     * @return A pointer to the new task member
     * @note This will increment the reference count of the task member
     */
    template <typename T>
    TaskMember<T> *ref_task_member(TaskMember<T> *old = nullptr)
    {
        if (old)
        {
            old->refcount++;
            return old;
        }
        else
        {
            auto res = alloc<TaskMember<T>>(1);
            res->refcount = 1;
            return res;
        }
    }

    /**
     * @brief Create a new task member
     * @return A pointer to the new task member
     */
    template <typename T>
    TaskMember<T> *make_task_member()
    {
        auto res = alloc<TaskMember<T>>(1);
        res->refcount = 1;
        return res;
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

    enum class UserFDType : uint8_t
    {
        VFS,
        PID,
        PIPE_READ,
        PIPE_WRITE
    };

    struct UserFDPipe
    {
        Deque<char> buffer;
        uint8_t readers;
        uint8_t writers;

        /**
         * @brief When closing the pipe, use this to check
         *      * whether there are still references to the pipe.
         * @return true if the pipe can be destroyed, false otherwise
         */
        bool is_destroyable();

        /**
         * @brief Write to the pipe
         * @param buf The buffer to write
         * @param size The size of the buffer
         * @return The number of bytes read, or -1 on error and sets `error`
         */
        ssize_t write(const void *buf, size_t size);

        /**
         * @brief Read from the pipe
         * @param buf The buffer to read into
         * @param size The size of the buffer
         * @return The number of bytes read, or -1 on error and sets `error`
         */
        ssize_t read(void *buf, size_t size);

        /**
         * @brief Poll the pipe
         * @param op The events to poll for. Bitmask of 0x1 (read) and 0x2 (write)
         * @return 1 if ready, 0 if not ready, -1 on error and set `error`
         */
        int poll(int op);
    };

    inline constexpr int USER_FD_PIPE_NONBLOCK = 0x2;

    struct UserFD
    {
        // FD_CLOEXEC, and for pipes only USER_FD_PIPE_NONBLOCK;
        int flags;
        UserFDType type;

        // Offset, only used for directories
        int64_t dir_offset = 0;

        union
        {
            int vfs_fd;

            uint32_t pid;

            UserFDPipe *pipe;
        };
    };

    struct FDTable
    {
        Vector<UserFD> fds;
    };

    struct SignalHandler 
    {
        void (*fn)(struct Task *, sys_siginfo *, sys_sigaction *);
        sys_sigaction action;
    };

    struct SignalHandlers
    {
        SignalHandler sig_handlers[64];
    };

    struct FSInfo
    {
        // The starting point of the filesystem, as seen by the process
        String root_path = "/";

        // The current working directory of the process
        // This is relative to the absolute root path, not the process root path
        //
        // Note: Due to the way getcwd works, we must initialize this to "//" instead of "/"
        // ( It subtracts the root path from the CWD to get what the CWD is relative to the root, so
        //   "//", removing the root path at the beginning, which is "/", results in "/". )
        String cwd_path = "//";

        int umask = 0;
    };

    struct PendingSignal
    {
        sys_siginfo info;
    };

    struct PendingSignalQueue
    {
        Deque<PendingSignal> rt_sigqueue;
        Map<uint8_t, PendingSignal> normal_signals;
    };
    
    class Session
    {
    public:
        Vector<class ProcessGroup*> pgroups;
        uint32_t sid;
        DeviceID controlling_tty = {};
    };

    class ProcessGroup
    {
    public:
        /**
         * @brief Get the session ID of the process group
         * @return The session ID of the process group, or 0 if the process group is not associated with a session
         */
        uint32_t get_sid();

        ProcessGroup() = default;
        ~ProcessGroup();
        ProcessGroup(const ProcessGroup &) = delete;
        ProcessGroup &operator=(const ProcessGroup &) = delete;
        ProcessGroup(ProcessGroup &&);
        ProcessGroup &operator=(ProcessGroup &&);

        TaskMember<Session> *session;
        Vector<class Process*> processes;
        uint32_t pgid; // Process Group ID
    };

    enum class ProcessStateChangeType : uint8_t
    {
        EXIT,
        STOP,
        CONTINUE,
        // Abnormal termination, e.g. crash or signal
        TERMINATE,
    };

    struct ProcessStateChange
    {
        ProcessStateChangeType type;

        uint32_t uid, gid;

        union 
        {
            // EXIT
            uint8_t exit_code;

            // STOP, CONTINUE, TERMINATE
            uint8_t signal;
        };
    };

    class Process
    {
    public:

        Process() = default;
        ~Process();
        Process(const Process &) = delete;
        Process &operator=(const Process &) = delete;
        Process(Process &&);
        Process &operator=(Process &&);

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
         * @brief Ignore a signal for the process
         * @param signo The signal number to ignore
         * @return 0 on success, -1 on failure and set `error`
         * @note This will set the signal handler to a handler that does nothing
         */
        int ignore_signal(int signo);

        /**
         * @brief Set the handler for a signal to the default handler of that signal
         * @param signo The signal number to set the handler for
         * @return 0 on success, -1 on failure and set `error`
         * @note This will set it to the default handler, such as terminating for SIGINT, or doing nothing for SIGCHLD
         */
        int default_signal(int signo);

        /**
         * @brief Load an ELF executable into the process memory space
         * @param path The path to the ELF executable
         * @param argv The command line arguments for the ELF executable
         * @param envp The environment variables for the ELF executable
         * @param dirfd The directory file descriptor to open the executable in, or -1 for root
         * @return 0 on success, -1 on failure and set `error`
         * @note This will kill all threads, and replace the memory space
         */
        int exec_elf(const char *path, const char *const *argv, const char *const *envp, int dirfd = -1);

        /**
         * @brief Load an executable into the process memory space
         * @param path The path to the executable
         * @param argv The command line arguments for the executable
         * @param envp The environment variables for the executable
         * @param dirfd The directory file descriptor to open the executable in, or -1 for root
         * @return 0 on success, -1 on failure and set `error`
         * @note This will kill all threads, and replace the memory space
         * @note This can forward to `load_elf` if the executable is an ELF file, or run an
         *     * interpreter if the executable is a script
         */
        int exec(const char *path, const char *const *argv, const char *const *envp, int dirfd = -1);

        /**
         * @brief Join a process group
         * @param pg The process group to join
         * @return 0 on success, -1 on failure and set `error`
         * @note This will leave the current pgroup, if any, and join the new pgroup
         * @note If pg is null, it will just leave the current pgroup
         */
        int join_process_group(TaskMember<ProcessGroup> *pg);

        TaskMember<ProcessGroup> *pg;
        TaskMember<SignalHandlers> *signal_handlers;
        TaskMember<FSInfo> *fs_info;
        Vector<Task*> tasks;
        PendingSignalQueue shared_pending_signals;
        Map<uint32_t, ProcessStateChange> children_state_changes;
        Vector<uint32_t> supplementary_gids; // Supplementary group IDs

        uint32_t pid; // Process ID
        uint32_t ppid; // Parent Process ID
        uint32_t uid, euid, suid;
        uint32_t gid, egid, sgid;
    };

    class Task
    {
    public:
        Task() = default;
        ~Task();
        Task(const Task &) = delete;
        Task &operator=(const Task &) = delete;
        Task(Task &&);
        Task &operator=(Task &&);

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
         * @brief Do the exit routine for the task
         * @param exit_code The exit code of the task, constructed with `make_wait_*` functions
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
         * @brief Get a UserFD from the file descriptor table
         * @param fd The file descriptor to get
         * @return A pointer to the UserFD, or nullptr if the file descriptor is invalid, and set `error`
         */
        UserFD *get_user_fd(int fd);

        /**
         * @brief Make a new task
         * @param clone_flags The flags controlling which parts to reference or copy
         * @return A newly allocated task, or nullptr on failure and set `error`
         * @note This will allocate a new task, and copy or reference the necessary parts from
         *     * the current task, depending on the `clone_flags` provided.
         * @note The `clone_flags` can be a combination of the `H_CLONE_*` flags defined in `abi/values.hpp`
         * @note This does not set the `tid` field, and does not put the new thread's ID in the `tasks` vector
         *     * and also, if it created a new process, the pid field
         */
        Task *clone(uint32_t clone_flags);

        /**
         * @brief Get the memory space
         * @return A reference to the memory space
         */
        MemorySpace &get_memory();

        /**
         * @brief Copy an object to userspace
         * @param obj The object to copy.
         * @param addr The userspace address to copy to
         * @param size The size of the object. Defaults to `sizeof(T)`
         * @return 0 on success, -1 on error and set `error`
         * @warning T must be POD
         */
        template <typename T>
        int copy_to_user(const T &obj, uint32_t addr, size_t size = sizeof(T))
        {
            return get_memory().memcpy_alloc(addr, &obj, size);
        }

        /**
         * @brief Copy an object from userspace
         * @param out Copies here.
         * @param addr The userspace address of the object
         * @param size The size of the object. Defaults to `sizeof(T)`
         * @return 0 on success, -1 on error and set `error`
         */
        template <typename T>
        int copy_from_user(T &out, uint32_t addr, size_t size = sizeof(T))
        {
            return get_memory().memcpy(&out, addr, size);
        }

        /**
         * @brief Initialize the thread ID
         * @param new_id The new thread ID
         * @return 0 on success, -1 on error
         * @note This is only called when the task is added to the scheduler
         */
        int init_tid(int new_id);

        /**
         * @brief Send a signal to the task
         * @param signo The signal number to send
         * @param uid The user ID of the sender, or 0 if the sender is the kernel
         * @param pid The process ID of the sender, or 0 if the sender is the kernel
         * @return 0 on success, -1 on failure and set `error`
         */
        int send_signal(int signo, uint32_t uid = 0, uint32_t pid = 0);

        /**
         * @brief Send a signal to the task, with a signal info structure
         * @param siginfo The signal info structure to send
         * @return 0 on success, -1 on failure and set `error`
         */
        int send_signal(const sys_siginfo &siginfo);

        /**
         * @brief Check if a signal is blocked
         * @param signo The signal number to check
         * @return 1 if blocked, 0 if not, -1 on error and set `error`
         */
        int is_signal_blocked(int signo);

        /**
         * @brief Check if a signal is ignored
         * @param signo The signal number to check
         * @return 1 if ignored, 0 if not, -1 on error and set `error`
         */
        int is_signal_ignored(int signo);

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

        /**
         * @brief Process a user path, to make it absolute
         * @param path The path to process
         * @return The processed path, or nullptr on failure and set `error`
         * @note This will append either the CWD or the root path to the beginning of the path,
         *     * depending on whether the path is absolute or relative, and return a new string
         * @note This also takes ownership of the path, so it will deallocate it later
         */
        char *process_user_path(char *user_path);

        /**
         * @brief Close a file descriptor
         * @param fd The thread file descriptor to close
         * @return 0 on success, -1 on error
         */
        int close(int fd);

        TaskMember<EmulatorMemory> *memory;
        TaskMember<uint32_t> *program_brk;
        TaskMember<FDTable> *fd_table;
        TaskMember<Process> *process;
        PendingSignalQueue pending_signals;
        RiscVEmulator emulator;

        
        // Last time the task was scheduled
        // Value is the systick (see platform/platform.hpp)
        uint64_t last_tick = 0;
        
        // Signal bitmask, bits calculated as (1 << (signo - 1))
        // Warning: A signal is blocked if the bit is 0, and it is not blocked if the bit is 1
        // This is the opposite of the `sigprocmask` behavior, so be careful
        uint64_t sig_mask = 0xFFFFFFFFFFFFFFFF;
        
        // Blocking operation
        // If not nullptr, this is called and the tick is skipped
        // Must set itself to nullptr when done
        void (*blocking_operation)(Task &);

        // Blocking operation saved data
        uint32_t blocking_operation_saved[2];

        uint32_t tid = 0;
        uint32_t ptid = 0;

        // (code << 8) | (status & 0xFF)
        uint16_t exit_code = 0;
        
        // Sent to the parent process on exit, if we are the leader of the task group
        uint8_t exit_signal = 0;
        bool is_paused : 1 = false;
        bool is_dead : 1 = false;
    };

    // VFS file descriptor reference count
    extern UnorderedMap<int, unsigned int> fd_refcount;
} // namespace Hamster

