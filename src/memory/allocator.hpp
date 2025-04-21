
#pragma once

#include <platform/platform.hpp>
#include <utility>
#include <cassert>
#include <cstdint>
#include <new>
#include <type_traits>

#ifndef NDEBUG

#include <unordered_set>
#include <cstdio>
#include <cstdlib>

#if __cplusplus >= 202002L
# include <source_location>
#endif // __cplusplus >= 202002L

#endif // NDEBUG

namespace Hamster
{
#ifndef NDEBUG
    namespace
    {
        std::unordered_set<void *> allocated_pointers;
    } // namespace
#endif // NDEBUG
    

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
    
#ifndef NDEBUG
        assert(allocated_pointers.find(user_ptr) == allocated_pointers.end());
        allocated_pointers.insert(user_ptr);
#endif // NDEBUG

        for (unsigned int i = 0; i < N; ++i)
        {
            new (user_ptr + i) T(std::forward<Args>(args)...);
        }
        return user_ptr;
    }

    template <typename T>
    void dealloc(T *ptr
#if !defined(NDEBUG) && __cplusplus >= 202002L
        , std::source_location loc = std::source_location::current()
#endif // !defined(NDEBUG) && __cplusplus >= 202002L
        )
    {
        if (!ptr)
            return;

#ifndef NDEBUG
        if (allocated_pointers.find(ptr) == allocated_pointers.end())
        {
# if __cplusplus >= 202002L
            printf("Error: double free detected at %s:%d\n", loc.file_name(), (int)loc.line());
# else // __cplusplus >= 202002L
            printf("Error: double free detected\n");
# endif // __cplusplus >= 202002L
            printf("Pointer: %p\n", (void*)ptr);
            abort();
            return;
        }
        allocated_pointers.erase(ptr);
#endif // NDEBUG

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