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

#ifndef NDEBUG

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

    // Test copy constructor
    {
        Hamster::Page page1;

        // fill with sequential data
        for (int i = 0; i < HAMSTER_PAGE_SIZE; ++i)
        {
            page1[i] = (uint8_t)i;
        }
        Hamster::Page page2(page1);
        assert(!page2.is_swapped());
        // Check if data is copied correctly
        for (int i = 0; i < HAMSTER_PAGE_SIZE; ++i)
        {
            assert(page2[i] == (uint8_t)i);
        }
    }

    // Test copy assignment
    {
        Hamster::Page page1;

        // fill with sequential data
        for (int i = 0; i < HAMSTER_PAGE_SIZE; ++i)
        {
            page1[i] = (uint8_t)i;
        }
        Hamster::Page page2;
        page2 = page1;
        assert(!page2.is_swapped());
        // Check if data is copied correctly
        for (int i = 0; i < HAMSTER_PAGE_SIZE; ++i)
        {
            assert(page2[i] == (uint8_t)i);
        }
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

    // fill with data
    for (int j = 0; j < HAMSTER_PAGE_SIZE; ++j)
    {
        assert(mem_space.write_byte(HAMSTER_PAGE_SIZE + j, (uint8_t)j) == 0);
    }

    // check data
    for (int j = 0; j < HAMSTER_PAGE_SIZE; ++j)
    {
        uint8_t val;
        assert(mem_space.read_byte(HAMSTER_PAGE_SIZE + j, val) == 0);
        assert(val == (uint8_t)j);
    }

    // fill with random data
    for (int j = 0; j < 16 * HAMSTER_PAGE_SIZE; ++j)
    {
        assert(mem_space.write_byte(HAMSTER_PAGE_SIZE + j, (uint8_t)hash_int(j)) == 0);
    }

    // check data
    for (int j = 0; j < 16 * HAMSTER_PAGE_SIZE; ++j)
    {
        uint8_t val;
        assert(mem_space.read_byte(HAMSTER_PAGE_SIZE + j, val) == 0);
        assert(val == (uint8_t)hash_int(j));
    }

    mem_space.swap_out_all();

    // check data
    for (int j = 0; j < 16 * HAMSTER_PAGE_SIZE; ++j)
    {
        uint8_t val;
        assert(mem_space.read_byte(HAMSTER_PAGE_SIZE + j, val) == 0);
        assert(val == (uint8_t)hash_int(j));
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
        assert(!mem_space.is_allocated(HAMSTER_PAGE_SIZE + j * HAMSTER_PAGE_SIZE));
    }

    // memcpy
    uint8_t *src = Hamster::alloc<uint8_t>(0x1234);
    uint8_t *dest = Hamster::alloc<uint8_t>(0x1234);

    for (int j = 0; j < 0x1234; ++j)
    {
        src[j] = (uint8_t)hash_int(j);
    }

    assert(mem_space.memcpy(0x1234, src, 0x1234) == 0);
    assert(mem_space.memcpy(dest, 0x1234, 0x1234) == 0);

    for (uint64_t j = 0; j < 0x1234; ++j)
    {
        assert(dest[j] == src[j]);
        assert(mem_space.write_byte(0x1234 + j, src[j]) == 0);
    }

    Hamster::dealloc(src);
    Hamster::dealloc(dest);

    // big data
    // might be slow, so disable
#   if !defined(ARDUINO)

    for (int j = 0; j < 256; ++j)
    {
        assert(i >= 0);
        for (int k = 0; k < HAMSTER_PAGE_SIZE; ++k)
        {
            uint64_t addr = j * HAMSTER_PAGE_SIZE + k;
            assert(mem_space.write_byte(addr, (uint8_t)hash_int(addr)) == 0);
        }
    }

    mem_space.swap_out_all();

    for (uint64_t j = 0; j < 256 * HAMSTER_PAGE_SIZE; ++j)
    {
        uint8_t val;
        assert(mem_space.read_byte(j, val) == 0);
        assert(val == (uint8_t)hash_int(j));
    }
#   endif

    // Test memory space copy constructor

    // fill with some data
    for (int j = 0; j < HAMSTER_PAGE_SIZE; ++j)
    {
        assert(mem_space.write_byte(HAMSTER_PAGE_SIZE + j, (uint8_t)j) == 0);
    }
    // check data
    for (int j = 0; j < HAMSTER_PAGE_SIZE; ++j)
    {
        uint8_t val;
        assert(mem_space.read_byte(HAMSTER_PAGE_SIZE + j, val) == 0);
        assert(val == (uint8_t)j);
    }
    // create a copy
    Hamster::MemorySpace mem_space_copy(mem_space);
    // check data in the copy
    for (int j = 0; j < HAMSTER_PAGE_SIZE; ++j)
    {
        uint8_t val;
        assert(mem_space_copy.read_byte(HAMSTER_PAGE_SIZE + j, val) == 0);
        assert(val == (uint8_t)j);
    }
    // Another copy
    Hamster::MemorySpace mem_space_copy2;
    mem_space_copy2 = mem_space;
    // check data in the second copy
    for (int j = 0; j < HAMSTER_PAGE_SIZE; ++j)
    {
        uint8_t val;
        assert(mem_space_copy2.read_byte(HAMSTER_PAGE_SIZE + j, val) == 0);
        assert(val == (uint8_t)j);
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

    // Page edge cases
    Hamster::Page page;
    page.swap_out();
    // Accessing swapped-out page returns dummy
    uint8_t &dummy_ref = page[0];
    dummy_ref = 55;
    assert(Hamster::Page::get_dummy() == 55);
    page.swap_in();
    // Out-of-bounds access returns dummy
    uint8_t &oob_ref = page[HAMSTER_PAGE_SIZE + 1000];
    oob_ref = 77;
    assert(Hamster::Page::get_dummy() == 77);
    // Set/get flags
    page.get_flags() = 0xABCD;
    assert(page.get_flags() == 0xABCD);

    // MemorySpace edge cases
    Hamster::MemorySpace ms;
    // Write/read to unallocated address (should auto-allocate)
    assert(ms.write_byte(0x100000, 0x42) == 0);
    uint8_t val = 0;
    assert(ms.read_byte(0x100000, val) == 0 && val == 0x42);
    // Deallocate already deallocated page
    assert(ms.deallocate_page(0x100000) == 0);
    assert(ms.deallocate_page(0x100000) == -1);
    // Allocate page at 0x200000
    assert(ms.write_byte(0x200000, 0x0) == 0);
    // Set permissions and check enforcement
    assert(ms.set_permissions(0x200000, 0x0) == 0); // no access
    assert(ms.write_byte(0x200000, 0x11) == -1); // should fail
    assert(ms.set_permissions(0x200000, 0x2) == 0); // write only
    assert(ms.write_byte(0x200000, 0x22) == 0);
    assert(ms.set_permissions(0x200000, 0x1) == 0); // read only
    assert(ms.write_byte(0x200000, 0x33) == -1); // should fail
    // Copy/move assignment and self-assignment
    Hamster::MemorySpace ms2;
    ms2 = ms;
    ms2 = ms2;
    Hamster::MemorySpace ms3(std::move(ms2));
    ms3 = std::move(ms3);

    // mmap/munmap edge cases (simulate with invalid params)
    assert(ms.mmap(0x300000, 0x1000, 0x3, MAP_PRIVATE, -1, 0) == -1); // invalid fd

    // STLAllocator with map
    Hamster::Map<int, int> stl_map;
    stl_map[1] = 2;
    stl_map[3] = 4;
    assert(stl_map[1] == 2 && stl_map[3] == 4);
}

#endif // NDEBUG
