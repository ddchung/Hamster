// test memory

#include <memory/allocator.hpp>
#include <cassert>

void test_memory()
{
    int *ptr = Hamster::alloc<int>(42);
    assert(ptr != nullptr);
    assert(*ptr == 42);
    Hamster::dealloc(ptr);

    int *arr = Hamster::alloc<int, 10>(42);
    assert(arr != nullptr);
    for (int i = 0; i < 10; ++i)
    {
        assert(arr[i] == 42);
    }
    Hamster::dealloc(arr);
}
