// Hamster small unique pointer

#pragma once

#include <memory/allocator.hpp>

namespace Hamster
{
    template <typename T>
    class UniquePtr
    {
    public:
        /**
         * @brief Constructs a unique pointer, taking ownership
         * @param obj The object to take
         */
        UniquePtr(T *obj = nullptr) : obj(obj) {}

        UniquePtr(const UniquePtr &) = delete;
        UniquePtr &operator=(const UniquePtr &) = delete;
        UniquePtr(UniquePtr &&other) : obj(other.obj) { other.obj = nullptr; }
        UniquePtr &operator=(UniquePtr &&other)
        {
            if (this != &other)
            {
                clear();
                std::swap(obj, other.obj);
            }
            return *this;
        }
        ~UniquePtr()
        {
            clear();
        }

        explicit operator bool() const { return obj; }

        T &operator*() { return *obj; }
        const T &operator*() const { return *obj; }
        T *operator->() { return obj; }
        const T *operator->() const { return obj; }
        T *get() { return obj; }
        const T *get() const { return obj; }
        void clear()
        {
            dealloc(obj);
            obj = nullptr;
        }
        T *release()
        {
            T *o = obj;
            obj = nullptr;
            return o;
        }
        template <typename... Args>
        void construct(Args &&... args)
        {
            clear();
            obj = alloc<T>(1, std::forward<Args>(args)...);
        }

    private:
        T *obj;
    };
} // namespace Hamster

