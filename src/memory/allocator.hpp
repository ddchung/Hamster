
#pragma once

#include <platform/platform.hpp>
#include <utility>
#include <cassert>
#include <cstdint>
#include <new>
#include <type_traits>

#ifndef NDEBUG

#include <unordered_map>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <typeinfo>

#if __cplusplus >= 202002L
# include <source_location>
#endif // __cplusplus >= 202002L

#endif // NDEBUG

namespace Hamster
{
#ifndef NDEBUG
    inline std::unordered_map<void *, std::string> allocated_pointers;

    template <typename T>
    inline const char *type_name()
    {
        return __PRETTY_FUNCTION__;
    }

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
        allocated_pointers[user_ptr] = type_name<T>();
#endif // NDEBUG

        for (unsigned int i = 0; i < N; ++i)
        {
            new (user_ptr + i) T(std::forward<Args>(args)...);
        }
        return user_ptr;
    }

    template <typename T>
    void dealloc(T *ptr_
#if !defined(NDEBUG) && __cplusplus >= 202002L
        , std::source_location loc = std::source_location::current()
#endif // !defined(NDEBUG) && __cplusplus >= 202002L
        )
    {
        using U = std::remove_cv_t<std::remove_pointer_t<T>>;

        auto ptr = (U *)ptr_;

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
        else if (allocated_pointers[ptr] != type_name<U>())
        {
# if __cplusplus >= 202002L
            printf("Warning: possible type mismatch detected at %s:%d\n", loc.file_name(), (int)loc.line());
            printf("Expected: %s\n", allocated_pointers[ptr].c_str());
            printf("Actual: %s\n", type_name<U>());
# else // __cplusplus >= 202002L
            printf("Warning: possible type mismatch detected\n");
            printf("Expected: %s\n", allocated_pointers[ptr].c_str());
            printf("Actual: %s\n", type_name<U>());
# endif // __cplusplus >= 202002L
        }
        allocated_pointers.erase(ptr);
#endif // NDEBUG

        constexpr unsigned int additional = (sizeof(unsigned int) + sizeof(T) - 1) / sizeof(T);
        unsigned int *meta_ptr = (unsigned int *)(ptr - additional);
        unsigned int N = *meta_ptr;

        for (unsigned int i = 0; i < N; ++i)
        {
            ptr[i].~U();
        }

        _free(meta_ptr);
    }
} // namespace Hamster