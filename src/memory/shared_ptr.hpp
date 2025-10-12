// Hamster reference counted container

#pragma once

#include <memory/allocator.hpp>
#include <utility>
#include <cstddef>
#include <cassert>
#include <type_traits>

namespace Hamster
{
    enum class SharedPtrCopyType
    {
        DEEP,
        SHALLOW
    };

    /**
     * @brief A lightweight shared_ptr because `std::shared_ptr` is too big
     * @note Cannot hold polymorphic types
     */
    template <typename T, 
              typename RefCountType = size_t,
              SharedPtrCopyType default_copy_type = SharedPtrCopyType::DEEP>
    class SharedPtr
    {
        static_assert(!std::is_polymorphic_v<T>, "SharedPtr: T must not be polymorphic");

        struct Object
        {
            template <typename... Args>
            Object(Args&&... args)
                : obj(std::forward<Args>(args)...), refcount(1)
            {
            }
            
            T obj;
            RefCountType refcount;
        };
    public:
        struct Construct {};

        /**
         * @brief Construct a new SharedPtr with a new object
         * @param args The args to forward to the object
         * @note Pass a Construct() to signify constructing the shared pointer's object
         */
        template <typename... Args>
        SharedPtr(Construct, Args &&... args)
        {
            obj = nullptr;
            construct(std::forward<Args>(args)...);
        }

        SharedPtr(std::nullptr_t = nullptr)
        {
            obj = nullptr;
        }

        /**
         * @brief Construct a new value
         * @param args The args to pass to T's constructor
         */
        template <typename... Args>
        void construct(Args &&... args)
        {
            clear();
            obj = alloc<Object>(1, std::forward<Args>(args)...);
        }

        /**
         * @brief Copy a shared pointer
         * @param other The other shared pointer
         * @param copy_type One of `CopyType::{DEEP, SHALLOW}`
         * @warning For shallow copies, `OtherT` must equal `T` and `OtherRefType` must equal `RefCountType`
         */
        template <typename OtherT, typename OtherRefType, SharedPtrCopyType other_cpy_type>
        SharedPtr(const SharedPtr<OtherT, OtherRefType, other_cpy_type> &other, SharedPtrCopyType copy_type = default_copy_type)
        {
            // Ensure obj is not garbage
            obj = nullptr;
            assign(other, copy_type);
        }

        // Copy constructor
        SharedPtr(const SharedPtr &other)
            : SharedPtr(other, default_copy_type)
        {
        }

        SharedPtr(SharedPtr &&other)
        {
            obj = other.obj;
            other.obj = nullptr;
        }

        /**
         * @brief Assign a shared pointer (like copy constructor)
         * @param other The other shared pointer
         * @param copy_type One of `CopyType::{DEEP, SHALLOW}`
         * @note See comments on copy constructor
         * @return *this
         */
        template <typename OtherT, typename OtherRefType, SharedPtrCopyType other_cpy_type>
        SharedPtr &assign(const SharedPtr<OtherT, OtherRefType, other_cpy_type> &other, SharedPtrCopyType copy_type = default_copy_type)
        {
            if ((void *)obj == (void *)other.obj)
                return *this;

            clear();

            if (!other.obj)
            {
                obj = nullptr;
                return *this;
            }

            switch (copy_type)
            {
            case SharedPtrCopyType::DEEP:
                if constexpr(std::is_constructible_v<T, OtherT &>)
                    obj = alloc<Object>(1, other.obj->obj);
                else
                {
                    assert(false && "SharedPtr: Cannot deep-copy object with no copy constructor");
                    __builtin_unreachable();
                }
                break;
            case SharedPtrCopyType::SHALLOW:
                // note: cannot put into assert because of comma
                constexpr auto is_same = std::is_same_v<T, OtherT> && std::is_same_v<RefCountType, OtherRefType>;
                if constexpr(!is_same)
                {
                    assert(false && "SharedPtr: Cannot shallow-copy from different type");
                    __builtin_unreachable();
                }

                obj = other.obj;
                obj->refcount++;
                break;
            }

            return *this;
        }

        // Assign with the default copy type
        template <typename OtherT, typename OtherRefType, SharedPtrCopyType other_cpy_type>
        SharedPtr &operator=(const SharedPtr<OtherT, OtherRefType, other_cpy_type> &other)
        {
            return assign(other);
        }

        // Copy assign
        SharedPtr &operator=(const SharedPtr &other)
        {
            return assign(other);
        }

        SharedPtr &operator=(SharedPtr &&other)
        {
            if ((void *)obj == (void *)other.obj)
                return *this;

            clear();
            obj = other.obj;
            other.obj = nullptr;
            return *this;
        }

        ~SharedPtr()
        {
            clear();
        }

        explicit operator bool() const
        {
            return obj != nullptr;
        }

        /**
         * @brief Get the object
         * @return A reference to the object
         */
        T &operator *() const
        {
            return obj->obj;
        }

        /**
         * @brief Alias for `**this`
         */
        T &get() const
        {
            return obj->obj;
        }

        /**
         * @brief Access a member of the object
         */
        T *operator ->() const
        {
            return &obj->obj;
        }

        /**
         * @brief Clear the shared pointer
         */
        void clear()
        {
            if (obj)
            {
                if (--obj->refcount == 0)
                    dealloc(obj);
                obj = nullptr;
            }
        }

    private:
        Object *obj;
    };
} // namespace Hamster
