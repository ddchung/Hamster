// page management

#pragma once

#include <cstdint>
#include <memory/stl_sequential.hpp>

namespace Hamster
{
    class PageManager
    {
        PageManager() = default;
        ~PageManager() = default;
        PageManager(const PageManager&) = delete;
        PageManager& operator=(const PageManager&) = delete;
        PageManager(PageManager&&) = delete;
        PageManager& operator=(PageManager&&) = delete;

        class PageImpl
        {
        public:
            PageImpl();
            ~PageImpl();

            PageImpl(const PageImpl&) = delete;
            PageImpl& operator=(const PageImpl&) = delete;

            PageImpl(PageImpl&&);
            PageImpl& operator=(PageImpl&&);

            int swap_in(int swp_idx);
            int swap_out(int swp_idx);

            // returns nullptr if page is swapped out
            uint8_t *get_data() { return data; }

            bool is_used() const { return used; }
            bool is_swapped() const { return swapped; }

            void set_used(bool u) { used = u; }

            uint16_t &get_flags() { return flags; }

        private:
            uint8_t *data;
            uint16_t flags;
            bool used : 1;
            bool swapped : 1;

            void allocate();
            void deallocate();
        };

    public:
        static PageManager &instance()
        {
            static PageManager instance;
            return instance;
        }

        // swap out a page
        // returns 0 on success, <0 on error
        int swap_out(int32_t page);

        // swap in a page
        // returns 0 on success, <0 on error
        int swap_in(int32_t page);

        // check if a page is swapped out
        bool is_swapped(int32_t page);

        // open a new page
        // returns the page id, or
        // negative value on error
        int32_t open_page();

        // close a page
        // returns it to the pool
        // =0 success, <0 error
        int close_page(int32_t page);

        // set a byte on a page
        // =0 success, <0 error
        int set_byte(int32_t page, uint32_t offset, uint8_t value);

        // get a reference to a byte
        // returns a reference to a dummy on error
        // the reference is valid until the page is swapped out or closed
        // and it will not re-validate ever again
        uint8_t &get_byte(int32_t page, uint32_t offset);

        // get a reference to the byte dummy
        // see get_byte_ref()
        static uint8_t &get_byte_dummy();

        // get the page flags
        // reference returned is always valid
        // until the page is closed
        // returns a reference to a dummy on error
        //
        // page flags are not touched by this manager,
        // the user can do anything with it
        uint16_t &get_flags(int32_t page);

        // get the page flag dummy
        // see get_flags()
        static uint16_t &get_flag_dummy();

        // get the page data
        // returns nullptr if the page is swapped out
        uint8_t *get_data(int32_t page);
        
    private:
        Vector<PageImpl> pages;
    };
} // namespace Hamster
