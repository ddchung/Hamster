
#pragma once

#include <platform/platform.hpp>
#include <utility>
#include <cassert>
#include <cstdint>
#include <new>


namespace Hamster 
{
    template <typename T, typename... Args>
    T* alloc(unsigned int N = 1, Args&&... args)
    {
        constexpr unsigned int additional = (sizeof(unsigned int) / sizeof(T)) == 0 ? 1 : (sizeof(unsigned int) / sizeof(T));

        void *ptr = _malloc(sizeof(T) * (N + additional));
        assert(ptr != nullptr);
        assert(reinterpret_cast<uintptr_t>(ptr) % alignof(T) == 0);
        assert(reinterpret_cast<uintptr_t>(ptr) % alignof(unsigned int) == 0);

        *reinterpret_cast<unsigned int*>(ptr) = N;
        ptr = (T*)ptr + additional;

        for (unsigned int i = 0; i < N; ++i) {
            new (static_cast<T*>(ptr) + i) T(std::forward<Args>(args)...);
        }
        return static_cast<T*>(ptr);
    }

    template <typename T>
    void dealloc(T* ptr) {
        if (ptr == nullptr)
            return;

        constexpr unsigned int additional = (sizeof(unsigned int) / sizeof(T)) == 0 ? 1 : (sizeof(unsigned int) / sizeof(T));

        ptr -= additional;

        unsigned int N = *(unsigned int *)(ptr);

        for (unsigned int i = 0; i < N; ++i) {
            ((T*)ptr)[i].~T();
        }
        _free((void*)ptr);
    }
} // namespace Hamster