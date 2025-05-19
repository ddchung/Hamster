// page implementation

#include <memory/page.hpp>
#include <memory/page_manager.hpp>

namespace Hamster
{
    Page::Page()
    {
        page_id = page_manager.open_page();
    }

    Page::Page(Page &&other)
    {
        page_id = other.page_id;
        other.page_id = -1;
    }

    Page &Page::operator=(Page &&other)
    {
        if (this != &other)
        {
            page_id = other.page_id;
            other.page_id = -1;
        }
        return *this;
    }

    Page::~Page()
    {
        if (page_id >= 0)
        {
            page_manager.close_page(page_id);
        }
        page_id = -1;
    }

    bool Page::is_swapped()
    {
        return page_manager.is_swapped(page_id);
    }

    uint8_t *Page::get_data()
    {
        return page_manager.get_data(page_id);
    }

    int Page::swap_in()
    {
        return page_manager.swap_in(page_id);
    }

    int Page::swap_out()
    {
        return page_manager.swap_out(page_id);
    }

    uint8_t &Page::operator[](size_t index)
    {
        return page_manager.get_byte(page_id, index);
    }

    uint8_t &Page::get_dummy_byte()
    {
        return page_manager.get_byte_dummy();
    }

    uint16_t &Page::get_flags()
    {
        return page_manager.get_flags(page_id);
    }
} // namespace Hamster

