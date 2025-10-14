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
            EXIT,
            STOP,
            CONT,
        };

        using enum Type;

        Type type;
        uint32_t pid;

        union
        {
            // EXIT
            uint32_t exit_code;

            // STOP, CONT, TERM
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
        ~Process();

        Process(const Process &) = delete;
        Process &operator=(const Process &) = delete;
        Process(Process &&) = delete;
        Process &operator=(Process &&) = delete;

        uint32_t get_pid() const { return pid; }

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
         * @param argv The arguments
         * @param envp The environment variables
         * @return 0 on success, -1 on error
         * @note This will clear all tasks except one, and wipe the memory space
         */
        int exec(int fd, const char *const *argv, const char *const *envp);

        /**
         * @brief Make the process exit
         * @param code The exit code to exit with
         * @return 0 on success, -1 on error
         */
        int exit(uint16_t code);

        const SharedPtr<ProcessGroup> &get_process_group() const { return pgroup; }
        const SharedPtr<TaskSignalHandlers> &get_signal_handlers() const { return signal_handlers; }
        const SharedPtr<TaskFSInfo> &get_fs_info() const { return fs_info; }
        size_t num_tasks() const { return tasks.size(); }
        Task *get_leader() const { return leader; }

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
        ~Task();

        // Give access to default constructor
        friend Task *alloc<Task>(size_t N);

        /**
         * @brief Make a new task, from this one
         * @param flags Flags controlling which parts to share and which ones to copy
         * @return The new task, or nullptr on error and set `error`
         * @note See documentation on the `clone` Linux system call for more info
         */
        Task *clone(int flags);

        /**
         * @brief Make an entirely new task
         * @param fd The file descriptor of the executable file to load
         * @param argv The arguments for the task
         * @param envp The environment variables for the task
         * @return The new task, or nullptr on error and set `error`
         * @note This creates a completely new task, in its own process, process group, and session
         */
        static Task *create_task(int fd, const char *const *argv = {nullptr}, const char *const *envp = {nullptr});

        /**
         * @brief Get a task by TID
         * @param tid The TID of the task to get
         * @return A weak pointer to the task, or nullptr on error and set `error`
         */
        static Task *get_task(uint32_t tid);

        /**
         * @brief Make this task exit
         * @return 0 on success, -1 on error
         */
        int exit();

        /**
         * @brief Open a relative directory file descriptor
         * @param thread_dfd The userspace relative file descriptor
         * @param path The path
         * @return A VFS file descriptor `fd` such that doing a relative operation `*at(fd, path)` will
         *         result in the intended target. -1 on error and set `error`
         */
        int open_rel_fd(int thread_dfd, const char *path);

        /**
         * @brief Enter a blocking operation
         * @param callback The blocking callback, called every once in a while
         * @param interrupt_callback The callback to call to interrupt the blocking operation partway through
         * @return 0 on success, -1 on error
         */
        int block(void (*callback)(Task &), void (*interrupt_callback)(Task &));

        /**
         * @brief Interrupt the blocking operation
         * @return 0 on success, -1 on error
         */
        int interrupt_block();

        const SharedPtr<Process> &get_process() const { return process; }
        MemorySpace &get_memory() const { return memory->ms; }
        uint32_t get_brk() const { return memory->brk; }
        void set_brk(uint32_t brk) { memory->brk = brk; }
        const SharedPtr<TaskFDTable> &get_fd_table() const { return fd_table; }
        RiscVEmulator &get_emulator() { return emulator; }
        uint32_t get_tid() const { return tid; }

    private:
        // Private zero-initialize, with shared pointers = nullptr
        Task() = default;

        void run() override;
        void add_to_scheduler();

        static inline UnorderedMap<uint32_t, Task *> tasks; // weak pointers
        static inline uint32_t next_tid = 1;

        SharedPtr<Process> process;
        SharedPtr<Memory> memory;
        SharedPtr<TaskFDTable> fd_table;
        TaskSignalQueue pending_signals;
        TaskSignalMask signal_mask;
        RiscVEmulator emulator;
        void (*blocking_operation)(Task &) = nullptr;
        uint32_t blocking_operation_saved[3] = {}; // Optionally used by blocking operations
        void (*interrupt_blocking)(Task &) = nullptr; // Called when signal recieved while blocking
        uint32_t tid;
        Task *parent = nullptr;
        uint32_t sig_alt_stack = 0;
        sys_ucontext signal_saved_state;
        uint32_t clear_child_tid = 0;
        uint32_t robust_list = 0;
        uint32_t robust_list_size;
        bool is_paused : 1 = false;
        bool is_vfork : 1 = false; // clears blocking operation of parent on memory space release
    };

    /**
     * @brief Create a new task
     * @param path The path to the executable
     * @param argv The argument for the task
     * @param envp The environment variables
     */
    Task *spawn(const char *path, const char *const *argv = {nullptr}, const char *const *envp = {nullptr});
} // namespace Hamster
