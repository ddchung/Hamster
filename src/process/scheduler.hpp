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
         * @param file The executable
         * @param argv The arguments to pass to the new process, nullptr for empty args
         * @param envp The environment variables to pass to the new process, nullptr for empty env
         * This sets the uid, gid, and ppid to 0 on the new process
         * @return 0 on success, -1 on error
         */
        int spawn(File file, const char *const *argv = nullptr, const char *const *envp = nullptr);

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

    private:
        
        int do_tick(Task &);

        // TID to task
        Map<uint32_t, Task *> tasks;

        uint32_t next_tid = 1;
    };

    extern Scheduler scheduler;
} // namespace Hamster

