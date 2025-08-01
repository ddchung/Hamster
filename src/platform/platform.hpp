// Hamster platform-dependent stubs

#pragma once

#include <cstdint>
#include <cstddef>
#include <platform/config.hpp>

namespace Hamster
{
    // int-returning functions return 0 on success
    // or negative error code on failure
    // unless otherwise specified

    // Platform-dependent initialization
    // called at the start of the program
    int _init_platform();

    // Mount the root filesystem
    int _mount_rootfs();

    // Each page takes up HAMSTER_PAGE_SIZE bytes
    int _swap_out(int index, const uint8_t *data);

    // Load in a page
    // write to *data
    // please either write all or don't write at all
    int _swap_in(int index, uint8_t *data);

    // Remove a swap page
    int _swap_rm(int index);

    // Removes all swap pages
    int _swap_rm_all();

    // Allocate some memory on heap
    // returns nullptr on failure
    void *_malloc(size_t size);

    // Free some memory on heap
    // returns 0 on success
    // returns -1 on failure
    int _free(void *ptr);

    // Log
    int _log(const char * msg);
    int _log(char c);

    // Trace
    // This is a debug function, by default it does nothing
    // If overridden, it is **VERY IMPORTANT** that it traces to a different
    // place than _log, or else the output will be incomprehensible
    ///
    // It is a printf drop-in replacement
    // It is encouraged to use the C lib's *printf functions, instead of a hand-rolled one
    // because of potential usages of uncommon format specifiers (like "%.*s")
    __attribute__((format(printf, 1, 2)))
    void _trace(const char *fmt, ...);
}
