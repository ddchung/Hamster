// C++ basic tree data structure

#pragma once

#include <memory/stl_sequential.hpp>
#include <utility>

namespace Hamster
{
    template <typename T>
    class Tree
    {
        struct Node
        {
            explicit Node() : parent(nullptr) {}

            template <typename U>
            explicit Node(U&& value) : data(std::forward<U>(value)), parent(nullptr) {}

            template <typename... Args>
            explicit Node(Args&&... args) : data(std::forward<Args>(args)...), parent(nullptr) {}

            T data;
            Vector<Node> children;
            Node *parent;
        };
        
    public:
        class Iterator
        {
        public:
            friend class Tree;

            inline operator bool() const { return current != nullptr; }
            inline T &operator*() { return current->data; }
            inline T *operator->() { return &current->data; }
            inline Vector<Node> &children() { return current->children; }

            void remove(size_t index);

            template <typename U>
            void insert(U&& value);

            template <typename... Args>
            void emplace(Args&&... args);

            // Get an iterator to a child at index
            Iterator operator[](size_t index);

            // Move *this to child at index
            // returns *this
            Iterator &move_to(size_t index);
            
            // Move through the tree in a depth-first manner
            // returns *this
            Iterator &operator++();


            // Move *this to *this->parent
            // returns *this
            Iterator &operator--();

            // postfix version of ++
            Iterator operator++(int);

            // postfix version of --
            Iterator operator--(int);


        private:
            Iterator(Node *node) : current(node) {}
            Node *current;
        };

        Tree() : r() {}

        // Get an iterator to the root
        Iterator root() { return Iterator(&r); }

        // Get a depth-first iterator to the root
        Iterator begin() { return Iterator(&r); }

        Iterator end() { return Iterator(nullptr); }

    private:
        Node r;
    };
} // namespace Hamster

template <typename T>
void Hamster::Tree<T>::Iterator::remove(size_t index)
{
    if (current == nullptr || index >= current->children.size())
        return;

    current->children.erase(current->children.begin() + index);
}


template <typename T>
template <typename U>
void Hamster::Tree<T>::Iterator::insert(U&& value)
{
    if (current == nullptr)
        return;
        
    current->children.emplace_back(std::forward<U>(value));
    current->children.back().parent = current;
}

template <typename T>
template <typename... Args>
void Hamster::Tree<T>::Iterator::emplace(Args&&... args)
{
    if (current == nullptr)
        return;

    current->children.emplace_back(std::forward<Args>(args)...);
    current->children.back().parent = current;
}

template <typename T>
typename Hamster::Tree<T>::Iterator Hamster::Tree<T>::Iterator::operator[](size_t index)
{
    if (current == nullptr || index >= current->children.size())
        return Iterator(nullptr);

    return Iterator(&current->children[index]);
}

template <typename T>
typename Hamster::Tree<T>::Iterator &Hamster::Tree<T>::Iterator::move_to(size_t index)
{
    if (current != nullptr && index < current->children.size())
        current = &current->children[index];
    else
        current = nullptr;
    return *this;
}

template <typename T>
typename Hamster::Tree<T>::Iterator &Hamster::Tree<T>::Iterator::operator++()
{
    if (current == nullptr)
        return *this;
    if (current->children.size() > 0)
        current = &current->children[0];
    else while (current->parent != nullptr)
    {
        size_t new_index = current - current->parent->children.data() + 1;
        if (new_index >= current->parent->children.size())
            current = current->parent;
        else
        {
            current = &current->parent->children[new_index];
            return *this;
        }
    }
    
    current = nullptr;
    return *this;
}

template <typename T>
typename Hamster::Tree<T>::Iterator Hamster::Tree<T>::Iterator::operator++(int)
{
    Iterator temp = *this;
    ++(*this);
    return temp;
}

template <typename T>
typename Hamster::Tree<T>::Iterator &Hamster::Tree<T>::Iterator::operator--()
{
    if (current != nullptr && current->parent != nullptr)
        current = current->parent;
    else
        current = nullptr;
    return *this;
}

template <typename T>
typename Hamster::Tree<T>::Iterator Hamster::Tree<T>::Iterator::operator--(int)
{
    Iterator temp = *this;
    --(*this);
    return temp;
}
