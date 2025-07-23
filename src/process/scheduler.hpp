// Hamster scheduler

#pragma once

#include <process/task.hpp>

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
         * This sets the uid, gid, and ppid to 0 on the new process
         * @return 0 on success, -1 on error
         */
        int spawn(const char *path, const char *const *argv = nullptr, const char *const *envp = nullptr);
    };

    extern Scheduler scheduler;
} // namespace Hamster

