
#pragma once

#include <platform/platform.hpp>
#include <utility>
#include <cassert>
#include <cstdint>
#include <new>
#include <type_traits>

#ifndef NDEBUG

#include <unordered_set>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <typeinfo>

#if __cplusplus >= 202002L
#include <source_location>
#endif // __cplusplus >= 202002L

#endif // NDEBUG

namespace Hamster
{
#ifndef NDEBUG
    inline std::unordered_set<void *> allocated_pointers;
#endif // NDEBUG

    template <typename T, typename... Args>
    T *alloc(std::size_t N = 1, Args &&...args)
    {
        static_assert(std::is_object<T>::value, "T must be object type");
        const std::size_t alignment = alignof(T);
        const std::size_t data_size = sizeof(T) * N;
        // header holds (void* original_malloc_ptr) + (std::size_t count)
        const std::size_t header_size = sizeof(void *) + sizeof(std::size_t);
        // add alignment-1 so we can align the user-pointer
        const std::size_t total_size = header_size + data_size + (alignment - 1);

        // raw block from malloc
        void *raw = std::malloc(total_size);
        assert(raw != nullptr);

        // find an address after the header that satisfies alignment
        uintptr_t raw_addr = reinterpret_cast<uintptr_t>(raw);
        uintptr_t begin = raw_addr + header_size;
        uintptr_t aligned_addr = (begin + (alignment - 1)) & ~(alignment - 1);
        char *user_ptr = reinterpret_cast<char *>(aligned_addr);

        // store our header immediately before user_ptr
        //  [ raw pointer ] at user_ptr - header_size
        //  [ count       ] at user_ptr - sizeof(std::size_t)
        void **raw_slot = reinterpret_cast<void **>(user_ptr - header_size);
        std::size_t *count_slot = reinterpret_cast<std::size_t *>(user_ptr - sizeof(std::size_t));
        *raw_slot = raw;
        *count_slot = N;

        // placement-new the array
        T *result = reinterpret_cast<T *>(user_ptr);
        for (std::size_t i = 0; i < N; ++i)
        {
            new (result + i) T(std::forward<Args>(args)...);
        }

#ifndef NDEBUG
        // store the pointer in a set for debugging
        allocated_pointers.insert(result);
#endif // NDEBUG

        return result;
    }

    // dealloc<T>(p) — destroy the N objects and free only the original malloc() pointer
    template <typename T>
    void dealloc(T *cv_p
#if __cplusplus >= 202002L && !defined(NDEBUG)
                 , std::source_location loc = std::source_location::current()
#endif // __cplusplus >= 202002L
    )
    {
        using U = std::remove_cv_t<std::remove_pointer_t<T>>;

        U *p = (U *)cv_p;

        if (!p)
            return;

        #ifndef NDEBUG
        if (allocated_pointers.find(p) == allocated_pointers.end())
        {
            #if __cplusplus >= 202002L
            fprintf(stderr, "Invalid pointer %p passed to dealloc called from %s:%d\n", (void*)p, (const char *)loc.file_name(), (int)loc.line());
            #else
            fprintf(stderr, "Invalid pointer %p passed to dealloc\n", p);
            #endif // __cplusplus >= 202002L
            fprintf(stderr, "Pointer not found in allocated pointers set\n");
            fprintf(stderr, "This can be caused by possible double deletion or memory corruption\n");
            abort();
        }
        allocated_pointers.erase(p);
        #endif // NDEBUG

        const std::size_t header_size = sizeof(void *) + sizeof(std::size_t);
        char *user_ptr = (char *)(p);

        // recover header
        void **raw_slot = reinterpret_cast<void **>(user_ptr - header_size);
        std::size_t *count_slot = reinterpret_cast<std::size_t *>(user_ptr - sizeof(std::size_t));
        void *raw = *raw_slot;
        std::size_t N = *count_slot;

        // destroy in reverse-order
        for (std::size_t i = N; i-- > 0;)
        {
            (cv_p + i)->~T();
        }

        std::free(raw);
    }
} // namespace Hamster