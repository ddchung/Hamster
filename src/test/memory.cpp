// test memory

#include <memory/allocator.hpp>
#include <memory/page.hpp>
#include <memory/memory_space.hpp>
#include <memory/stl_sequential.hpp>
#include <memory/stl_map.hpp>
#include <memory/tree.hpp>
#include <platform/platform.hpp>
#include <cassert>
#include <cstdlib>

static unsigned int hash_int(unsigned int x) {
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}

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

    // Test default constructor
    {
        Hamster::Page page;
        assert(!page.is_swapped());
    }

    // Test move constructor
    {
        Hamster::Page page1;
        Hamster::Page page2(std::move(page1));
        assert(!page2.is_swapped());
    }

    // Test move assignment
    {
        Hamster::Page page1;
        Hamster::Page page2 = std::move(page1);
        assert(!page2.is_swapped());
    }

    // Test swap_in and swap_out
    {
        Hamster::Page page;
        assert(page.swap_out() == 0);
        assert(page.swap_in() == 0);
        assert(!page.is_swapped());
    }

    // Test operator[]
    {
        Hamster::Page page;
        page[0] = 42;
        assert(page[0] == 42);
    }

    // Test get_dummy
    {
        uint8_t &dummy = Hamster::Page::get_dummy();
        dummy = 99;
        assert(Hamster::Page::get_dummy() == 99);
    }

    {
        Hamster::Page page;
        for (int i = 0; i < HAMSTER_PAGE_SIZE; ++i)
        {
            page[i] = (uint8_t)i;
        }
        for (int i = 0; i < HAMSTER_PAGE_SIZE; ++i)
        {
            assert(page[i] == (uint8_t)i);
        }
        page.swap_out();
        page.swap_in();
        for (int i = 0; i < HAMSTER_PAGE_SIZE; ++i)
        {
            assert(page[i] == (uint8_t)i);
        }
    }

    // Memory Space
    Hamster::MemorySpace mem_space;
    i = mem_space.allocate_page(HAMSTER_PAGE_SIZE);
    assert(i >= 0);
    assert(mem_space.get_page_data(HAMSTER_PAGE_SIZE) != nullptr);
    assert(mem_space.get_page_data(HAMSTER_PAGE_SIZE + HAMSTER_PAGE_SIZE - 1) != nullptr);
    assert(mem_space.get_page_data(HAMSTER_PAGE_SIZE + HAMSTER_PAGE_SIZE + 0x10) == nullptr);

    // fill with data
    for (int j = 0; j < HAMSTER_PAGE_SIZE; ++j)
    {
        mem_space[HAMSTER_PAGE_SIZE + j] = (uint8_t)j;
    }

    // check data
    for (int j = 0; j < HAMSTER_PAGE_SIZE; ++j)
    {
        assert(mem_space[HAMSTER_PAGE_SIZE + j] == (uint8_t)j);
    }

    for (int j = 1; j < 16; ++j)
    {
        i = mem_space.allocate_page(HAMSTER_PAGE_SIZE + j * HAMSTER_PAGE_SIZE);
        assert(i >= 0);
    }

    // fill with random data
    for (int j = 0; j < 16 * HAMSTER_PAGE_SIZE; ++j)
    {
        mem_space[HAMSTER_PAGE_SIZE + j] = (uint8_t)hash_int(j);
    }

    // check data
    for (int j = 0; j < 16 * HAMSTER_PAGE_SIZE; ++j)
    {
        assert(mem_space[HAMSTER_PAGE_SIZE + j] == (uint8_t)hash_int(j));
    }

    // swap in and out
    mem_space.swap_out_pages();
    mem_space.swap_in_pages();

    // check data
    for (int j = 0; j < 16 * HAMSTER_PAGE_SIZE; ++j)
    {
        assert(mem_space[HAMSTER_PAGE_SIZE + j] == (uint8_t)hash_int(j));
    }

    // deallocate pages
    for (int j = 0; j < 16; ++j)
    {
        i = mem_space.deallocate_page(HAMSTER_PAGE_SIZE + j * HAMSTER_PAGE_SIZE);
        assert(i == 0);
    }

    // check that all pages are deallocated
    for (int j = 0; j < 16; ++j)
    {
        assert(mem_space.get_page_data(HAMSTER_PAGE_SIZE + j * HAMSTER_PAGE_SIZE) == nullptr);
    }

    // big data
    // might be slow, so disable
#   if !defined(ARDUINO)

    for (int j = 0; j < 256; ++j)
    {
        i = mem_space.allocate_page(j * HAMSTER_PAGE_SIZE);
        assert(i >= 0);
        for (int k = 0; k < HAMSTER_PAGE_SIZE; ++k)
        {
            uint64_t addr = j * HAMSTER_PAGE_SIZE + k;
            mem_space[addr] = (uint8_t)hash_int(addr);
            assert(mem_space[addr] == (uint8_t)hash_int(addr));
        }
    }

    mem_space.swap_out_pages();
    mem_space.swap_in_pages();

    for (uint64_t j = 0; j < 256 * HAMSTER_PAGE_SIZE; ++j)
    {
        assert(mem_space[j] == (uint8_t)hash_int(j));
    }
#   endif

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
}
