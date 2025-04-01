// c++ maps using Hamster's STLAllocator

#pragma once

#include <memory/stl_allocator.hpp>
#include <map>
#include <unordered_map>

namespace Hamster
{
    template <typename Key, typename Value>
    using Map = std::map<Key, Value, std::less<Key>, STLAllocator<std::pair<const Key, Value>>>;
    
    template <typename Key, typename Value>
    using Multimap = std::multimap<Key, Value, std::less<Key>, STLAllocator<std::pair<const Key, Value>>>;

    template <typename Key, typename Value>
    using UnorderedMap = std::unordered_map<Key, Value, std::hash<Key>, std::equal_to<Key>, STLAllocator<std::pair<const Key, Value>>>;

    template <typename Key, typename Value>
    using UnorderedMultimap = std::unordered_multimap<Key, Value, std::hash<Key>, std::equal_to<Key>, STLAllocator<std::pair<const Key, Value>>>;
} // namespace Hamster
