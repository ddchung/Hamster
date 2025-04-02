// Simple process

#pragma once

#include <memory/memory_space.hpp>

namespace Hamster
{
    class ProcessHelper;

    class Process
    {
    public:
        Process(const Process&) = delete;
        Process& operator=(const Process&) = delete;

        ~Process() = default;

        inline int get_id() const { return id; }

        inline int tick()
        {
            if (!tick_impl)
                return -1;
            memory_space.swap_in_pages();
            int result = tick_impl(*this);
            memory_space.swap_out_pages();
            return result;
        }

        // tick 64 times
        // more efficient than calling tick 64 times
        inline int tick64()
        {
            int result = 0;

            if (!tick_impl)
                return -1;
            memory_space.swap_in_pages();
            for (int i = 0; i < 64; ++i)
            {
                result = tick_impl(*this);
                if (result != 0)
                    break;
            }
            memory_space.swap_out_pages();
            return result;
        }
        
        friend class ProcessHelper;

    private:
        Process(int id, int (*tick)(Process&))
            : id(id), tick_impl(tick) {}
        
        int (*tick_impl)(Process&);
        
        MemorySpace memory_space;
        int id;
    };

    class ProcessHelper
    {
    public:
        inline static MemorySpace& get_memory_space(Process& process)
        {
            return process.memory_space;
        }

        inline static Process make_process(int id, int (*tick)(Process&))
        {
            return Process(id, tick);
        }
    private:
        static int process_id_counter;
    };
} // namespace Hamster

