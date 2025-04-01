// memory space made of pages

#pragma once

#include <memory/page.hpp>
#include <cstdint>
#include <unordered_map>
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

        template <typename R, typename... Args>
        void do_action_on_pages(R (Page::*action)(Args...), Args&&... args)
        {
            for (auto &page : pages)
            {
                (page.second.*action)(std::forward<Args>(args)...);
            }
        }

        inline void swap_out_pages() { do_action_on_pages(&Page::swap_out); }
        inline void swap_in_pages() { do_action_on_pages(&Page::swap_in); }

        // get a specific byte of data
        // returns a reference to the byte
        // or a reference to the page dummy on error
        uint8_t &operator[](uint64_t addr);

        uint8_t *get_page_data(uint64_t addr);

        static uint64_t get_addr_offset(uint64_t addr);
        static uint64_t get_page_start(uint64_t addr);
    private:
        std::unordered_map<uint64_t, Page> pages;
    };
} // namespace Hamster

