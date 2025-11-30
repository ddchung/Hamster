// Hamster task

#pragma once

#include <process/task_signal_handlers.hpp>
#include <process/task_signal_queue.hpp>
#include <process/task_signal_mask.hpp>
#include <process/task_fd_table.hpp>
#include <process/task_fs_info.hpp>
#include <riscv/riscv_emulator.hpp>
#include <kscheduler/kscheduler.hpp>
#include <memory/stl_sequential.hpp>
#include <memory/memory_space.hpp>
#include <memory/shared_ptr.hpp>
#include <memory/allocator.hpp>
#include <memory/stl_map.hpp>
#include <memory/stl_set.hpp>
#include <abi/structs.hpp>

namespace Hamster
{
    struct ProcessStateChange
    {
        enum class Type
        {
            EXIT, // Includes exiting, terminating, coredumping, etc.
            STOP,
            CONT,
        };

        using enum Type;

        Type type;
        uint32_t pid;
        uint32_t pgid;
        uint32_t uid;

        union
        {
            // EXIT
            uint32_t exit_code;

            // STOP, CONT
            uint8_t signo;
        };
        
    };

    class Session
    {
    public:
        Session(uint32_t sid);
        ~Session() = default;

        Session(const Session &) = delete;
        Session &operator=(const Session &) = delete;
        Session(Session &&) = delete;
        Session &operator=(Session &&) = delete;

        /**
         * @brief Add a process group to the session
         * @param pgroup The process group
         */
        void add_process_group(class ProcessGroup *pgroup) { pgroups.emplace(pgroup); }

        /**
         * @brief Remove a process group from the session
         * @param pgroup The process group to remove
         */
        void remove_process_group(class ProcessGroup *pgroup) { pgroups.erase(pgroup); }

        const Set<class ProcessGroup *> &get_process_groups() const { return pgroups; }

        uint32_t get_sid() const { return sid; }

    private:
        Set<class ProcessGroup *> pgroups; // weak pointers
        uint32_t sid;
        // TODO: controlling tty
    };

    class ProcessGroup
    {
    public:
        ProcessGroup(uint32_t pgid, const SharedPtr<Session> &session = nullptr);
        ~ProcessGroup();

        ProcessGroup(const ProcessGroup &) = delete;
        ProcessGroup &operator=(const ProcessGroup &) = delete;
        ProcessGroup(ProcessGroup &&) = delete;
        ProcessGroup &operator=(ProcessGroup &&) = delete;

        /**
         * @brief Add a process to the pgroup
         * @param process The process to add
         */
        void add_process(class Process *process) { processes.emplace(process); }
        
        /**
         * @brief Remove a process from the group
         * @param process The process to remove
         */
        void remove_process(class Process *process) { processes.erase(process); }

        const Set<class Process *> &get_processes() const { return processes; }

        uint32_t get_pgid() const { return pgid; }

        const SharedPtr<Session> &get_session() const { return session; }

        // There must be at least one process in the group
        const SharedPtr<ProcessGroup> &get_shared_ptr() const;

    private:
        SharedPtr<Session> session;
        Set<class Process *> processes; // weak pointers
        uint32_t pgid;
    };

    class Process
    {
    public:
        // Note: This automatically adds `leader` into `this->tasks`
        Process(uint32_t pid, class Task *leader, const SharedPtr<ProcessGroup> &pgroup = nullptr,
                const SharedPtr<TaskSignalHandlers> &signal_handlers = nullptr,
                const SharedPtr<TaskFSInfo> &fs_info = nullptr, Process *parent = nullptr, int uid = 0, int euid = 0, int suid = 0,
                int gid = 0, int egid = 0, int sgid = 0, const Vector<int> &groups = {});
        ~Process() = default;

        Process(const Process &) = delete;
        Process &operator=(const Process &) = delete;
        Process(Process &&) = delete;
        Process &operator=(Process &&) = delete;

        /**
         * @brief Add a thread to the process
         * @param task The task to add
         */
        void add_task(Task *task);

        /**
         * @brief Remove a task from the process
         * @param task The task to remove
         */
        void remove_task(Task *task);

        /**
         * @brief Load an executable file
         * @param fd The VFS file descriptor of the file
         * @param leader The new leader task
         * @param argv The arguments. nullptr for empty arguments
         * @param envp The environment variables. nullptr for empty environment
         * @return 0 on success, -1 on error
         * @note This will clear all tasks except one, and wipe the memory space
         * @note `leader` must be one of the tasks in this process
         */
        int exec(int fd, Task *leader, const char *const *argv = nullptr, const char *const *envp = nullptr);

        /**
         * @brief Make the process exit
         * @param code The exit code to exit with
         * @return 0 on success, -1 on error
         */
        int exit(uint16_t code);

        /**
         * @brief Set the process group ID
         * @param pgid The new process group ID. If 0, set to the process's PID
         * @return 0 on success, -1 on error
         * @note If `pgid != 0`, the process group must already exist in the same session
         */
        int set_pgid(uint32_t pgid);

        /**
         * @brief Make and join a new session
         * @return 0 on success, -1 on error
         * @note The calling process must not already be a process group leader
         */
        int setsid();

        /**
         * @brief Send a pause state change to parent process
         * @param signo Signal that caused the pause
         */
        void notify_pause(uint8_t signo);

        /**
         * @brief Send a continue state change to parent process
         */
        void notify_continue();

        uint32_t get_pid() const { return pid; }
        uint32_t get_ppid() const { return parent ? parent->pid : 0; }
        const SharedPtr<ProcessGroup> &get_process_group() const { return pgroup; }
        const SharedPtr<TaskSignalHandlers> &get_signal_handlers() const { return signal_handlers; }
        const SharedPtr<TaskFSInfo> &get_fs_info() const { return fs_info; }
        TaskSignalQueue &get_pending_signals() { return pending_signals; }
        Deque<ProcessStateChange> &get_state_changes() { return state_changes; }
        size_t num_tasks() const { return tasks.size(); }
        Task *get_leader() const { return leader; }
        Process *get_parent() const { return parent; }
        void get_uid(int *uid, int *euid, int *suid) const;
        void get_gid(int *gid, int *egid, int *sgid) const;
        void set_uid(int uid = -1, int euid = -1, int suid = -1);
        void set_gid(int gid = -1, int egid = -1, int sgid = -1);
        const Vector<int> &get_groups() const { return groups; }
        int set_groups(const Vector<int> &groups);
        const Set<Task *> &get_tasks() const { return tasks; }
        const Set<Process *> &get_children() const { return children; }

    private:
        SharedPtr<ProcessGroup> pgroup;
        SharedPtr<TaskSignalHandlers> signal_handlers;
        SharedPtr<TaskFSInfo> fs_info;
        Set<class Task *> tasks; // weak pointers
        Set<Process *> children;
        TaskSignalQueue pending_signals;
        Deque<ProcessStateChange> state_changes;
        Task *leader; // weak pointer to leader task, also present in `tasks`
        Process *parent; // weak pointer, may be null
        uint32_t pid;
        int uid, euid, suid;
        int gid, egid, sgid;
        Vector<int> groups;
    };

    class Task : private BaseKTask
    {
        struct Memory
        {
            MemorySpace ms;
            uint32_t brk;
        };
    public:
        Task(const Task &) = delete;
        Task &operator=(const Task &) = delete;
        Task(Task &&) = delete;
        Task &operator=(Task &&) = delete;
        ~Task() = default;

        using BlockingCallback = void (*)(Task &, uint64_t);

        // Give access to default constructor
        friend Task *alloc<Task>(size_t N);

        /**
         * @brief Make a new task, from this one
         * @param flags Flags controlling which parts to share and which ones to copy
         * @param stack, ptid_loc, tls, ctid_loc See Linux `clone` syscall docs
         * @return A weak pointer to new task, or nullptr on error and set `error`
         * @note See documentation on the `clone` Linux system call for more info. Note that
         *       this is *NOT* the glibc wrapper that accepts a function, but the raw system call.
         */
        Task *clone(uint32_t flags, uint32_t stack, uint32_t ptid_loc, uint32_t tls, uint32_t ctid_loc);

        /**
         * @brief Make an entirely new task
         * @param fd The file descriptor of the executable file to load
         * @param argv The arguments for the task
         * @param envp The environment variables for the task
         * @return The new task, or nullptr on error and set `error`
         * @note This creates a completely new task, in its own process, process group, and session
         */
        static Task *create_task(int fd, const char *const *argv = nullptr, const char *const *envp = nullptr);

        /**
         * @brief Get a task by TID
         * @param tid The TID of the task to get
         * @return A weak pointer to the task, or nullptr on error and set `error`
         */
        static Task *get_task(uint32_t tid);

        /**
         * @brief Get a task by PID
         * @param pid The PID of the task to get
         * @return A weak pointer to any matching task, or nullptr on error and set `error`
         */
        static Task *get_task_pid(uint32_t pid);

        /**
         * @brief Get a task by PGID
         * @param pgid The PGID of the task to get
         * @return A weak pointer any task in the pgroup, or nullptr on error and set `error`
         */
        static Task *get_task_pgid(uint32_t pgid);

        /**
         * @brief Get a pointer to the init process
         * @warning Don't use this directly. Intended for use by Process.
         */
        static Process *get_init_process();

        /**
         * @brief Make this task exit
         * @param code The exit code
         * @return 0 on success, -1 on error
         * @note `code` is only used if this is the last task in the process
         */
        int exit(uint16_t code);

        /**
         * @brief Make all tasks in the process exit
         * @param code The exit code
         * @return 0 on success, -1 on error
         */
        int exit_group(uint16_t code);

        /**
         * @brief Pause the process
         * @param signo The signal number that caused the pause
   */
        void pause(uint8_t signo);

        /**
         * @brief Unpause the process
         */
        void unpause();

        /**
         * @brief Check if the task is permitted to send a signal to another task
         * @param other The other task
         * @param signal The signal in question
         * @return 0 if OK, -1 otherwise and set `error`
         */
        int check_can_signal(Task &other, const sys_siginfo &signal);

        /**
         * @brief Get the last tick time
         * @return The system tick that the task last executed an instruction
         */
        uint64_t get_last_tick();

        /**
         * @brief Open a relative directory file descriptor
         * @param thread_dfd The userspace relative file descriptor
         * @param path The path
         * @return A VFS file descriptor `fd` such that doing a relative operation `*at(fd, path)` will
         *         result in the intended target. -1 on error and set `error`
         */
        int open_rel_fd(int thread_dfd, const char *path);

        /**
         * @brief Open a file, given a path, relative directory, and flags
         * @param thread_dfd The userspace relative file descriptor
         * @param path The path of the file. May be null if flags has AT_EMPTY_PATH
         * @param flags Open flags, and AT_EMPTY_PATH, and `0x03` to check for executability
         * @return A new VFS file descriptor to that file, or -1 on error and set `error`
         * @note `flags` will be cleared of `OPEN_CREAT`
         * @note Use the custom flag `0x03` to do permissions checking with X_OK too
         */
        int open_rel_file(int thread_dfd, const char *path, int flags);

        /**
         * @brief Enter a blocking operation
         * @param callback The blocking callback, called every once in a while
         * @param saved Data to save information for the blocking operation callback
         * @param interrupt_callback The callback to call to interrupt the blocking operation partway through
         * @return 0 on success, -1 on error
         * @note By default, the interrupt callback sets register `a0` to `-EINTR` and ends blocking
         */
        int block(BlockingCallback callback, uint64_t saved, BlockingCallback interrupt_callback = nullptr);

        /**
         * @brief Enter a blocking operation
         * @param callback The blocking callback. Called with registers from a0-a5
         *      * and continues blocking if it errors with EAGAIN and returns -1
         * @return 0 on success, -1 on error
         * @note This will set the `a0` register to the return value of callback once completed, and if it is
         *       -1, then it will set it to an error code instead
         * @note To return a literal `-EAGAIN` to userspace, return `- H_EAGAIN` instead of `-1` and setting error
         */
        int block(int (*callback)(Task &, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t));

        /**
         * @brief Interrupt the blocking operation
         * @return 0 on success, -1 on error
         */
        int interrupt_block();

        /**
         * @brief End the blocking operation. Should be used only in blocking callbacks
         */
        void end_block();

        /**
         * @brief Send a signal to all permitted processes except PID 1
         * @param siginfo The signal to send
         */
        void signal_all_processes(const sys_siginfo &siginfo);

        /**
         * @brief Load an executable into the task's memory space
         * @param fd The VFS file descriptor of the executable file
         * @param argv The arguments
         * @param envp The environment variables
         * @return 0 on success, -1 on error
         * @warning Don't use this directly, use `Task::exec` instead. this function is
         *          to be used internally by `Process::exec`
         * @note Don't pass nullptr for argv/envp, pass empty arrays instead
         */
        int load_executable(int fd, const char *const *argv, const char *const *envp);

        /**
         * @brief Replace the process image with a new executable
         * @param fd The VFS file descriptor of the executable file
         * @param argv The arguments. nullptr for empty arguments
         * @param envp The environment variables. nullptr for empty environment
         * @return 0 on success, -1 on error
         * @warning This will kill all other tasks in the current process
         */
        int exec(int fd, const char *const *argv = nullptr, const char *const *envp = nullptr);

        /**
         * @brief Copy a POD structure into the task's memory space
         * @param dest The destination address in the task's memory space
         * @param src The source structure
         * @param size The size of the structure, in bytes. Defaults to sizeof(T)
         * @return 0 on success, -1 on error
         */
        template <typename T>
        int copy_to_memory(uint32_t dest, const T &src, size_t size = sizeof(T))
        {
            return memory->ms.memcpy(dest, &src, size);
        }

        /**
         * @brief Copy a POD structure from the task's memory space
         * @param dest The destination structure
         * @param src The source address in the task's memory space
         * @param size The size of the structure, in bytes. Defaults to sizeof(T)
         * @return 0 on success, -1 on error
         */
        template <typename T>
        int copy_from_memory(T &dest, uint32_t src, size_t size = sizeof(T))
        {
            return memory->ms.memcpy(&dest, src, size);
        }

        /**
         * @brief Map a memory region
         * @param addr The virtual address to map. 0 to auto-allocate
         * @param size The size of the region to map
         * @param perms The permissions for the region, composed by bitwise-ORing `PERM_*` flags
         * @param flags The mmap flags, controlling the type of mapping
         * @param fd The VFS file descriptor to back the mapping. -1 for anonymous
         * @param offset The backing file offset. **SPECIFIED IN MULIPLES OF 4096**
         * @return The mapped address, or UINT32_MAX on error and set `error`
         */
        uint32_t mmap(uint32_t addr, uint32_t size, uint8_t perms, int flags, int fd = -1, uint32_t offset = 0);

        /**
         * @brief Get a VFS file descriptor from a user FD
         * @param task_fd the userspace fd slot
         * @return The VFS file descriptor, or -1 on error and set `error`
         */
        int get_vfs_fd(int task_fd);

        /**
         * @brief Set the program break
         * @param brk The new program break. 0 to query the current break
         * @return The new program break, or the old break on error and set `error`
         * @note mbrk(0) can be used to get the current break
         */
        uint32_t mbrk(uint32_t brk = 0);

        /**
         * @brief Set the alternate signal stack
         * @param new_stack The new alternate signal stack. nullptr to not set
         * @param old_stack The old alternate signal stack. nullptr to ignore
         * @return 0 on success, -1 on error
         */
        int sigaltstack(const sys_sigaltstack *new_stack, sys_sigaltstack *old_stack);

        /**
         * @brief Set the clear_child_tid address
         * @param addr The address to set. 0 to disable
         */
        void set_tid_address(uint32_t addr);

        /**
         * @brief Set the robust list head pointer
         * @param head The head of the linked list
         */
        void set_robust_list(uint32_t head);

        /**
         * @brief Set the process group ID
         * @param pgid The new process group ID. 0 to set it to the PID
         * @return 0 on success, -1 on error
         * @note If pgid != 0, then it must specify an existing process group in the same session
         */
        int set_pgid(uint32_t pgid);

        /**
         * @brief Save the current CPU state of the task
         * @return The saved ucontext
         */
        sys_ucontext save_state();

        /**
         * @brief Load a saved CPU state
         * @param state The CPU state to load
         * @return 0 on success, -1 on error
         */
        void load_state(const sys_ucontext &state);

        /**
         * @brief Register a signal handler
         * @param signo The signal number
         * @param handler The handler
         * @return 0 on success, -1 on error
         */
        int sigaction(uint8_t signo, sys_sigaction handler);

        /**
         * @brief Get a signal action for a handler
         * @param signo The signal number
         * @return The action for the handler
         */
        sys_sigaction get_sigaction(uint8_t signo);

        /**
         * @brief Restore pre-signal state
         * @note This restores all registers and CPU state to how it was before
         */
        void sigreturn();

        /**
         * @brief Check for a state changes in child processes
         * @param idtype The type of id to wait for
         * @param id The id to wait for
         * @param siginfo The signal info structure to fill
         * @param options The wait options
         * @return 0 on success, -1 on error
         * @note If there are no matching state changes, this returns -1 and sets `error` to `EAGAIN`
         */
        int waitid(int idtype, uint32_t id, sys_siginfo *siginfo, int options);

        /**
         * @brief Check access to a file
         * @param dfd The VFS directory file descriptor
         * @param file The file path, relative to `dfd`
         * @param mode The access mode to check, composed by bitwise-ORing `PERM_*` flags
         * @param flags Flags controlling the access check
         * @return 0 on success, -1 on error
         */
        int accessat(int dfd, const char *file, int mode, int flags);

        /**
         * @brief Get the currently running task. **SEE WARNINGS**
         * @return A weak pointer to any currently running task, or nullptr if there is none
         * @warning Please refrain from using this as much as possible, and use proper dependency
         *        * passing. This is for any functions where it would be unfeasible to implement
         *        * this.
         */
        static Task *get_current_task();

        // MemorySpace functions

        int memcpy(void *dest, uint32_t src, uint32_t len);
        int memcpy(uint32_t dest, const void *src, uint32_t len);
        int memset(uint32_t addr, uint8_t value, uint32_t len);
        const void *mem_make_iterator_read(uint32_t addr);
        void *mem_make_iterator(uint32_t addr);
        template <typename T>
        T *mem_read_until_zero(uint32_t addr)
        {
            return memory->ms.read_until_zero<T>(addr);
        }
        char *mem_get_string(uint32_t addr);
        int mem_is_mapped(uint32_t loc, uint32_t size) const;
        int munmap(uint32_t addr, uint32_t size);
        int munmap_all();
        int mprotect(uint32_t addr, uint32_t size, uint8_t perms);
        int8_t mem_get_permissions(uint32_t loc, uint32_t size = 1);
        int futex_wait(uint32_t addr, void (*callback)());
        int futex_wake(uint32_t addr, uint32_t count);
        int futex_requeue(uint32_t wake_addr, uint32_t wake_count, uint32_t requeue_addr, uint32_t requeue_count);

        // TaskFDTable functions

        BaseTaskFD *get_fd(int fd) const;
        int get_fd_flags(int fd) const;
        int set_fd_flags(int fd, int flags);
        int close_fd(int fd);
        int set_fd(BaseTaskFD *task_fd, int fd = -1);
        int dup_fd(int fd, int new_fd);
        int allocate_fd(int start = 0);
        void close_cloexec_fds();
        void clear_fds();

        // TaskSignalQueue functions

        int send_signal(const sys_siginfo &siginfo); // to this task
        int send_signal_process(const sys_siginfo &siginfo); // to the process
        int send_signal_pgroup(const sys_siginfo &siginfo); // to whole process group
        size_t pending_signals_size() const;
        size_t pending_signals_size_process() const;

        // TaskSignalMask functions

        void block_signal(uint8_t signo);
        void unblock_signal(uint8_t signo);
        void set_signal_mask(const sys_sigset &sigset);
        void set_signal_blocked(uint8_t signo, bool blocked);
        int is_signal_blocked(uint8_t signo) const;
        uint64_t get_signal_mask(bool invert = false) const;
        sys_sigset get_signal_sigset() const;

        // TaskSignalHandlers functions

        int is_signal_handler(uint8_t signo);
        int is_signal_ignored(uint8_t signo);
        int is_signal_default(uint8_t signo);

        // TaskFSInfo functions

        int chroot(const char *path);
        int chdir(const char *path);
        char *getcwd(); // relative to current root
        char *get_abs_cwd(); // absolute, from VFS /
        int get_umask() const;
        void set_umask(int new_umask);
        int mask_mode(int mode) const;

        // Process functions

        bool is_leader() const;
        uint32_t get_pid() const;
        uint32_t get_ppid() const;
        uint32_t get_pgid() const;
        uint32_t get_sid() const;
        void get_uid(int *uid = nullptr, int *euid = nullptr, int *suid = nullptr) const;
        void get_gid(int *gid = nullptr, int *egid = nullptr, int *sgid = nullptr) const;
        const Vector<int> &get_groups() const;
        void set_uid(int uid = -1, int euid = -1, int suid = -1);
        void set_gid(int gid = -1, int egid = -1, int sgid = -1);
        int set_groups(const Vector<int> &groups);
        const Set<Task *> &get_process_tasks() const;
        const Set<Process *> &get_children_processes() const;
        int setsid();

        RiscVEmulator &get_emulator() { return emulator; }
        uint32_t get_tid() const { return tid; }
        Task *get_parent() const { return parent; }
        bool is_blocking() const { return blocking_operation != nullptr; }
        bool is_pause() const { return is_paused; }

    private:
        // Private zero-initialize, with shared pointers = nullptr
        Task() = default;

        void run() override;
        void add_to_scheduler();

        static inline UnorderedMap<uint32_t, Task *> tasks; // weak pointers
        static inline uint32_t next_tid = 1;
        static inline Task *current_task = nullptr;

        SharedPtr<Process> process;
        SharedPtr<Memory> memory;
        SharedPtr<TaskFDTable> fd_table;
        TaskSignalQueue pending_signals;
        TaskSignalMask signal_mask;
        RiscVEmulator emulator;
        BlockingCallback blocking_operation = nullptr;
        int (*blocking_operation_alt)(Task &, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t); // Used by other overload of `block`
        uint64_t blocking_operation_saved = 0; // Optionally used by blocking operations
        BlockingCallback interrupt_blocking = nullptr; // Called when signal recieved while blocking
        uint64_t last_instruction_tick = 0;
        uint32_t tid;
        Task *parent = nullptr; // may be null
        sys_sigaltstack alt_signal_stack = {};
        sys_ucontext signal_saved_state = {};
        uint32_t clear_child_tid = 0;
        uint32_t robust_list = 0;
        bool is_paused : 1 = false;
        bool is_vfork : 1 = false; // clears blocking operation of parent on memory space release
        bool is_handling_signal : 1 = false;
    };

    /**
     * @brief Create a new task
     * @param path The path to the executable
     * @param argv The argument for the task
     * @param envp The environment variables
     */
    Task *spawn(const char *path, const char *const *argv = nullptr, const char *const *envp = nullptr);
} // namespace Hamster
