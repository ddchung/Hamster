
#pragma once

#include <platform/platform.hpp>
#include <utility>
#include <cassert>
#include <new>


namespace Hamster 
{
    template <typename T, typename... Args>
    T* alloc(unsigned int N = 1, Args&&... args)
    {
        void *ptr = _malloc(sizeof(T) * N);
        assert(ptr != nullptr);
        for (unsigned int i = 0; i < N; ++i) {
            new (static_cast<T*>(ptr) + i) T(std::forward<Args>(args)...);
        }
        return static_cast<T*>(ptr);
    }

    template <typename T>
    void dealloc(T* ptr) {
        if (ptr) {
            ptr->~T();
            _free(ptr);
        }
    }

    template <typename T, unsigned int N>
    void dealloc(T* ptr) {
        if (ptr) {
            for (unsigned int i = 0; i < N; ++i) {
                (static_cast<T*>(ptr) + i)->~T();
            }
            _free(ptr);
        }
    }
} // namespace Hamster