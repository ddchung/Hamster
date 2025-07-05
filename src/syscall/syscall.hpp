// Hamster system calls

#pragma once

#include <process/thread.hpp>
#include <cstdint>

namespace Hamster
{
    inline int do_syscall(Thread &thread) {return 0;}
} // namespace Hamster

