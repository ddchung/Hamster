// implementation of page.hpp

#include <memory/page.hpp>
#include <memory/allocator.hpp>
#include <platform/config.hpp>
#include <platform/platform.hpp>
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <utility>

#include <cstdio>

static uint8_t dummy;

namespace Hamster
{
    void Page::gen_id()
    {
        swap_idx = rand();
    }

    void Page::deallocate()
    {
        dealloc<uint8_t, HAMSTER_PAGE_SIZE>(data);
        data = nullptr;
    }

    void Page::allocate()
    {
        deallocate();
        data = alloc<uint8_t, HAMSTER_PAGE_SIZE>(0);
    }

    Page::Page()
        : data(nullptr), swap_idx(-1), swapped(false), locked(false)
    {
        allocate();
        gen_id();
    }

    Page::Page(uint8_t *data)
        : data(data), swap_idx(-1), swapped(false), locked(false)
    {
        assert(data != nullptr);
        gen_id();
    }

    Page::Page(int swap_idx)
        : data(nullptr), swap_idx(swap_idx), swapped(true), locked(false)
    {
    }

    Page::Page(Page &&other)
        : data(other.data), swap_idx(other.swap_idx), swapped(other.swapped),
          locked(other.locked)
    {
        other.data = nullptr;
        other.swap_idx = -1;
        other.swapped = false;
        other.locked = false;
    }

    Page &Page::operator=(Page &&other)
    {
        std::swap(*this, other);
        return *this;
    }
    
    Page::~Page()
    {
        deallocate();
    }

    int Page::swap_in()
    {
        if (!data)
            allocate();
        if (locked)
            return -1;
        if (_swap_in(swap_idx, data) < 0)
            return -2;
        swapped = false;
        return 0;
    }

    int Page::swap_out()
    {
        if (swapped)
            return 0;
        if (locked)
            return -1;
        if (!data)
            return -2;
        if (_swap_out(swap_idx, data) < 0)
            return -3;
        deallocate();
        swapped = true;
        return 0;
    }

    uint8_t &Page::get_dummy()
    {
        return dummy;
    }

    uint8_t &Page::operator[](size_t index)
    {
        if (swapped || !data || index >= HAMSTER_PAGE_SIZE)
            return get_dummy();
        return data[index];
    }
}
