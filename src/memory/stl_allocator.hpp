// C++ allocator for STL containers using Hamster malloc/free

#pragma once

#include <platform/platform.hpp>

namespace Hamster
{
    template <typename T>
    class STLAllocator
    {
    public:
        using value_type = T;

        STLAllocator() = default;

        template <typename U>
        STLAllocator(const STLAllocator<U>&) {}

        T* allocate(std::size_t n)
        {
            return static_cast<T*>(_malloc(n * sizeof(T)));
        }

        void deallocate(T* p, std::size_t)
        {
            _free(p);
        }
    };

    template <typename T, typename U>
    bool operator==(const STLAllocator<T>&, const STLAllocator<U>&)
    {
        return true;
    }

    template <typename T, typename U>
    bool operator!=(const STLAllocator<T>&, const STLAllocator<U>&)
    {
        return false;
    }
} // namespace Hamster

