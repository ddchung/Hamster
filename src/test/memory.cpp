// test memory

#include <memory/allocator.hpp>
#include <memory/page_manager.hpp>
#include <memory/memory_space.hpp>
#include <memory/stl_sequential.hpp>
#include <memory/stl_map.hpp>
#include <memory/tree.hpp>
#include <memory/circular_buffer.hpp>
#include <memory/allocator.hpp>
#include <platform/platform.hpp>
#include <memory/shared_ptr.hpp>
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

    uint32_t id = pm.allocate_page();

    pm.free_page(id);

    id = pm.allocate_page();

    // Write some data to the page
    const char *data = "Hello, World!";
    ssize_t bytes_written = pm.write(id, 0, data, strlen(data));
    assert(bytes_written == (ssize_t)strlen(data));

    // Read the data back from the page
    char buffer[256];
    ssize_t bytes_read = pm.read(id, 0, buffer, strlen(data));
    assert(bytes_read != -1);
    assert(bytes_read == (ssize_t)strlen(data));
    assert(strncmp(buffer, data, strlen(data)) == 0);

    pm.free_page(id);
    // Write a lot of data to multiple pages
    constexpr size_t page_count = 512;
    for (size_t i = 0; i < page_count; ++i)
    {
        uint32_t id = pm.allocate_page();

        // Write some data to the page
        ssize_t bytes_written = pm.write(id, 0, data, strlen(data));
        assert(bytes_written != -1);

        // Read the data back from the page
        ssize_t bytes_read = pm.read(id, 0, buffer, strlen(data));
        assert(bytes_read != -1);
        assert(bytes_read == (ssize_t)strlen(data));
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
        5, 3, 1, 2, 7, 6, 8};

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
    for (int i = 0; i < 100; ++i)
        stl_vec.push_back(i);
    for (int i = 0; i < 100; ++i)
        assert(stl_vec[i] == i);
    stl_vec.clear();

    // STLAllocator with map
    Hamster::Map<int, int> stl_map;
    stl_map[1] = 2;
    stl_map[3] = 4;
    assert(stl_map[1] == 2 && stl_map[3] == 4);

    // --- CircularBuffer tests ---
    {
        Hamster::CircularBuffer<int> cb;
        assert(cb.empty());
        assert(cb.size() == 0);

        // Push elements

        // ()

        cb.push(10);
        // (10)
        assert(!cb.empty());
        assert(cb.size() == 1);
        assert(cb.front() == 10);

        cb.push(20);
        // (20, 10)
        assert(cb.size() == 2);
        assert(cb.front() == 20);

        cb.push(30);
        // (30, 10, 20)
        assert(cb.size() == 3);
        assert(cb.front() == 30);

        // Advance and retreat
        cb.advance();
        // (10, 20, 30)
        assert(cb.front() == 10);
        cb.advance();
        // (20, 30, 10)
        assert(cb.front() == 20);
        cb.retreat();
        // (10, 20, 30)
        assert(cb.front() == 10);
        cb.retreat();
        // (30, 10, 20)
        assert(cb.front() == 30);

        // Pop elements
        cb.pop();
        // (10, 20)
        assert(cb.size() == 2);
        assert(cb.front() == 10);
        cb.pop();
        // (20)
        assert(cb.size() == 1);
        assert(cb.front() == 20);
        cb.pop();
        // ()
        assert(cb.size() == 0);
        assert(cb.empty());

        // Test emplace
        cb.emplace(42);
        // (42)
        assert(cb.size() == 1);
        assert(cb.front() == 42);
        cb.pop();
        // ()
        assert(cb.empty());
    }

    // --- PageManager: double free, copy, permissions, swap, dirty, invalid ops ---
    {
        Hamster::PageManager &pm = Hamster::page_manager;
        uint32_t id = pm.allocate_page();

        // Copy page (copy-on-write)
        id = pm.allocate_page();
        uint32_t id2 = pm.copy(id);
        assert(pm.is_id_valid(id2));
        pm.free_page(id);
        pm.free_page(id2);

        // Permissions
        id = pm.allocate_page();
        assert(pm.set_permissions(id, Hamster::PERM_READ) == 0);
        assert(pm.get_permissions(id) == Hamster::PERM_READ);
        pm.free_page(id);

        // Swap in/out, mark dirty
        id = pm.allocate_page();
        assert(pm.swap_in(id) == 0);
        assert(pm.swap_out(id) == 0);
        assert(pm.swap_in(id) == 0);
        pm.mark_page_dirty(id);
        assert(pm.swap_out(id) == 0);
        pm.free_page(id);
    }

    // --- MemorySpace: mapping, unmapping, read/write, protection, edge cases ---
    {
        Hamster::MemorySpace ms;
        constexpr uint32_t page_size = HAMSTER_PAGE_SIZE;
        constexpr uint32_t region_size = page_size * 2;
        uint8_t perms = Hamster::PERM_READ | Hamster::PERM_WRITE;

        // Map anonymous region
        assert(ms.map_anonymous(0, region_size, perms) == 0);
        // Check mapping
        assert(ms.is_mapped(0, region_size) == 1);
        assert(ms.how_many_mapped(0, region_size) == 2);

        // Write to mapped region
        char testdata[] = "testdata";
        assert(ms.memcpy(0, testdata, sizeof(testdata)) == 0);
        char buf[32] = {0};
        assert(ms.memcpy(buf, 0, sizeof(testdata)) == 0);
        assert(strcmp(buf, testdata) == 0);

        // memset
        assert(ms.memset(0, 0xAB, 8) == 0);
        memset(buf, 0, sizeof(buf));
        assert(ms.memcpy(buf, 0, 8) == 0);
        for (int i = 0; i < 8; ++i)
            assert((unsigned char)buf[i] == 0xAB);

        // mprotect
        assert(ms.mprotect(0, page_size, Hamster::PERM_READ) == 0);
        // Unmap
        assert(ms.unmap(0, page_size) == 0);
        assert(ms.is_mapped(0, page_size) == 0);

        // Read from unmapped region (should fail)
        assert(ms.memcpy(buf, 0, 4) != 0);

        // memset_alloc and memcpy_alloc
        assert(ms.memset_alloc(page_size * 10, 0xCD, 4) == 0);
        memset(buf, 0, sizeof(buf));
        assert(ms.memcpy(buf, page_size * 10, 4) == 0);
        for (int i = 0; i < 4; ++i)
            assert((unsigned char)buf[i] == 0xCD);

        // read_until_zero
        char str[] = "abc\0def";
        assert(ms.memcpy_alloc(page_size * 20, str, sizeof(str)) == 0);
        char *out = ms.read_until_zero<char>(page_size * 20);
        assert(out != nullptr && strcmp(out, "abc") == 0);
        Hamster::dealloc(out);

        // Copy constructor/assignment (copy-on-write)
        Hamster::MemorySpace ms2 = ms;
        assert(ms2.is_mapped(page_size * 10, 4) == 1);
        Hamster::MemorySpace ms3;
        ms3 = ms2;
        assert(ms3.is_mapped(page_size * 10, 4) == 1);
    }

    // Page copying
    {
        Hamster::PageManager &pm = Hamster::page_manager;

        // Allocate a page
        uint32_t id = pm.allocate_page();

        uint8_t buf[32];

        // Fill with data
        for (uint8_t i = 0; i < sizeof(buf); ++i)
            buf[i] = i;

        for (uint16_t i = 0; i < (HAMSTER_PAGE_SIZE / sizeof(buf)); ++i)
            pm.write(id, i * sizeof(buf), buf, sizeof(buf));

        // Copy

        uint32_t id2 = pm.copy(id);
        assert(pm.is_id_valid(id2));

        // Check both

        uint8_t buf2[32];
        for (uint16_t i = 0; i < (HAMSTER_PAGE_SIZE / sizeof(buf)); ++i)
        {
            pm.read(id, i * sizeof(buf), buf2, sizeof(buf));
            assert(memcmp(buf, buf2, sizeof(buf)) == 0);

            pm.read(id2, i * sizeof(buf), buf2, sizeof(buf));
            assert(memcmp(buf, buf2, sizeof(buf)) == 0);
        }

        // Write to one

        uint8_t buf3[32];
        for (uint8_t i = 0; i < sizeof(buf); ++i)
            buf3[i] = sizeof(buf) - i;

        for (uint16_t i = 0; i < (HAMSTER_PAGE_SIZE / sizeof(buf)); ++i)
            pm.write(id, i * sizeof(buf), buf3, sizeof(buf));
        
        // Check both again

        for (uint16_t i = 0; i < (HAMSTER_PAGE_SIZE / sizeof(buf)); ++i)
        {
            // Make sure the first page has updated data
            pm.read(id, i * sizeof(buf), buf2, sizeof(buf));
            assert(memcmp(buf3, buf2, sizeof(buf)) == 0);

            // ..and the second page has the original data
            pm.read(id2, i * sizeof(buf), buf2, sizeof(buf));
            assert(memcmp(buf, buf2, sizeof(buf)) == 0);
        }

        pm.free_page(id);
        pm.free_page(id2);
    }

    // Test: Shared anonymous mapping in MemorySpace
    {
        Hamster::MemorySpace ms1;
        Hamster::MemorySpace ms2;
        constexpr uint32_t test_addr = 0x10000;
        constexpr uint32_t test_size = 0x1000;
        constexpr uint8_t perms = Hamster::PERM_READ | Hamster::PERM_WRITE;

        // Map a shared anonymous region in ms1
        int res = ms1.map_shared_anonymous(test_addr, test_size, perms);
        assert(res == 0);
        // Write data to ms1
        uint8_t pattern[test_size];
        for (uint32_t i = 0; i < test_size; ++i) pattern[i] = i % 256;
        res = ms1.memcpy(test_addr, pattern, test_size);
        assert(res == 0);

        // Copy ms1 to ms2 (should share the mapping)
        ms2 = ms1;

        // Read from ms2 and check data matches
        uint8_t readback[test_size];
        res = ms2.memcpy(readback, test_addr, test_size);
        assert(res == 0);
        assert(memcmp(pattern, readback, test_size) == 0);

        // Write new data in ms2
        for (uint32_t i = 0; i < test_size; ++i) readback[i] = 255 - (i % 256);
        res = ms2.memcpy(test_addr, readback, test_size);
        assert(res == 0);

        // Read from ms1 and check it sees the new data (shared)
        uint8_t readback2[test_size];
        res = ms1.memcpy(readback2, test_addr, test_size);
        assert(res == 0);
        assert(memcmp(readback, readback2, test_size) == 0);
    }

    // Test MemorySpace with big data
    {
// Warning: takes a while
#if 0
        constexpr uint32_t size = 16 * 1024 * 1024;
        Hamster::MemorySpace ms;

        ms.map_anonymous(0, size, Hamster::PERM_READ | Hamster::PERM_WRITE);
        ms.memset(0, 0, size);
        ms.memset(0, 0xFF, size);

        for (uint16_t i = 0; i < 8192; ++i)
            ms.memset(0, i % 256, size);
#endif
    }

    // Test Hamster::SharedPtr
    {
        // Basic construction
        Hamster::SharedPtr<int> sp1 = sp1.make_shared(123);
        assert(*sp1 == 123);
        // Copy (deep by default)
        Hamster::SharedPtr<int> sp2 = sp1;
        assert(*sp2 == 123);
        *sp2 = 456;
        // Deep copy: changing sp2 does not affect sp1
        assert(*sp1 == 123);
        assert(*sp2 == 456);

        // Shallow copy
        Hamster::SharedPtr<int, size_t, Hamster::SharedPtrCopyType::SHALLOW> sp3 = sp3.make_shared(789);
        Hamster::SharedPtr<int, size_t, Hamster::SharedPtrCopyType::SHALLOW> sp4 = sp3;
        assert(*sp3 == 789);
        assert(*sp4 == 789);
        *sp4 = 321;
        // Shallow copy: changing sp4 affects sp3
        assert(*sp3 == 321);
        assert(*sp4 == 321);

        // Move semantics
        Hamster::SharedPtr<int> sp5 = std::move(sp2);
        assert(*sp5 == 456);
        assert(!sp2);

        // Test with a struct
        struct Point { int x, y; };
        Hamster::SharedPtr<Point> p1 = p1.make_shared(1, 2);
        assert(p1->x == 1 && p1->y == 2);
        Hamster::SharedPtr<Point> p2 = p1;
        p2->x = 10;
        assert(p1->x == 1); // deep copy
        assert(p2->x == 10);

        Hamster::SharedPtr<float> f1 = f1.make_shared();

        *f1 = 123.456f;
        assert(*f1 == 123.456f);

        {
            Hamster::SharedPtr<float> f2{f1, Hamster::SharedPtrCopyType::SHALLOW};
            *f2 = 234.567f;

            assert(*f1 == 234.567f);
            assert(*f2 == 234.567f);
        }

        assert(*f1 == 234.567f);
    }

    // Test futexes
    {
        Hamster::MemorySpace ms;

        // static to be accessible within lambda
        static int i = 0;
        static Hamster::MemorySpace *p_ms;

        p_ms = &ms;

        assert(ms.map_anonymous(0, HAMSTER_PAGE_SIZE, Hamster::PERM_READ | Hamster::PERM_WRITE) == 0);

        ms.futex_wait(0, []{
            uint32_t word;
            assert(p_ms && p_ms->memcpy(&word, 0, 4) == 0);
            assert(word == 0x1234);
            i = 0x4321;
        });

        assert(i == 0);

        uint32_t word = 0x1234;
        assert(ms.memcpy(0, &word, 4) == 0);

        assert(ms.futex_wake(0, 1) == 1);

        assert(i == 0x4321);
    }

    // Futexes with bitsets
    {
        Hamster::MemorySpace ms;

        int i = 0;

        assert(ms.map_anonymous(0, HAMSTER_PAGE_SIZE, Hamster::PERM_READ | Hamster::PERM_WRITE) == 0);

        ms.futex_wait(0, [](void *data){
            *(int *)data = 0x1234;
        }, &i, 1 << 0);

        ms.futex_wait(0, [](void *data){
            *(int *)data = 0x2345;
        }, &i, 1 << 1);

        assert(ms.futex_wake(0, UINT32_MAX, 1 << 3) == 0);
        assert(i == 0);
        assert(ms.futex_wake(0, 1, 1 << 1) == 1);
        assert(ms.futex_wake(0, 1, 1 << 1) == 0);
        assert(i == 0x2345);
        assert(ms.futex_wake(0, 1) == 1);
        assert(i == 0x1234);
    }
}

#endif // NDEBUG
