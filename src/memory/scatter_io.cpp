
#include <memory/scatter_io.hpp>

namespace Hamster
{
    size_t ioveclen(const IOVec *iov, size_t iovcnt)
    {
        size_t size = 0;
        for (size_t i = 0; i < iovcnt; ++i)
            size += iov[i].size;
        return size;
    }
} // namespace Hamster
