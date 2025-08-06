// Hamster kernel scheduler implementation

#include <kscheduler/kscheduler.hpp>
#include <platform/platform.hpp>
#include <platform/config.hpp>
#include <memory/allocator.hpp>
#include <errno/errno.h>
#include <cassert>

namespace Hamster
{
    KScheduler::~KScheduler()
    {
        // Clean up
        for (auto &[id, task] : tasks)
        {
            dealloc(task);
        }
        tasks.clear();
    }

    int KScheduler::add_task(BaseKTask *task)
    {
        if (!task || task->id == 0)
        {
            error = EINVAL;
            return -1;
        }

        auto it = tasks.find(task->id);
        if (it != tasks.end())
        {
            error = EEXIST;
            return -1;
        }

        tasks[task->id] = task;
        return 0;
    }

    int KScheduler::remove_task(uint32_t id)
    {
        auto it = tasks.find(id);
        if (it == tasks.end())
        {
            error = ESRCH;
            return -1;
        }

        dealloc(it->second);
        tasks.erase(it);
        return 0;
    }

    int KScheduler::tick()
    {
        // Get system time
        uint64_t now = _get_sys_time();

        // Tick loop
        for (auto &[id, task] : tasks)
        {
            if (task->flags & KSCHED_REMOVE_NOW)
                continue;
            if (task->next_tick <= now)
            {
                // Check if overdue
                if (task->next_tick + HAMSTER_KSCHED_OVERDUE_TIME < now)
                {
                    // Update interval if it is auto-interval, mark for removal otherwise
                    if (task->flags & KSCHED_AUTO_INTERVAL)
                        task->next_tick = now + task->interval;
                    else
                        task->flags |= KSCHED_REMOVE_NOW;
                }
                else
                {
                    // Tick the task
                    bool remove_next_tick_before = (task->flags & KSCHED_REMOVE_NEXT_TICK) != 0;
                    task->run();
                    task->tick_count++;
                    task->last_tick = now;
                    if (task->flags & KSCHED_AUTO_INTERVAL)
                        task->next_tick = now + task->interval;
                    bool remove_next_tick_after = (task->flags & KSCHED_REMOVE_NEXT_TICK) != 0;
                    if (remove_next_tick_before && remove_next_tick_after)
                        task->flags |= KSCHED_REMOVE_NOW; // Mark for removal
                }
            }
        }

        // Remove loop
        for (auto it = tasks.begin(); it != tasks.end();)
        {
            if (it->second->flags & KSCHED_REMOVE_NOW)
            {
                dealloc(it->second);
                it = tasks.erase(it);
            }
            else
            {
                ++it;
            }
        }

        return 0;
    }
} // namespace Hamster

