
#pragma once

#include <platform/platform.hpp>
#include <utility>
#include <cassert>
#include <cstdint>
#include <new>
#include <type_traits>

#include <cstdio>

namespace Hamster
{
    template <typename T, typename... Args>
    T *alloc(unsigned int N = 1, Args &&...args)
    {
        constexpr unsigned int additional = (sizeof(unsigned int) + sizeof(T) - 1) / sizeof(T);
        void *raw_ptr = _malloc(sizeof(T) * (N + additional));
        assert(raw_ptr != nullptr);

        uintptr_t raw_addr = reinterpret_cast<uintptr_t>(raw_ptr);
        assert(raw_addr % alignof(T) == 0);
        assert(raw_addr % alignof(unsigned int) == 0);

        auto *meta_ptr = reinterpret_cast<unsigned int *>(raw_ptr);
        *meta_ptr = N;

        T *user_ptr = reinterpret_cast<T *>(meta_ptr) + additional; // safely skip metadata
        for (unsigned int i = 0; i < N; ++i)
        {
            new (user_ptr + i) T(std::forward<Args>(args)...);
        }
        return user_ptr;
    }

    template <typename T>
    void dealloc(T *ptr)
    {
        if (!ptr)
            return;

        constexpr unsigned int additional = (sizeof(unsigned int) + sizeof(T) - 1) / sizeof(T);
        unsigned int *meta_ptr = (unsigned int *)(ptr - additional);
        unsigned int N = *meta_ptr;

        for (unsigned int i = 0; i < N; ++i)
        {
            ptr[i].~T();
        }

        _free(meta_ptr);
    }
} // namespace Hamster