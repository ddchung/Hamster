
#pragma once

#include <cstdint>
#include <cstddef>

namespace Hamster
{
    // Note: This is NOT the ABI io vector struct, see `struct sys_iovec` in `abi/structs.hpp`
    struct IOVec
    {
        void *data;
        size_t size;
    };

    // Counts the total number of bytes in iov
    size_t ioveclen(const IOVec *iov, size_t iovcnt);
} // namespace Hamster

