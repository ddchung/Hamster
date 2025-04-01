// C++ sequential containers using Hamster's STLAllocator

#pragma once

#include <memory/stl_allocator.hpp>
#include <deque>
#include <forward_list>
#include <list>
#include <vector>
#include <string>

namespace Hamster
{
    template <typename T>
    using Vector = std::vector<T, STLAllocator<T>>;
    
    template <typename T>
    using Deque = std::deque<T, STLAllocator<T>>;
    
    template <typename T>
    using List = std::list<T, STLAllocator<T>>;
    
    template <typename T>
    using ForwardList = std::forward_list<T, STLAllocator<T>>;
    
    using String = std::basic_string<char, std::char_traits<char>, STLAllocator<char>>;
} // namespace Hamster
