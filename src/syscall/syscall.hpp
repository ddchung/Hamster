// Hamster system calls

#pragma once

#include <syscall/syscall_id.hpp>
#include <process/thread.hpp>
#include <cstdint>

namespace Hamster
{
    int do_syscall(Thread &thread);
} // namespace Hamster

