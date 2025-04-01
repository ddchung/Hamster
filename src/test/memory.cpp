// test memory

#include <memory/allocator.hpp>
#include <memory/page.hpp>
#include <platform/platform.hpp>
#include <cassert>

void test_memory()
{
    int i;

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
    Hamster::dealloc<int, 10>(arr);

    // Test default constructor
    {
        Hamster::Page page;
        assert(!page.is_swapped());
        assert(!page.is_locked());
    }

    // Test constructor with data
    {
        uint8_t *test_data = Hamster::alloc<uint8_t, HAMSTER_PAGE_SIZE>(0);
        Hamster::Page page(test_data);
        assert(!page.is_swapped());
        assert(!page.is_locked());
    }

    // Test constructor with swap index
    {
        Hamster::Page page(42);
        assert(page.is_swapped());
        assert(!page.is_locked());
        assert(page.get_swap_index() == 42);
    }

    // Test move constructor
    {
        Hamster::Page page1;
        Hamster::Page page2(std::move(page1));
        assert(!page2.is_swapped());
        assert(!page2.is_locked());
    }

    // Test move assignment
    {
        Hamster::Page page1;
        Hamster::Page page2 = std::move(page1);
        assert(!page2.is_swapped());
        assert(!page2.is_locked());
    }

    // Test lock and unlock
    {
        Hamster::Page page;
        page.lock();
        assert(page.is_locked());
        page.unlock();
        assert(!page.is_locked());
    }

    // Test swap_in and swap_out
    {
        uint8_t data[HAMSTER_PAGE_SIZE] = {0};
        Hamster::_swap_out(42, data);

        Hamster::Page page(42);
        assert(page.is_swapped());
        page.swap_in();
        assert(!page.is_swapped());
        page.swap_out();
        assert(page.is_swapped());
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
}
