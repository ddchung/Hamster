// C++ sets using Hamster's STLAllocator

#pragma once

#include <memory/stl_allocator.hpp>
#include <set>
#include <unordered_set>

namespace Hamster
{
    template <typename Key>
    using Set = std::set<Key, std::less<Key>, STLAllocator<Key>>;
    
    template <typename Key>
    using Multiset = std::multiset<Key, std::less<Key>, STLAllocator<Key>>;

    template <typename Key>
    using UnorderedSet = std::unordered_set<Key, std::hash<Key>, std::equal_to<Key>, STLAllocator<Key>>;

    template <typename Key>
    using UnorderedMultiset = std::unordered_multiset<Key, std::hash<Key>, std::equal_to<Key>, STLAllocator<Key>>;
} // namespace Hamster
