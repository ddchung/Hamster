// Paging system

#pragma once

#include <cstdint>
#include <cstddef>

namespace Hamster
{
    class Page
    {
    public:
        // all-zero default ctor
        Page();

        // takes ownership of data
        Page(uint8_t *data);

        // takes ownership of a swap page
        Page(int swap_index);

        Page(const Page&) = delete;
        Page &operator=(const Page&) = delete;

        Page(Page&&);
        Page &operator=(Page&&);

        ~Page();

        inline int get_swap_index() const { return swap_idx; }
        inline bool is_swapped() const { return swapped; }
        inline bool is_locked() const { return locked; }

        inline int lock() { locked = true; return 0; }
        inline int unlock() { locked = false; return 0; }

        int swap_in();
        int swap_out();

        // Get a reference to an element at a given index
        // Returns a reference to a dummy if swapped out
        uint8_t &operator[](size_t idx);

        // Get a reference to said dummy
        // see operator[]
        static uint8_t &get_dummy();

    private:
        // data
        // not allocated when swapped out
        uint8_t *data;

        // randomized swap index
        int swap_idx;

        // flags
        bool swapped : 1;
        bool locked : 1;

        void allocate();
        void deallocate();
        void gen_id();
    };
} // namespace Hamster

