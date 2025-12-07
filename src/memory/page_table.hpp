// Hamster page table

#pragma once

#include <platform/config.hpp>
#include <memory/page_manager.hpp>
#include <cstdint>
#include <cstddef>
#include <cassert>

#define PAGE_TABLE_INDEX(addr) ((addr) >> (HAMSTER_PAGE_SIZE_BITS))

namespace Hamster
{
    class PageTable
    {
    public:
        static constexpr uint16_t PAGE_ID_UNUSED = 0xFFFF;

        PageTable();
        ~PageTable();
        PageTable(const PageTable &other);
        PageTable &operator=(const PageTable &other);
        PageTable(PageTable &&other);
        PageTable &operator=(PageTable &&other);

        /**
         * @brief Get a page ID of a certain address
         * @param address The virtual address to translate
         * @return The corresponding page ID, or PAGE_ID_UNUSED if not found
         */
        uint32_t get_page(uint32_t address) const
        {
            uint32_t index = PAGE_TABLE_INDEX(address);
            if (index >= HAMSTER_PAGES_PER_PROC)
                return PAGE_ID_UNUSED;
            return page_ids[index];
        }

        /**
         * @brief Get a page ID directly
         * @param index The index of the page to get
         * @return The corresponding page ID, or PAGE_ID_UNUSED if not found
         * @note `index = address >> HAMSTER_PAGE_SIZE_BITS`
         */
        uint32_t get_page_direct(uint32_t index) const
        {
            assert(index < HAMSTER_PAGES_PER_PROC);
            return page_ids[index];
        }

        /**
         * @brief Set a page ID for a certain address
         * @param address The virtual address to set
         * @param page_id The page ID to set
         * @note `page_id` may be PAGE_ID_UNUSED, to indicate unmapping
         * @warning This takes ownership of the page ID
         */
        void set_page(uint32_t address, uint32_t page_id)
        {
            uint32_t index = PAGE_TABLE_INDEX(address);
            set_page_direct(index, page_id);
        }

        /**
         * @brief Set a page ID directly
         * @param index The index of the page
         * @note See: `get_page_direct` for info on how `index` works
         */
        void set_page_direct(uint32_t index, uint32_t page_id)
        {
            assert(index < HAMSTER_PAGES_PER_PROC);
            assert(page_id == PAGE_ID_UNUSED || page_id < 0xFFFF); // uint16_t max, but page_manager still uses old ID type (uint32_t)

            uint16_t &id = page_ids[index];

            if (id != PAGE_ID_UNUSED)
                page_manager.free_page(id);
            id = page_id;
        }

        /**
         * @brief Clear all pages
         */
        void clear();

    private:
        uint16_t page_ids[HAMSTER_PAGES_PER_PROC];
    };
} // namespace Hamster

#undef PAGE_TABLE_INDEX

