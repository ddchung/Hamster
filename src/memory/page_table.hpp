// Hamster page table

#pragma once

#include <platform/config.hpp>
#include <cstdint>
#include <cstddef>

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
        uint32_t get_page(uint32_t address) const;

        /**
         * @brief Get a page ID directly
         * @param index The index of the page to get
         * @return The corresponding page ID, or PAGE_ID_UNUSED if not found
         * @note `index = address >> HAMSTER_PAGE_SIZE_BITS`
         */
        uint32_t get_page_direct(uint32_t index) const;

        /**
         * @brief Set a page ID for a certain address
         * @param address The virtual address to set
         * @param page_id The page ID to set
         * @note `page_id` may be PAGE_ID_UNUSED, to indicate unmapping
         * @warning This takes ownership of the page ID
         */
        void set_page(uint32_t address, uint32_t page_id);

        /**
         * @brief Set a page ID directly
         * @param index The index of the page
         * @note See: `get_page_direct` for info on how `index` works
         */
        void set_page_direct(uint32_t index, uint32_t page_id);

        /**
         * @brief Clear all pages
         */
        void clear();

    private:
        uint16_t page_ids[HAMSTER_PAGES_PER_PROC];
    };
} // namespace Hamster

