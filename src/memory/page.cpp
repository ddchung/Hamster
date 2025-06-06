// page implementation

#include <memory/page.hpp>
#include <memory/page_manager.hpp>
#include <memory/allocator.hpp>
#include <cstring>

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

    Page::Page(const Page &other)
    {
        page_id = page_manager.open_page();
        if (page_id < 0)
            return;
        
        // Flags
        get_flags() = other.get_flags();
        
        if (other.is_swapped())
        {
            // If the other page is swapped out, we need to swap it in
            if (other.swap_in() < 0)
            {
                page_manager.close_page(page_id);
                page_id = -1;
                return;
            }
        }
        if (is_swapped())
        {
            // If this page is swapped out, we need to swap it in
            if (swap_in() < 0)
            {
                page_manager.close_page(page_id);
                page_id = -1;
                return;
            }
        }

        // Copy data
        uint8_t *this_data = get_data();
        const uint8_t *other_data = other.get_data();
        if (this_data && other_data)
            memcpy(this_data, other_data, HAMSTER_PAGE_SIZE);
        else if (this_data)
            memset(this_data, 0, HAMSTER_PAGE_SIZE);
        else
        {
            page_manager.close_page(page_id);
            page_id = -1;
        }
    }

    Page &Page::operator=(const Page &other)
    {
        if (this != &other)
        {
            // Close the current page if it is open
            if (page_id >= 0)
            {
                page_manager.close_page(page_id);
            }

            // Open a new page
            page_id = page_manager.open_page();
            if (page_id < 0)
                return *this;

            // Copy flags
            get_flags() = other.get_flags();

            if (other.is_swapped())
            {
                // If the other page is swapped out, we need to swap it in
                if (other.swap_in() < 0)
                {
                    page_manager.close_page(page_id);
                    page_id = -1;
                    return *this;
                }
            }
            if (is_swapped())
            {
                // If this page is swapped out, we need to swap it in
                if (swap_in() < 0)
                {
                    page_manager.close_page(page_id);
                    page_id = -1;
                    return *this;
                }
            }

            // Copy data
            uint8_t *this_data = get_data();
            const uint8_t *other_data = other.get_data();
            if (this_data && other_data)
                memcpy(this_data, other_data, HAMSTER_PAGE_SIZE);
            else if (this_data)
                memset(this_data, 0, HAMSTER_PAGE_SIZE);
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

    bool Page::is_swapped() const
    {
        return page_manager.is_swapped(page_id);
    }

    uint8_t *Page::get_data() const
    {
        return page_manager.get_data(page_id);
    }

    int Page::swap_in() const
    {
        return page_manager.swap_in(page_id);
    }

    int Page::swap_out() const
    {
        return page_manager.swap_out(page_id);
    }

    uint8_t &Page::operator[](size_t index) const
    {
        return page_manager.get_byte(page_id, index);
    }

    uint8_t &Page::get_dummy_byte()
    {
        return page_manager.get_byte_dummy();
    }

    uint16_t &Page::get_flags() const
    {
        return page_manager.get_flags(page_id);
    }
} // namespace Hamster

