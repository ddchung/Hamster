// Hamster kernel scheduler
// This schedules kernel tasks, not user processes.
// For the userspace scheduler, see `process/scheduler.hpp`

#pragma once

#include <memory/stl_map.hpp>
#include <cstdint>

namespace Hamster
{
    inline constexpr int KSCHED_REMOVE_NEXT_TICK = 1 << 0; // Remove this task after the next tick, like a one-shot
    inline constexpr int KSCHED_AUTO_INTERVAL = 1 << 1; // Automatically set the next tick to the current time + interval
    inline constexpr int KSCHED_REMOVE_NOW = 1 << 2; // Remove as soon as the scheduler sees it
    inline constexpr int KSCHED_REMOVE_ALL = 1 << 3; // Remove all tasks, used for shutdown

    class BaseKTask
    {
    public:
        virtual ~BaseKTask() = default;

        /**
         * @brief Run the task
         */
        virtual void run() = 0;

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
         * @brief Change an existing task's ID
         * @param old_id The current ID of the task
         * @param new_id The new ID to assign to the task
         * @return 0 on success, -1 on failure and set `error`
         */
        int move_task(uint32_t old_id, uint32_t new_id);

        /**
         * @brief Manually remove a task from the kernel scheduler
         * @param id The ID of the task to remove
         * @return 0 on success, -1 on failure and set `error`
         */
        int remove_task(uint32_t id);

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

