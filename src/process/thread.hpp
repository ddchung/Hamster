// simple thread
// a part of a process

#pragma once

#include <memory/memory_space.hpp>

namespace Hamster
{
    class Thread
    {
    public:
        // Note: All derived classes *must* have a constructor
        // accepting a MemorySpace reference as the first argument
        inline Thread(MemorySpace &) : memory_space(memory_space) {}

        virtual ~Thread() = default;

        Thread(const Thread &) = delete;
        Thread &operator=(const Thread &) = delete;

        struct RunCode
        {
            int exit_code;
            bool terminate : 1;
        };

        virtual RunCode tick() = 0;

    protected:
        MemorySpace &memory_space;
    };
} // namespace Hamster

