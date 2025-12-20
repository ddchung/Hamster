// Static object pool for fixed-size allocation

#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <cassert>
#include <bitset>

namespace Hamster
{
    template <size_t Size, size_t Capacity, size_t Align = 1>
    class StaticAllocator
    {
        static_assert(Size > 0, "Size must be greater than zero");
        static_assert(Align > 0, "Align must be nonzero");
        static_assert(Capacity > 0, "Capacity must be greater than zero");
        static_assert((Align & (Align - 1)) == 0, "Align must be a power of two");
        static_assert((Size % Align) == 0, "Size must be a multiple of Align");
        static_assert(Size * 8 > std::bit_width(Capacity - 1), "Size is too small to index all objects in the pool");

        template<size_t Bits>
        using intx_t = 
            std::conditional_t<Bits <= 8,  int8_t,
            std::conditional_t<Bits <= 16, int16_t,
            std::conditional_t<Bits <= 32, int32_t, int64_t>>>;
        
        union alignas(Align) Object
        {
            // Has object
            uint8_t data[Size];
            
            // Free object, index of next free object (or -1 if end of free list)
            intx_t<Size> next;
        };

    public:
        StaticAllocator()
        {
            // Initialize pool as all free
            // forward linked list
            for (size_t i = 0; i < Capacity - 1; ++i)
                objects[i].next = i + 1;
            objects[Capacity - 1].next = -1;
            first_free = 0;
        }

        /**
         * @brief Allocate an object from the pool
         * @return The object, or nullptr if the pool is full
         */
        void *allocate()
        {
            if (first_free == -1)
                return nullptr;

            assert((size_t)first_free < Capacity);
            Object *obj = objects + first_free;
            first_free = obj->next;
            return obj;
        }

        /**
         * @brief Deallocate an object back to the pool
         * @param obj The object to deallocate
         */
        void deallocate(void *void_obj)
        {
            Object *obj = reinterpret_cast<Object *>(void_obj);
            assert(obj != nullptr && obj >= objects && obj < objects + Capacity);
            obj->next = first_free;
            first_free = obj - objects;
        }

        /**
         * @brief Get a map of free objects
         * @return A bitset where each bit represents whether the corresponding object is free
         */
        std::bitset<Capacity> get_free_map() const
        {
            std::bitset<Capacity> free_map;
            auto index = first_free;
            while (index != -1)
            {
                free_map.set(index);
                index = objects[index].next;
            }
            return free_map;
        }

        /**
         * @brief Get an object by index. Must be allocated.
         * @param index The index of the object
         * @note To be used wtih get_free_map() to iterate allocated objects
         */
        Object *get_object(size_t index)
        {
            assert(index < Capacity);
            assert(!get_free_map().test(index)); // must be allocated. slow, but only on debug build
            return &objects[index];
        }

    private:
        Object objects[Capacity];
        intx_t<Size> first_free;
    };

    template <typename T, size_t Capacity>
    class StaticPool
    {
    public:
        StaticPool() = default;
        StaticPool(StaticPool &&) = delete;
        ~StaticPool()
        {
            if constexpr(!std::is_trivially_destructible_v<T>)
            {
                // Destroy allocated objects
                std::bitset<Capacity> free_map = allocator.get_free_map();
                for (size_t i = 0; i < Capacity; ++i)
                    if (!free_map.test(i)) // allocated
                        ((T *)allocator.get_object(i))->~T();
            }
        }

        /**
         * @brief Allocate an object
         */
        template <typename... Args>
        T *allocate(Args&&... args)
        {
            auto obj = (T *)allocator.allocate();
            if constexpr(!std::is_trivially_constructible_v<T, Args...> || sizeof...(Args) > 0)
                if (obj)
                    new (obj) T(std::forward<Args>(args)...);
            return obj;
        }

        /**
         * @brief Deallocate an object
         */
        void deallocate(T *obj)
        {
            assert(obj != nullptr);
            if constexpr(!std::is_trivially_destructible_v<T>)
                obj->~T();
            allocator.deallocate((StaticAllocator<sizeof(T), Capacity, alignof(T)> *)obj);
        }

    private:
        StaticAllocator<sizeof(T), Capacity, alignof(T)> allocator;
    };
} // namespace Hamster

