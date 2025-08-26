// Page table

#include <memory/page_table.hpp>
#include <memory/allocator.hpp>
#include <memory/page_manager.hpp>
#include <cassert>
#include <cstring>

#define PAGE_TABLE_INDEX(addr) ((addr) >> (HAMSTER_PAGE_SIZE_BITS))

namespace Hamster
{
    PageTable::PageTable()
    {
        memset(page_ids, 0xFF, sizeof(page_ids));
    }

    PageTable::~PageTable()
    {
        clear();
    }

    PageTable::PageTable(const PageTable &other)
        : PageTable()
    {
        for (size_t i = 0; i < HAMSTER_PAGES_PER_PROC; i++)
        {
            if (other.page_ids[i] != PAGE_ID_UNUSED)
                page_ids[i] = page_manager.copy(other.page_ids[i]);
        }
    }

    PageTable &PageTable::operator=(const PageTable &other)
    {
        if (this != &other)
        {
            clear();
            for (size_t i = 0; i < HAMSTER_PAGES_PER_PROC; i++)
            {
                if (other.page_ids[i] != PAGE_ID_UNUSED)
                    page_ids[i] = page_manager.copy(other.page_ids[i]);
            }
        }
        return *this;
    }

    PageTable::PageTable(PageTable &&other)
        : page_ids()
    {
        for (size_t i = 0; i < HAMSTER_PAGES_PER_PROC; i++)
        {
            page_ids[i] = other.page_ids[i];
            other.page_ids[i] = PAGE_ID_UNUSED;
        }
    }

    PageTable &PageTable::operator=(PageTable &&other)
    {
        if (this != &other)
        {
            for (size_t i = 0; i < HAMSTER_PAGES_PER_PROC; i++)
                std::swap(page_ids[i], other.page_ids[i]);
        }
        return *this;
    }

    uint32_t PageTable::get_page(uint32_t address) const
    {
        uint32_t index = PAGE_TABLE_INDEX(address);
        assert(index < HAMSTER_PAGES_PER_PROC);
        return page_ids[index];
    }

    uint32_t PageTable::get_page_direct(uint32_t index) const
    {
        assert(index < HAMSTER_PAGES_PER_PROC);
        return page_ids[index];
    }

    void PageTable::set_page(uint32_t address, uint32_t page_id)
    {
        uint32_t index = PAGE_TABLE_INDEX(address);
        set_page_direct(index, page_id);
    }

    void PageTable::set_page_direct(uint32_t index, uint32_t page_id)
    {
        assert(index < HAMSTER_PAGES_PER_PROC);
        assert(page_id == PAGE_ID_UNUSED || page_id < 0xFFFF); // uint16_t max, but page_manager still uses old ID type (uint32_t)

        uint16_t &id = page_ids[index];

        if (id != PAGE_ID_UNUSED)
            page_manager.free_page(id);
        id = page_id;
    }

    void PageTable::clear()
    {
        for (uint16_t &id : page_ids)
        {
            if (id != PAGE_ID_UNUSED)
            {
                page_manager.free_page(id);
                id = PAGE_ID_UNUSED;
            }
        }
    }
} // namespace Hamster
