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
#include <algorithm>

static unsigned int hash_int(unsigned int x) {
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}

using namespace Hamster;

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
    MemorySpace mem;
    
    
    // ------------------------------------------------------------------------
    // Test 1: Basic page allocation and deallocation
    // ------------------------------------------------------------------------
    
    uint64_t page_addr = 0x1000;
    
    // Allocate a single page
    assert(mem.allocate_page(page_addr) == 0);
    assert(mem.is_allocated(page_addr));
    assert(mem.get_page_count() == 1);
    
    // Deallocate the page
    assert(mem.deallocate_page(page_addr) == 0);
    assert(!mem.is_allocated(page_addr));
    assert(mem.get_page_count() == 0);
    
    // Allocate multiple pages
    for (uint64_t i = 0; i < 10; i++) {
        assert(mem.allocate_page(page_addr + (i * 0x1000)) == 0);
    }
    assert(mem.get_page_count() == 10);
    
    // Try deallocating a non-existent page (should fail)
    assert(mem.deallocate_page(0x20000) == -1);
    
    // Clean up for next test
    for (uint64_t i = 0; i < 10; i++) {
        mem.deallocate_page(page_addr + (i * 0x1000));
    }
    assert(mem.get_page_count() == 0);
    
    // ------------------------------------------------------------------------
    // Test 2: Range allocation
    // ------------------------------------------------------------------------
    
    // Allocate a range spanning multiple pages
    uint64_t range_addr = 0x2000;
    uint64_t range_size = 0x5000; // 5 pages
    
    assert(mem.allocate_range(range_addr, range_size) == 0);
    assert(mem.is_allocated(range_addr));
    assert(mem.is_allocated(range_addr + range_size - 1));
    assert(mem.get_page_count() == 5);
    
    // Allocate another range with custom permissions
    uint64_t ro_range_addr = 0x10000;
    uint64_t ro_range_size = 0x2000; // 2 pages
    
    assert(mem.allocate_range(ro_range_addr, ro_range_size, MemorySpace::PERM_READ) == 0);
    assert(mem.is_allocated(ro_range_addr));
    assert(mem.check_perm(ro_range_addr, MemorySpace::PERM_READ));
    assert(!mem.check_perm(ro_range_addr, MemorySpace::PERM_WRITE));
    assert(mem.get_page_count() == 7);
    
    // Try allocating an overlapping range (should fail)
    assert(mem.allocate_range(range_addr + 0x1000, 0x1000) == -1);
    
    // Try allocating zero-sized range (should fail)
    assert(mem.allocate_range(0x30000, 0) == -1);
    
    // ------------------------------------------------------------------------
    // Test 3: Get and set bytes
    // ------------------------------------------------------------------------
    
    // Test setting and getting a byte
    assert(mem.set(range_addr, 0x42) == 0);
    assert(mem.get(range_addr) == 0x42);
    
    // Test setting and getting multiple bytes
    for (uint64_t i = 0; i < 0x100; i++) {
        assert(mem.set(range_addr + i, static_cast<uint8_t>(i)) == 0);
    }
    
    for (uint64_t i = 0; i < 0x100; i++) {
        assert(mem.get(range_addr + i) == static_cast<uint8_t>(i));
    }
    
    // Test accessing unallocated memory (should fail)
    assert(mem.set(0x20000, 0xFF) == -1);
    assert(mem.get(0x20000) == -1);
    
    // Test setting a byte in read-only memory (should fail)
    assert(mem.set(ro_range_addr, 0xFF) == -1);
    
    // ------------------------------------------------------------------------
    // Test 4: Memory operations (memcpy, memset)
    // ------------------------------------------------------------------------
    
    // Allocate two separate pages for memory operations
    uint64_t src_addr = 0x30000;
    uint64_t dst_addr = 0x31000;
    assert(mem.allocate_page(src_addr) == 0);
    assert(mem.allocate_page(dst_addr) == 0);
    
    // Fill source page with pattern
    for (uint64_t i = 0; i < 0x100; i++) {
        assert(mem.set(src_addr + i, static_cast<uint8_t>(i)) == 0);
    }
    
    // Test memcpy within memory space
    assert(mem.memcpy(dst_addr, src_addr, 0x100) == 0);
    
    // Verify the copy worked
    for (uint64_t i = 0; i < 0x100; i++) {
        assert(mem.get(dst_addr + i) == static_cast<uint8_t>(i));
    }
    
    // Test memset
    assert(mem.memset(dst_addr, 0xAA, 0x100) == 0);
    
    // Verify the memset worked
    for (uint64_t i = 0; i < 0x100; i++) {
        assert(mem.get(dst_addr + i) == 0xAA);
    }
    
    // Test memcpy with buffer
    uint8_t buffer[0x100];
    for (uint64_t i = 0; i < 0x100; i++) {
        buffer[i] = static_cast<uint8_t>(0xFF - i);
    }
    
    // Test memcpy_from_buffer
    assert(mem.memcpy_from_buffer(dst_addr, buffer, 0x100) == 0);
    
    // Verify the memcpy_from_buffer worked
    for (uint64_t i = 0; i < 0x100; i++) {
        assert(mem.get(dst_addr + i) == static_cast<uint8_t>(0xFF - i));
    }
    
    // Test memcpy_to_buffer
    uint8_t output_buffer[0x100] = {0};
    assert(mem.memcpy_to_buffer(output_buffer, dst_addr, 0x100) == 0);
    
    // Verify the memcpy_to_buffer worked
    for (uint64_t i = 0; i < 0x100; i++) {
        assert(output_buffer[i] == static_cast<uint8_t>(0xFF - i));
    }
    
    // Test invalid memory operations
    // 1. Source not allocated
    assert(mem.memcpy(dst_addr, 0x40000, 0x100) == -1);
    
    // 2. Destination not allocated
    assert(mem.memcpy(0x40000, src_addr, 0x100) == -1);
    
    // 3. Operation crosses page boundary
    assert(mem.memcpy(dst_addr, src_addr, 0x2000) == -1);
    
    // ------------------------------------------------------------------------
    // Test 5: Permission management
    // ------------------------------------------------------------------------
    
    // Allocate a page with all permissions
    uint64_t perm_addr = 0x50000;
    assert(mem.allocate_page(perm_addr, MemorySpace::PERM_READ | MemorySpace::PERM_WRITE | MemorySpace::PERM_EXECUTE) == 0);
    
    // Verify permissions
    assert(mem.check_perm(perm_addr, MemorySpace::PERM_READ));
    assert(mem.check_perm(perm_addr, MemorySpace::PERM_WRITE));
    assert(mem.check_perm(perm_addr, MemorySpace::PERM_EXECUTE));
    assert(mem.check_perm(perm_addr, MemorySpace::PERM_READ | MemorySpace::PERM_WRITE));
    assert(mem.check_perm(perm_addr, MemorySpace::PERM_READ | MemorySpace::PERM_EXECUTE));
    assert(mem.check_perm(perm_addr, MemorySpace::PERM_WRITE | MemorySpace::PERM_EXECUTE));
    assert(mem.check_perm(perm_addr, MemorySpace::PERM_READ | MemorySpace::PERM_WRITE | MemorySpace::PERM_EXECUTE));
    
    // Change permissions to read-only
    assert(mem.set_page_perms(perm_addr, MemorySpace::PERM_READ) == 0);
    
    // Verify new permissions
    assert(mem.check_perm(perm_addr, MemorySpace::PERM_READ));
    assert(!mem.check_perm(perm_addr, MemorySpace::PERM_WRITE));
    assert(!mem.check_perm(perm_addr, MemorySpace::PERM_EXECUTE));
    
    // Try writing to read-only memory (should fail)
    assert(mem.set(perm_addr, 0x42) == -1);
    
    // Change permissions to read-write
    assert(mem.set_page_perms(perm_addr, MemorySpace::PERM_READ | MemorySpace::PERM_WRITE) == 0);
    
    // Try writing again (should succeed)
    assert(mem.set(perm_addr, 0x42) == 0);
    assert(mem.get(perm_addr) == 0x42);
    
    // Try setting permissions on non-allocated page (should fail)
    assert(mem.set_page_perms(0x60000, MemorySpace::PERM_READ) == -1);
    
    // ------------------------------------------------------------------------
    // Test 6: Edge cases
    // ------------------------------------------------------------------------
    
    // Allocate at address 0
    assert(mem.allocate_page(0) == 0);
    assert(mem.is_allocated(0));
    
    // Allocate at a very high address
    uint64_t high_addr = 0xFFFFFFFFFFFF0000;
    assert(mem.allocate_page(high_addr) == 0);
    assert(mem.is_allocated(high_addr));
    
    // Try setting and getting from these locations
    assert(mem.set(0, 0x55) == 0);
    assert(mem.get(0) == 0x55);
    
    assert(mem.set(high_addr, 0xAA) == 0);
    assert(mem.get(high_addr) == 0xAA);
    
    // ------------------------------------------------------------------------
    // Test 7: Swapping mechanism
    // ------------------------------------------------------------------------
    
    // First, allocate many pages to potentially trigger swapping
    Vector<uint64_t> many_pages;
    for (uint64_t i = 0; i < 100; i++) {
        uint64_t addr = 0x100000 + (i * 0x1000);
        assert(mem.allocate_page(addr) == 0);
        many_pages.push_back(addr);
        
        // Set a marker value in each page
        assert(mem.set(addr, static_cast<uint8_t>(i)) == 0);
    }
    
    // Access the pages in reverse order to potentially trigger swapping
    for (auto it = many_pages.rbegin(); it != many_pages.rend(); ++it) {
        uint64_t addr = *it;
        uint8_t expected = static_cast<uint8_t>((addr - 0x100000) / 0x1000);
        assert(mem.get(addr) == expected);
    }
    
    // Try swapping off all pages
    assert(mem.swap_off_all() == 0);
    
    // Access the pages again - they should be swapped back in automatically
    for (uint64_t addr : many_pages) {
        uint8_t expected = static_cast<uint8_t>((addr - 0x100000) / 0x1000);
        assert(mem.get(addr) == expected);
    }
    
    // ------------------------------------------------------------------------
    // Test 8: Comprehensive memory test with random access patterns
    // ------------------------------------------------------------------------
    
    // Allocate a large continuous region
    uint64_t large_base = 0x200000;
    uint64_t large_size = 0x10000; // 16 pages
    assert(mem.allocate_range(large_base, large_size) == 0);
    
    // Fill with pattern
    for (uint64_t i = 0; i < large_size; i++) {
        assert(mem.set(large_base + i, static_cast<uint8_t>(i & 0xFF)) == 0);
    }
    
    // Create a random access pattern
    Vector<uint64_t> access_pattern;
    for (uint64_t i = 0; i < 1000; i++) {
        access_pattern.push_back(large_base + (i * 37) % large_size);
    }
    
    // Shuffle the access pattern
    std::random_shuffle(access_pattern.begin(), access_pattern.end());
    
    // Access according to pattern
    for (uint64_t addr : access_pattern) {
        uint8_t expected = static_cast<uint8_t>((addr - large_base) & 0xFF);
        assert(mem.get(addr) == expected);
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
}
