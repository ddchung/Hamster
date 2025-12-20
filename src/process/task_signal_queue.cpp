// Hamster task signal queue

#include <process/task_signal_queue.hpp>
#include <abi/values.hpp>
#include <errno/errno.h>
#include <cassert>

namespace Hamster
{
    int TaskSignalQueue::push(const sys_siginfo &siginfo)
    {
        if (siginfo.signo < 1 || siginfo.signo > 64)
        {
            error = H_EINVAL;
            return -1;
        }

        if (siginfo.signo >= H_SIGRTMIN)
        {
            // realtime signal
            signal_queue.push_back(siginfo);
        }
        else
        {
            // normal signal
            signal_map[siginfo.signo] = siginfo;
        }

        return 0;
    }

    const sys_siginfo *TaskSignalQueue::peek(const TaskSignalMask &mask) const
    {
        // Check in order from lowest to highest signal number

        for (const auto &[signo, siginfo] : signal_map)
        {
            assert(signo >= 1 && signo <= 64);
            assert(siginfo.signo == signo);
            if (mask.check(signo) == 0)
            {
                // Not blocked
                return &siginfo;
            }
        }

        // Not found in map, try queue

        for (const sys_siginfo &siginfo : signal_queue)
        {
            assert(siginfo.signo >= 1 && siginfo.signo <= 64);
            if (mask.check(siginfo.signo) == 0)
            {
                return &siginfo;
            }
        }

        return nullptr;
    }

    void TaskSignalQueue::pop(const TaskSignalMask &mask)
    {
        // Remove the lowest numbered signal that also isn't blocked

        for (auto map_it = signal_map.begin(); map_it != signal_map.end(); ++map_it)
        {
            if (mask.check(map_it->first) == 0)
            {
                // Not blocked
                signal_map.erase(map_it);
                return;
            }
        }

        for (auto queue_it = signal_queue.begin(); queue_it != signal_queue.end(); ++queue_it)
        {
            if (mask.check(queue_it->signo) == 0)
            {
                signal_queue.erase(queue_it);
                return;
            }
        }
    }

    size_t TaskSignalQueue::size() const
    {
        return signal_map.size() + signal_queue.size();
    }
} // namespace Hamster

