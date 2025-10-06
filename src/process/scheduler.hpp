// Hamster scheduler

#pragma once

#include <process/task.hpp>
#include <memory/stl_map.hpp>

namespace Hamster
{
    class Scheduler
    {
    public:
        /**
         * @brief Add a task to the scheduler.
         * @param task The task to add.
         * @return The task ID on success, 0 on failure, and set `error`
         */
        uint32_t add_task(Task *task);

        /**
         * @brief Get a task by its ID.
         * @param tid The task ID.
         * @return Pointer to the task on success, nullptr on failure, and set `error`
         * @note The returned pointer does not own the task
         */
        Task *get_task(uint32_t tid);

        /**
         * @brief Tick all runnable tasks
         * This is called every system tick to allow tasks to run.
         * @return -1 on error, 0 otherwise
         */
        int tick();

        /**
         * @brief Make a new process from an executable
         * @param path The path to the executable
         * @param argv The arguments to pass to the new process, nullptr for empty args
         * @param envp The environment variables to pass to the new process, nullptr for empty env
         * @param dirfd The directory file descriptor to open the executable in, or -1 for root
         * This sets the uid, gid, and ppid to 0 on the new process
         * @return 0 on success, -1 on error
         */
        int spawn(const char *path, const char *const *argv = nullptr, const char *const *envp = nullptr,
                  int dirfd = -1);

        /**
         * @brief For all processes that have PPID = `pid`, set their ppid to 1 (init)
         * @param pid The PID match
         * @note This is used to make init adopt child processes
         */
        int adopt_children(uint32_t pid);

        /**
         * @brief Get a process by PID
         * @param pid The PID of the process
         * @return A non-owning pointer to the process, or nullptr on error
         * @note This will first check TID `pid` for quick access, but if not found, then
         *     * it will check all threads. If still not found, return nullptr
         */
        Process *get_process(uint32_t pid);

        /**
         * @brief Get a process group by PGID
         * @param pgid The PGID of the process group
         * @return A non-owning pointer to the process group, or nullptr on error
         * @note This will first check TID `pgid` for quick access, but
         *     * if not found, then it will linearly check all tasks.
         *     * If still not found, return nullptr
         */
        ProcessGroup *get_process_group(uint32_t pgid);

        /**
         * @brief Get a session by SID
         * @param sid The SID of the session
         * @return A non-owning pointer to the session, or nullptr on error
         * @note This will first check TID `sid` for quick access, but
         *     * if not found, then it will linearly check all tasks.
         *     * If still not found, return nullptr
         */
        Session *get_session(uint32_t sid);

        /**
         * @brief Get the currently running task
         * @return A non-owning pointer to the currently running task, or nullptr if no task is running
         * @note This will return the currently running task
         * @note This will only work from a system call or anything called from a system call
         */
        Task *get_current_task()
        { return current_task; }

        size_t num_tasks() const
        { return tasks.size(); }

        Map<uint32_t, Task *> &get_tasks()
        { return tasks; }

    private:
        
        int do_tick(Task &);

        // TID to task
        Map<uint32_t, Task *> tasks;

        uint32_t next_tid = 1;

        Task *current_task = nullptr;
    };

    extern Scheduler scheduler;
} // namespace Hamster

