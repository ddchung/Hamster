// Hamster kernel scheduler
// This schedules kernel tasks, not user processes.
// For the userspace scheduler, see `process/scheduler.hpp`

#pragma once

#include <memory/stl_map.hpp>
#include <cstdint>

namespace Hamster
{
    inline constexpr int KSCHED_AUTO_INTERVAL = 1 << 0; // Automatically set the next tick to the current time + interval
    inline constexpr int KSCHED_REMOVE_NOW = 1 << 1; // Remove as soon as the scheduler sees it

    class BaseKTask
    {
    public:
        virtual ~BaseKTask() = default;

        /**
         * @brief Run the task
         * @note This may be implemented as either non-returning with  yield(), or called repeatedly by the scheduler
         */
        virtual void run() = 0;

        /**
         * @brief Yield control to other tasks
         * @warning May not return if removed, so make sure nothing leaks
         */
        static void yield();

        // KSCHED_*
        int flags = 0;

        // Unique ID of the task
        // Note: When adding to the scheduler, with `KScheduler::add_task`, this must 
        //       already be set to a unique value, and also must not be 0
        uint32_t id = 0;

        // These are in system ticks, see `_get_sys_time()`
        uint64_t last_tick = 0; // When the task was last ticked
        uint64_t next_tick = 0; // When the task should be ticked next

        uint64_t tick_count = 0; // How many times the task has been ticked

        // Only used if KSCHED_AUTO_INTERVAL is set
        uint64_t interval = 0;
    };

    class KScheduler
    {
    public:
        KScheduler() = default;
        ~KScheduler();
        KScheduler(const KScheduler &) = delete;
        KScheduler &operator=(const KScheduler &) = delete;
        KScheduler(KScheduler &&) = delete;
        KScheduler &operator=(KScheduler &&) = delete;

        /**
         * @brief Add a task to the kernel scheduler
         * @param task The task to add
         * @return 0 on success, -1 on failure and set `error`
         * @note This takes ownership of the task, and will `dealloc` it when it is removed
         * @note If the ID is already in use, this fails with `H_EEXIST`
         * @note The task's ID must be already set
         */
        int add_task(BaseKTask *task);

        /**
         * @brief Mark a task to be removed later
         * @param id The ID of the task to remove
         * @return 0 on success, -1 on failure and set `error`
         * @note This defers the removal until later. It is guaranteed the task will not be run after this.
         */
        int remove_task(uint32_t id);

        /**
         * @brief Mark all tasks for removal
         */
        void remove_all();

        /**
         * @brief Tick all tasks that are ready
         * This will run the `run()` method of each task that is ready to be ticked, and 
         * remove all tasks that are ready for removal.
         * @return 0 on success, -1 on failure and set `error`
         */
        int tick();

        /**
         * @brief Check if there are still tasks
         */
        bool has_tasks() const { return !tasks.empty(); }

    private:
        UnorderedMap<uint32_t, BaseKTask *> tasks;
    };

    extern KScheduler kscheduler;
} // namespace Hamster

