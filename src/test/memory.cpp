// test memory

#include <memory/allocator.hpp>
#include <memory/page_manager.hpp>
#include <memory/stl_sequential.hpp>
#include <memory/stl_map.hpp>
#include <memory/tree.hpp>
#include <platform/platform.hpp>
#include <cassert>
#include <cstdlib>
#include <cstring>

#ifndef NDEBUG

void test_memory()
{
    int i;

    Hamster::dealloc((int *)nullptr); // should not crash

    int *ptr = Hamster::alloc<int>(1, 42);
    assert(ptr != nullptr);
    assert(*ptr == 42);
    Hamster::dealloc(ptr);

    Hamster::dealloc(Hamster::alloc<int>(1, 42));
    Hamster::dealloc(Hamster::alloc<int>(1, 42));
    Hamster::dealloc(Hamster::alloc<int>(1, 42));
    Hamster::dealloc(Hamster::alloc<int>(1, 42));

    Hamster::dealloc((int *)nullptr);
    Hamster::dealloc((int *)nullptr);
    Hamster::dealloc((int *)nullptr);

    int *arr = Hamster::alloc<int>(10, 42);
    assert(arr != nullptr);
    for (int i = 0; i < 10; ++i)
    {
        assert(arr[i] == 42);
    }
    Hamster::dealloc(arr);

    // more rigorous allocator tests
    {
        for (int i = 0; i < 100; ++i)
        {
            Hamster::Vector<Hamster::Vector<Hamster::Vector<int>>> vec3;
            Hamster::Vector<Hamster::Vector<int>> vec2;
            Hamster::Vector<int> vec1;

            for (int i = 0; i < 10; ++i)
            {
                vec1.push_back(i);
            }

            for (int i = 0; i < 10; ++i)
            {
                vec2.push_back(vec1);
            }

            for (int i = 0; i < 10; ++i)
            {
                vec3.push_back(vec2);
            }

            for (int i = 0; i < 10; ++i)
            {
                for (int j = 0; j < 10; ++j)
                {
                    for (int k = 0; k < 10; ++k)
                    {
                        assert(vec3[i][j][k] == k);
                    }
                }
            }
        }
    }

    // Page Manager
    Hamster::PageManager &pm = Hamster::page_manager;

    Hamster::PageEntry *entry = nullptr;
    uint32_t id = pm.allocate_page(entry);
    assert(id != -1);
    assert(entry != nullptr);

    pm.free_page(id);

    entry = nullptr;
    id = pm.allocate_page(entry);
    assert(id != -1);
    assert(entry != nullptr);

    // Write some data to the page
    const char *data = "Hello, World!";
    ssize_t bytes_written = pm.try_write(id, 0, data, strlen(data));
    if (bytes_written == -1)
    {
        pm.swap_in(id);
        bytes_written = pm.try_write(id, 0, data, strlen(data));
    }
    assert(bytes_written != -1);

    // Read the data back from the page
    char buffer[256];
    ssize_t bytes_read = pm.try_read(id, 0, buffer, strlen(data));
    assert(bytes_read != -1);
    assert(bytes_read == strlen(data));
    assert(strncmp(buffer, data, strlen(data)) == 0);

    pm.free_page(id);
    // Write a lot of data to multiple pages
    constexpr size_t page_count = 512;
    for (size_t i = 0; i < page_count; ++i)
    {
        Hamster::PageEntry *entry = nullptr;
        uint32_t id = pm.allocate_page(entry);
        assert(entry != nullptr);

        // Write some data to the page
        ssize_t bytes_written = pm.try_write(id, 0, data, strlen(data));
        if (bytes_written == -1)
        {
            pm.swap_in(id);
            bytes_written = pm.try_write(id, 0, data, strlen(data));
        }
        assert(bytes_written != -1);

        // Read the data back from the page
        ssize_t bytes_read = pm.try_read(id, 0, buffer, strlen(data));
        assert(bytes_read != -1);
        assert(bytes_read == strlen(data));
        assert(strncmp(buffer, data, strlen(data)) == 0);
    }

    // Free all allocated pages
    for (size_t i = 0; i < page_count; ++i)
    {
        pm.free_page(i);
    }

    // Tree
    Hamster::Tree<int> tree;

    /*
        5
       / \
      3   7
     / \ / \
     1 2 6 8
    */

    auto it = tree.root();

    *it = 5;

    it.emplace(3);
    it.emplace(7);

    it.move_to(0);

    it.emplace(1);
    it.emplace(2);

    --it;

    it.move_to(1);

    it.emplace(6);
    it.emplace(8);

    auto it2 = tree.root();

    assert(*it2 == 5);
    assert(*it2[0] == 3);
    assert(*it2[1] == 7);
    assert(*it2[0][0] == 1);
    assert(*it2[0][1] == 2);
    assert(*it2[1][0] == 6);
    assert(*it2[1][1] == 8);

    int expected[] = {
        5, 3, 1, 2, 7, 6, 8
    };

    i = 0;

    for (auto &x : tree)
    {
        assert(x == expected[i]);
        ++i;
    }

    // Allocating zero elements (should return nullptr or valid pointer)
    int *zero_ptr = Hamster::alloc<int>(0);
    Hamster::dealloc(zero_ptr);

    // STLAllocator edge cases
    Hamster::Vector<int> stl_vec;
    for (int i = 0; i < 100; ++i) stl_vec.push_back(i);
    for (int i = 0; i < 100; ++i) assert(stl_vec[i] == i);
    stl_vec.clear();

    // STLAllocator with map
    Hamster::Map<int, int> stl_map;
    stl_map[1] = 2;
    stl_map[3] = 4;
    assert(stl_map[1] == 2 && stl_map[3] == 4);
}

#endif // NDEBUG
