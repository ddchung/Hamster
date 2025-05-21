// Hamster process

#include <memory/memory_space.hpp>

#pragma once

namespace Hamster
{
    class BaseProcess
    {
    public:
        virtual ~BaseProcess() = default;

    protected:
        MemorySpace mem_sp;
    };
} // namespace Hamster

