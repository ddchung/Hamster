// memory space made of pages

#pragma once

#include <memory/page.hpp>
#include <memory/stl_map.hpp>
#include <memory/stl_sequential.hpp>
#include <cstdint>
#include <utility>

namespace Hamster
{
    class MemorySpace
    {
    public:
        MemorySpace() = default;

        MemorySpace(const MemorySpace &) = delete;
        MemorySpace &operator=(const MemorySpace &) = delete;

        MemorySpace(MemorySpace &&);
        MemorySpace &operator=(MemorySpace &&);

        ~MemorySpace() = default;

        // allocate a page
        // returns the page id, or negative error code
        int allocate_page(uint64_t addr);

        // deallocate a page
        // returns 0 on success, or negative error code
        int deallocate_page(uint64_t addr);

        int swap_out_pages();
        int swap_in_pages();

        // get a specific byte of data
        // returns a reference to the byte
        // or a reference to the page dummy on error
        uint8_t &operator[](uint64_t addr);

        uint8_t *get_page_data(uint64_t addr);

        static uint64_t get_addr_offset(uint64_t addr);
        static uint64_t get_page_start(uint64_t addr);

        // Check if a page is allocated
        // returns true if the page is allocated
        // false if the page is not allocated
        bool is_page_allocated(uint64_t addr);

        // Check if a range of pages is allocated
        // returns true if all the pages within the range are allocated
        // false if any of the pages within the range are not allocated
        bool is_page_range_allocated(uint64_t addr, size_t size);

        int memcpy(uint64_t dest, uint64_t src, size_t size);
        int memcpy(uint64_t dest, uint8_t *src, size_t size);
        int memcpy(uint8_t *dest, uint64_t src, size_t size);

        int memset(uint64_t dest, uint8_t value, size_t size);

        uint16_t &get_page_flags(uint64_t addr);
    private:
        UnorderedMap<uint64_t, Page> pages;
        List<uint64_t> swapped_on_pages;

        // Queues in a page to be swapped out later
        // errors if page is locked
        int queue(uint64_t page);

        // If swapped_on_pages.size() > HAMSTER_CONCUR_PAGES
        // then pop and swap out trailing pages
        int clean();
    };
} // namespace Hamster

