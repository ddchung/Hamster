// test memory

#include <memory/allocator.hpp>
#include <memory/page.hpp>
#include <platform/platform.hpp>
#include <cassert>
#include <iostream>

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

    std::cout << "testing page" << std::endl;

    // Test default constructor
    {
        Hamster::Page page;
        assert(!page.is_swapped());
        assert(!page.is_locked());
    }

    std::cout << "1" << std::endl;

    // Test constructor with data
    {
        uint8_t *test_data = Hamster::alloc<uint8_t, HAMSTER_PAGE_SIZE>(0);
        Hamster::Page page(test_data);
        assert(!page.is_swapped());
        assert(!page.is_locked());
    }

    std::cout << "2" << std::endl;

    // Test constructor with swap index
    {
        Hamster::Page page(42);
        assert(page.is_swapped());
        assert(!page.is_locked());
        assert(page.get_swap_index() == 42);
    }

    std::cout << "3" << std::endl;

    // Test move constructor
    {
        Hamster::Page page1;
        Hamster::Page page2(std::move(page1));
        assert(!page2.is_swapped());
        assert(!page2.is_locked());
    }

    std::cout << "4" << std::endl;

    // Test move assignment
    {
        Hamster::Page page1;
        Hamster::Page page2 = std::move(page1);
        assert(!page2.is_swapped());
        assert(!page2.is_locked());
    }

    std::cout << "5" << std::endl;

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
}
