// Hamster page table

#pragma once

#include <platform/config.hpp>
#include <cstdint>
#include <cstddef>

namespace Hamster
{
    class PageTable
    {
        static constexpr uint32_t LEAF_NUM_PAGES = 1 << HAMSTER_PAGETABLE_LEAF_BITS;
        static constexpr uint32_t L2_NUM_ENTRIES = 1 << HAMSTER_PAGETABLE_L2_BITS;
        static constexpr uint32_t L1_NUM_ENTRIES = 1 << HAMSTER_PAGETABLE_L1_BITS;

        static_assert(
            (uint64_t)HAMSTER_PAGE_SIZE * LEAF_NUM_PAGES * L2_NUM_ENTRIES * L1_NUM_ENTRIES == 4294967296,
            "Invalid page table configuration"
        );

        struct LeafEntry
        {
            uint32_t page_ids[LEAF_NUM_PAGES];
            uint8_t refcount;
        };

        struct L2Table
        {
            LeafEntry *entries[L2_NUM_ENTRIES];
            uint8_t refcount;
        };

        struct L1Table
        {
            L2Table *tables[L1_NUM_ENTRIES];
            uint8_t refcount;
        };

        struct LeafCache
        {
            LeafEntry *leaf;
            uint32_t leaf_start;
        };

    public:
        static constexpr uint32_t PAGE_ID_UNUSED = 0xFFFFFFFF;

        PageTable();
        ~PageTable();
        PageTable(const PageTable &other);
        PageTable &operator=(const PageTable &other);
        PageTable(PageTable &&other);
        PageTable &operator=(PageTable &&other);

        /**
         * @brief Get a page ID of a certain address, for reading only
         * @param address The virtual address to translate
         * @return The corresponding page ID, or PAGE_ID_UNUSED if not found
         */
        uint32_t get_page_read(uint32_t address) const;

        /**
         * @brief Get a page ID of a certain address, for writing
         * @param address The virtual address to translate
         * @return The page ID, or PAGE_ID_UNUSED if not found
         * @note This differs from `get_page_read` because it may split
         *     * copy-on-write pages
         */
        uint32_t get_page_write(uint32_t address);

        /**
         * @brief Set a page ID for a certain address
         * @param address The virtual address to set
         * @param page_id The page ID to set
         * @note `page_id` may be PAGE_ID_UNUSED, to indicate unmapping
         * @warning This takes ownership of the page ID
         */
        void set_page(uint32_t address, uint32_t page_id);

        /**
         * @brief Clears all mappings
         */
        void clear();

        /**
         * @brief Sets the cache to look at a certain address
         * @param index The index of the cache to set. 0 or 1.
         * @param address The virtual address to set the cache for
         */
        void set_cache(uint32_t index, uint32_t address);

    private:
        L1Table *root;
        LeafCache cache[2] = {};

        // These functions allocate new parts of the table,
        // potentially copying existing entries

        L1Table *make_l1(L1Table *old_l1 = nullptr);
        L2Table *make_l2(L2Table *old_l2 = nullptr);
        LeafEntry *make_leaf(LeafEntry *old_leaf = nullptr);

        L1Table *replace_l1(L1Table *old_l1);
        L2Table *replace_l2(L2Table *old_l2);
        LeafEntry *replace_leaf(LeafEntry *old_leaf);

        void destroy_l1(L1Table *l1);
        void destroy_l2(L2Table *l2);

        // note: all pages in the leaf must be gone before this is called
        void destroy_leaf(LeafEntry *leaf);

        // Invalidate all caches
        void invalidate_caches();

        // Invalidate all caches in an L2 table
        void invalidate_caches(uint32_t l2_addr);

        // Invalidate all caches pointing to a certain leaf
        void invalidate_caches(LeafEntry *leaf);
    };
} // namespace Hamster

