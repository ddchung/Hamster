// implementation of page manager

#include <memory/page_manager.hpp>
#include <memory/allocator.hpp>
#include <platform/platform.hpp>
#include <platform/config.hpp>
#include <utility>

namespace Hamster
{
    PageManager::PageImpl::PageImpl()
        : data(nullptr), flags(0), used(false), swapped(false)
    {
        allocate();
    }

    PageManager::PageImpl::~PageImpl()
    {
        deallocate();
    }

    PageManager::PageImpl::PageImpl(PageImpl &&other)
        : data(other.data), flags(other.flags), used(other.used), swapped(other.swapped)
    {
        other.data = nullptr;
        other.used = false;
        other.swapped = true;
    }

    PageManager::PageImpl &PageManager::PageImpl::operator=(PageImpl &&other)
    {
        if (this != &other)
        {
            std::swap(data, other.data);
            std::swap(flags, other.flags);

            bool tmp = used;
            used = other.used;
            other.used = tmp;

            tmp = swapped;
            swapped = other.swapped;
            other.swapped = tmp;
        }
        return *this;
    }

    void PageManager::PageImpl::allocate()
    {
        deallocate();
        data = alloc<uint8_t>(HAMSTER_PAGE_SIZE);
    }

    void PageManager::PageImpl::deallocate()
    {
        dealloc(data);
        data = nullptr;
    }

    int PageManager::PageImpl::swap_in(int swp_idx)
    {   
        allocate();

        int ret = _swap_in(swp_idx, data);

        if (ret < 0)
        {
            deallocate();
            return ret;
        }

        swapped = false;
        return 0;
    }

    int PageManager::PageImpl::swap_out(int swp_idx)
    {
        if (!data)
            return -1;
        int ret = _swap_out(swp_idx, data);
        if (ret < 0)
            return ret;
        deallocate();
        swapped = true;
        return 0;
    }

    int PageManager::swap_out(int32_t page)
    {
        if (page < 0 || (size_t)page >= pages.size())
            return -1;

        return pages[page].swap_out(page);
    }

    int PageManager::swap_in(int32_t page)
    {
        if (page < 0 || (size_t)page >= pages.size())
            return -1;

        return pages[page].swap_in(page);
    }

    bool PageManager::is_swapped(int32_t page)
    {
        if (page < 0 || (size_t)page >= pages.size())
            return false;

        return pages[page].is_swapped();
    }

    int32_t PageManager::open_page()
    {
        for (size_t i = 0; i < pages.size(); ++i)
        {
            if (!pages[i].is_used())
            {
                pages[i].set_used(true);
                return i;
            }
        }

        if (pages.size() >= HAMSTER_MAX_PAGES)
            // out of memory
            return -1;
        
        // no free page
        pages.emplace_back();
        pages.back().set_used(true);
        return pages.size() - 1;
    }

    int PageManager::close_page(int32_t page)
    {
        if (page < 0 || (size_t)page >= pages.size())
            return -1;

        pages[page].set_used(false);
        return 0;
    }

    int PageManager::set_byte(int32_t page, uint32_t offset, uint8_t value)
    {
        if (page < 0 || (size_t)page >= pages.size())
            return -1;
        if (offset + 1 > HAMSTER_PAGE_SIZE)
            return -2;
        
        PageImpl &p = pages[page];

        if (p.is_swapped())
            return -3;
        if (!p.is_used())
            return -4;
        
        p.get_data()[offset] = value;
        return 0;
    }

    static uint8_t byte_dummy;

    uint8_t &PageManager::get_byte(int32_t page, uint32_t offset)
    {
        if (page < 0 || (size_t)page >= pages.size())
            return byte_dummy;
        if (offset + 1 > HAMSTER_PAGE_SIZE)
            return byte_dummy;

        PageImpl &p = pages[page];

        if (p.is_swapped())
            return byte_dummy;
        if (!p.is_used())
            return byte_dummy;

        return p.get_data()[offset];
    }

    uint8_t &PageManager::get_byte_dummy()
    {
        return byte_dummy;
    }

    static uint16_t flag_dummy;

    uint16_t &PageManager::get_flags(int32_t page)
    {
        if (page < 0 || (size_t)page >= pages.size())
            return flag_dummy;
        return pages[page].get_flags();
    }

    uint16_t &PageManager::get_flag_dummy()
    {
        return flag_dummy;
    }

    uint8_t *PageManager::get_data(int32_t page)
    {
        if (page < 0 || (size_t)page >= pages.size())
            return nullptr;

        PageImpl &p = pages[page];

        if (p.is_swapped())
            return nullptr;
        if (!p.is_used())
            return nullptr;

        return p.get_data();
    }
} // namespace Hamster

