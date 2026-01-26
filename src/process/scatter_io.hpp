// Hamster scatter IO

#pragma once

#include <memory/page_manager.hpp> // for PERM_*
#include <abi/structs.hpp>
#include <cstdint>
#include <cstddef>
#include <utility>
#include <sys/types.h>

namespace Hamster
{
    // Note: This is NOT the ABI io vector struct, see `struct sys_iovec` in `abi/structs.hpp`
    struct IOVec
    {
        void *data;
        size_t size;
    };

    /**
     * @brief Make a kernel IO vector from a userspace one
     * @param task The task
     * @param iovec_loc The location of the io vector in the task's memory
     * @param iovec_count The number of IO vectors
     * @param perms Required memory permissions, a bitmask of (PERM_*)
     * @return A newly allocated array of kernel IO vectors, in the right order. Be sure to deallocate. nullptr on error
     */
    std::pair<IOVec *, size_t> make_iovec(class Task &task, uint32_t iovec_loc, uint32_t iovec_count, uint8_t perms = PERM_READ);

    /**
     * @brief Make a kernel IO vector from a userspace one
     * @param task The task
     * @param iovec The io vectors
     * @param iovec_count The number of IO vectors
     * @param perms A bitmask of required memory permissions
     * @return A newly allocated array of kernel IO vectors, in the right order. Be sure to deallocate. nullptr on error
     */
    std::pair<IOVec *, size_t> make_iovec(class Task &task, const sys_iovec *iovec, size_t iovec_count, uint8_t perms = PERM_READ);

    /**
     * @brief Make a kernel IO vector from a single userspace buffer
     * @param task The task
     * @param buf_loc The location of the buffer
     * @param buf_size The size of the  buffer
     * @param perms A bitmask of required memory permissions
     * @return A newly allocated array of kernel IOVecs, or nullptr on error and set `error`
     */
    std::pair<IOVec *, size_t> make_iovec_buf(class Task &task, uint32_t buf_loc, uint32_t buf_size, uint8_t perms = PERM_READ);
} // namespace Hamster

