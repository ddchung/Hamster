// A Process

#pragma once

#include <memory/memory_space.hpp>
#include <memory/stl_sequential.hpp>
#include <process/thread.hpp>

#include <memory/allocator.hpp>
#include <utility>

namespace Hamster
{
    class Process
    {
        struct ThreadInfo
        {
            Thread *thread;

            // not used yet
            bool running : 1;
        };
    public:
        // Make a thread
        // ThreadClass: a class derived from Thread
        // ThreadClass must have a constructor
        // accepting a MemorySpace reference as the first argument
        template <class ThreadClass, typename... Args>
        int create_thread(Args&&...);

        Thread::RunCode tick_all();
        Thread::RunCode tick_all64();

        ~Process();

    private:
        MemorySpace memory_space;

        Vector<ThreadInfo> threads;
    };
} // namespace Hamster

template <class ThreadClass, typename... Args>
int Hamster::Process::create_thread(Args&&... args)
{
    Thread *thread = alloc<Thread>(memory_space, std::forward<Args>(args)...);
    ThreadInfo info;
    info.thread = thread;
    info.running = true;
    threads.push_back(info);
    return 0;
}
