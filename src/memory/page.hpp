// page

#pragma once

#include <cstdint>
#include <cstddef>
#include <memory/page_manager.hpp>

namespace Hamster
{
    class Page
    {
    public:
        // default zero-initialized page
        Page();

        Page(const Page &) = delete;
        Page &operator=(const Page &) = delete;

        Page(Page &&);
        Page &operator=(Page &&);

        ~Page();


        inline int get_page_id() const { return page_id; }
        
        [[deprecated]] inline int get_swap_index() const { return page_id; }

        // check if the page is swapped out
        bool is_swapped();

        // get the page data
        // returns null if the page is swapped out
        uint8_t *get_data();

        // swap in the page from the swap space
        // =0 ok <0 error
        int swap_in();

        // swap out the page to the swap space
        // =0 ok <0 error
        int swap_out();

        // returns a reference to a byte in the page
        // or a dummy byte if the page is swapped out
        //
        // Note: it is recommended to not keep the reference for long
        // as it is *very* easily invalidated
        uint8_t &operator[](size_t index);

        // get the dummy byte
        // see operator[] for details
        static uint8_t &get_dummy_byte();
        
        inline static uint8_t &get_dummy()
        { return get_dummy_byte(); }

        uint16_t &get_flags();

    private:
        int32_t page_id;
    };
} // namespace Hamster

