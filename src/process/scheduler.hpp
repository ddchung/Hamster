// Hamster scheduler

#pragma once

#include <process/base_thread.hpp>
#include <memory/stl_sequential.hpp>
#include <memory/stl_map.hpp>
#include <cstdint>

namespace Hamster
{
    class Scheduler
    {
    public:
        /**
         * @brief Add a new thread to the scheduler
         * @param thread The thread to add
         * @note This will take ownership of the thread
         * @return 0 on success, -1 on failure
         */
        int add_thread(BaseThread *thread);

        /**
         * @brief Add a new process to the scheduler
         * @param proc The process to add
         * @note This will take ownership of the process
         * @note Don't initialize the `pid` member of `proc`, as it will be set by the scheduler
         *     * but please do set `ppid`
         * @return 0 on success, -1 on failure
         */
        int add_process(ProcessData *proc);

        // Note that there are no remove functions, as threads will be removed when they stop,
        // and processes will be removed when they have no threads left

        /**
         * @brief Tick the scheduler once
         * @note This will tick all threads in the scheduler, if they aren't paused
         * @return The number of threads that were ticked, or -1 on failure
         */
        ssize_t tick();

        /**
         * @brief Get a process by ID
         * @param pid The ID of the process to get
         * @return A pointer to the process, or nullptr if it doesn't exist
         * @note This will return a weak pointer, so the process may be deleted at any time
         *     * Also because of this, you should not delete the process yourself
         */
        ProcessData *get_process(uint32_t pid);

    private:
        // Next PID
        uint32_t next_pid = 1;

        UnorderedMap<uint32_t, ProcessData *> processes;
        List<BaseThread *> threads;
    };

    extern Scheduler scheduler;
} // namespace Hamster

