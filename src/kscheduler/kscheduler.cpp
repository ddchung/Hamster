// Hamster kernel scheduler implementation

#include <kscheduler/kscheduler.hpp>
#include <platform/platform.hpp>
#include <platform/config.hpp>
#include <memory/allocator.hpp>
#include <errno/errno.h>
#include <cassert>

#include <libcthread/cthread.h>

extern "C"
{
    // Linked into libcthread
    size_t CTHREAD_MAX_THREADS = 128;

    void *cthread_malloc(size_t size)
    {
        return Hamster::_malloc(size);
    }

    void cthread_free(void *ptr)
    {
        int res = Hamster::_free(ptr);
        (void)res;
        assert(res == 0);
    }
}

namespace Hamster
{
    void BaseKTask::yield()
    {
        cthread_yield();
    }


    KScheduler::~KScheduler()
    {
        // Clean up
        for (auto &[id, task] : tasks)
            dealloc(task);
        tasks.clear();
    }

    int KScheduler::add_task(BaseKTask *task)
    {
        if (!task || task->id == 0)
        {
            error = H_EINVAL;
            return -1;
        }

        auto it = tasks.find(task->id);
        if (it != tasks.end())
        {
            error = H_EEXIST;
            return -1;
        }

        if (tasks.size() >= CTHREAD_MAX_THREADS)
        {
            error = H_EAGAIN;
            return -1;
        }

        tasks[task->id] = task;

        cthread_create(nullptr, [](void *p_task) {
            BaseKTask *task = (BaseKTask *)p_task;
            
            while (true)
            {
                cthread_yield();
                uint64_t now = _get_sys_time();
                if ((task->flags & KSCHED_REMOVE_NOW) == 0 && task->next_tick <= now)
                {
                    task->run();
                    ++task->tick_count;
                    task->last_tick = now;
                    if (task->flags & KSCHED_AUTO_INTERVAL)
                        task->next_tick = now + task->interval;
                }
                if (task->flags & KSCHED_REMOVE_NOW)
                {
                    // Remove from map and clean up
                    auto it = kscheduler.tasks.find(task->id);
                    if (it != kscheduler.tasks.end())
                        kscheduler.tasks.erase(it);
                    dealloc(task);
                    cthread_exit();
                }
            }
        }, task);

        return 0;
    }

    int KScheduler::remove_task(uint32_t id)
    {
        auto it = tasks.find(id);
        if (it == tasks.end())
        {
            error = H_ESRCH;
            return -1;
        }
        it->second->flags |= KSCHED_REMOVE_NOW;
        return 0;
    }

    void KScheduler::remove_all()
    {
        for (auto [id, task] : tasks)
            task->flags |= KSCHED_REMOVE_NOW;
    }

    int KScheduler::tick()
    {
        cthread_yield();
        return 0;
    }
} // namespace Hamster

