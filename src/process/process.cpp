// implementation of process.hpp


#include <process/process.hpp>
#include <process/thread.hpp>
#include <memory/allocator.hpp>

namespace Hamster
{
    Thread::RunCode Process::tick_all()
    {
        memory_space.swap_in_pages();
        for (auto thread : threads)
        {
            if (thread.running)
            {
                thread.thread->tick();
            }
        }
        memory_space.swap_out_pages();
    }

    Thread::RunCode Process::tick_all64()
    {
        memory_space.swap_in_pages();
        for (int i = 0; i < 64; i++)
        {
            for (auto thread : threads)
            {
                if (thread.running)
                {
                    thread.thread->tick();
                }
            }
        }
        memory_space.swap_out_pages();
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

