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
