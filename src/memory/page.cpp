// page implementation

#include <memory/page.hpp>
#include <memory/page_manager.hpp>

namespace Hamster
{
    Page::Page()
    {
        page_id = PageManager::instance().open_page();
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
            PageManager::instance().close_page(page_id);
        }
        page_id = -1;
    }

    bool Page::is_swapped()
    {
        return PageManager::instance().is_swapped(page_id);
    }

    uint8_t *Page::get_data()
    {
        return PageManager::instance().get_data(page_id);
    }

    int Page::swap_in()
    {
        return PageManager::instance().swap_in(page_id);
    }

    int Page::swap_out()
    {
        return PageManager::instance().swap_out(page_id);
    }

    uint8_t &Page::operator[](size_t index)
    {
        return PageManager::instance().get_byte(page_id, index);
    }

    uint8_t &Page::get_dummy_byte()
    {
        return PageManager::instance().get_byte_dummy();
    }

    uint16_t &Page::get_flags()
    {
        return PageManager::instance().get_flags(page_id);
    }
} // namespace Hamster

