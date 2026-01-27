// Globals

#include <memory/page_manager.hpp>
#include <memory/allocator.hpp>
#include <filesystem/vfs.hpp>
#include <kscheduler/kscheduler.hpp>
#include <network/network_manager.hpp>

#ifndef NDEBUG
#include <unordered_set>
#endif // NDEBUG

namespace Hamster
{
    // Order matters!
    // This file defines the sequence of construction and destruction of global objects
    // Higher priority objects should be constructed first, and therefore should appear first in this file

    /* These don't have any dependencies, put first */

    __attribute__((weak))
    void _init_allocator() {}

    /**
     * Allocator
     * requires: none
     * provides: allocator
     */
    static bool _dummy = []() {
        _init_allocator();
        return true;
    }();

    /**
     * Error
     * requires: none
     * provides: error
     */
    int error{0};

    /**
     * CLOCK_REALTIME
     * requires: none
     * provides: clock_rt_offset
     */
    int64_t clock_rt_offset{0};

    /**
     * Emulator monitor
     * requires: none
     * provides: total_instructions_executed
     */
    uint64_t total_instructions_executed{0};

    /**
     * Kernel Scheduler
     * requires: allocator, error
     * provides: kscheduler
     */
    KScheduler kscheduler;

    /**
     * FD Reference Count
     * requires: none
     * provides: fd_refcount
     */
    UnorderedMap<int, unsigned int> fd_refcount;

    /* These ones depend on each other, in this order */

    /**
     * Page Manager
     * requires: allocator, error
     * provides: page_manager
     */
    PageManager page_manager;

    /**
     * Ram Filesystem
     * requires: page_manager, allocator, error
     * provides: ramfs
     */

    /**
     * Virtual Filesystem
     * requires: *fs, allocator, error
     * provides: vfs
     */
    VFS vfs;

    /**
     * Process
     * requires: page_manager, vfs, allocator, error, fd_refcount
     * provides: process
     */

     /**
      * Network Manager
      */
     NetworkManager network_manager;
} // namespace Hamster

