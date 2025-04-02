// implementation of process.hpp


#include <process/process.hpp>
#include <process/thread.hpp>
#include <memory/allocator.hpp>

namespace Hamster
{
    Thread::RunCode Process::tick_all()
    {
        memory_space.swap_in_pages();
        for (size_t i = 0; i < threads.size(); i++)
        {
            if (threads[i].running)
            {
                current_thread = &threads[i];
                auto runcode = threads[i].thread->tick();
                current_thread = nullptr;

                if (runcode.terminate)
                {
                    threads[i].running = false;
                    dealloc<Thread>(threads[i].thread);
                    threads.erase(threads.begin() + i);
                    i--;
                }
            }
        }
        memory_space.swap_out_pages();

        if (threads.empty())
        {
            return {0, true}; // terminate
        }
        return {0, false}; // continue
    }

    Thread::RunCode Process::tick_all64()
    {
        memory_space.swap_in_pages();
        for (int i = 0; i < 64; i++)
        {
            for (size_t j = 0; j < threads.size(); j++)
            {
                if (threads[j].running)
                {
                    current_thread = &threads[j];
                    auto runcode = threads[j].thread->tick();
                    current_thread = nullptr;

                    if (runcode.terminate)
                    {
                        threads[j].running = false;
                        dealloc<Thread>(threads[j].thread);
                        threads.erase(threads.begin() + j);
                        j--;
                    }
                }
            }
        }
        memory_space.swap_out_pages();
        if (threads.empty())
        {
            return {0, true}; // terminate
        }
        return {0, false}; // continue
    }

    Process::~Process()
    {
        for (auto thread : threads)
        {
            dealloc<Thread>(thread.thread);
        }
        threads.clear();
    }
} // namespace Hamster

