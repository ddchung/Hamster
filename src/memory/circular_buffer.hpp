// Circular buffer data structure

#pragma once

#include <memory/allocator.hpp>
#include <cstddef>
#include <cstdint>
#include <cassert>
#include <utility>

namespace Hamster
{
    template <typename T>
    class CircularBuffer
    {
        struct Node
        {
            alignas(T) uint8_t data[sizeof(T)];
            Node * next = nullptr;
            Node * prev = nullptr;
            bool is_guard : 1 = false;
        };
    public:
        CircularBuffer();
        CircularBuffer(const CircularBuffer&) = delete;
        CircularBuffer& operator=(const CircularBuffer&) = delete;
        CircularBuffer(CircularBuffer&&);
        CircularBuffer& operator=(CircularBuffer&&);
        ~CircularBuffer();

        /**
         * @brief Get the front element
         * @warning This results in undefined behavior if `size() == 0`
         */
        T & front();

        /**
         * @brief Remove the front element
         */
        void pop();

        /**
         * @brief Insert an element, such that calling `front()` right after returns this new element.
         * @param value The element to insert
         */
        template <typename U>
        void push(U&& value)
        { emplace(std::forward<U>(value)); }

        /**
         * @brief Directly construct an element at the front
         * @param args The arguments to forward to the element's constructor
         */
        template <typename... Args>
        void emplace(Args&&... args);

        /**
         * @brief Move forward
         * @note This always has defined behavior, if the buffer is empty, the position will not change.
         */
        void advance();

        /**
         * @brief Move backward
         * @note This always has defined behavior, if the buffer is empty, the position will not change.
         */
        void retreat();

        /**
         * @brief Get the current size
         */
        size_t size() const
        { return count; }

        bool empty() const
        { return count == 0; }

    private:
        Node * pos;
        size_t count;
    };

    template <typename T>
    CircularBuffer<T>::CircularBuffer()
        : pos(alloc<Node>()), count(0)
    {
        pos->next = pos;
        pos->prev = pos;
        pos->is_guard = true;
    }

    template <typename T>
    CircularBuffer<T>::CircularBuffer(CircularBuffer &&other)
        : CircularBuffer()
    {
        std::swap(pos, other.pos);
        std::swap(count, other.count);
    }

    template <typename T>
    CircularBuffer<T>& CircularBuffer<T>::operator=(CircularBuffer &&other)
    {
        std::swap(pos, other.pos);
        std::swap(count, other.count);
        return *this;
    }

    template <typename T>
    CircularBuffer<T>::~CircularBuffer()
    {
        while (count > 0)
            pop();
        // Destroy the single guard
        dealloc(pos);

        pos = nullptr;
    }

    template <typename T>
    T & CircularBuffer<T>::front()
    {
        assert(count > 0);
        return *(T*)&pos->data;
    }

    template <typename T>
    void CircularBuffer<T>::pop()
    {
        assert(count > 0);
        assert(!pos->is_guard);
        Node * prev = pos->prev;
        Node * next = pos->next;
        prev->next = next;
        next->prev = prev;

        // Destroy the T
        ((T*)&pos->data)->~T();

        dealloc(pos);
        --count;

        pos = next;
        if (pos->is_guard)
            pos = pos->next;
    }

    template <typename T>
    template <typename... Args>
    void CircularBuffer<T>::emplace(Args&&... args)
    {
        // Make node
        Node *new_node = alloc<Node>();
        new ((T*)&new_node->data) T(std::forward<Args>(args)...);

        // Add to circle
        new_node->prev = pos;
        new_node->next = pos->next;
        pos->next->prev = new_node;
        pos->next = new_node;
        ++count;

        // see comment in declaration
        pos = new_node;
    }

    template <typename T>
    void CircularBuffer<T>::advance()
    {
        pos = pos->next;
        if (pos->is_guard)
            pos = pos->next;
    }

    template <typename T>
    void CircularBuffer<T>::retreat()
    {
        pos = pos->prev;
        if (pos->is_guard)
            pos = pos->prev;
    }
} // namespace Hamster

