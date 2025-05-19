// Globals

#include <process/thread_type_manager.hpp>
#include <process/riscv_rv32i_thread.hpp>
#include <memory/page_manager.hpp>
#include <memory/allocator.hpp>
#include <filesystem/vfs.hpp>
#include <errno/errno.h>
#include <elf.h>
#include <utility>

namespace Hamster
{
    // Order matters!
    // This file defines the sequence of construction and destruction of global objects
    // Higher priority objects should be constructed first, and therefore should appear first in this file

#ifndef NDEBUG
    std::unordered_set<void *> allocated_pointers;
#endif // NDEBUG

    int error{0};
    PageManager page_manager;
    VFS vfs;
    ThreadTypeManager thread_type_manager;

    // Static initialization functions

    static auto x  = []()
    {
        thread_type_manager.register_thread_type(EM_RISCV, [](MemorySpace &mem_space) -> BaseThread * {
            return alloc<RV32Thread>(1, mem_space);
        });
        return 0;
    }();
} // namespace Hamster

