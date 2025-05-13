// Memory Space Implementation

#include <memory/memory_space.hpp>
#include <memory/allocator.hpp>
#include <memory/page.hpp>
#include <errno/errno.h>
#include <algorithm>

namespace Hamster
{
    namespace
    {
        // Helper function to get the page start address
        uint64_t get_page_start(uint64_t addr)
        {
            return addr & ~(HAMSTER_PAGE_SIZE - 1);
        }

        // Helper function to get the page offset
        uint64_t get_page_offset(uint64_t addr)
        {
            return addr & (HAMSTER_PAGE_SIZE - 1);
        }
    }

    int MemorySpace::set(uint64_t addr, uint8_t value)
    {
        auto page_start = get_page_start(addr);
        auto page_offset = get_page_offset(addr);

        if (pages.find(page_start) == pages.end())
        {
            if (!allocate_page(page_start, PERM_READ | PERM_WRITE | PERM_EXECUTE))
                return -1;

            // try again
            return set(addr, value);
        }

        if (!check_perm(page_start, PERM_WRITE))
        {
            // Cannot write
            error = EACCES;
            return -1;
        }

        if (swap_in_page(page_start) < 0)
            return -1;
        
        uint8_t &byte = pages[page_start][page_offset];
        if (&byte == &Page::get_dummy_byte())
        {
            // Byte access failed
            error = EIO;
            return -1;
        }

        byte = value;
        return 0;
    }

    int16_t MemorySpace::get(uint64_t addr)
    {
        auto page_start = get_page_start(addr);
        auto page_offset = get_page_offset(addr);

        if (pages.find(page_start) == pages.end())
        {
            // Page not found
            error = ENOENT;
            return -1;
        }

        if (!check_perm(page_start, PERM_READ))
        {
            // Cannot read
            error = EACCES;
            return -1;
        }

        if (swap_in_page(page_start) < 0)
            return -1;

        uint8_t &byte = pages[page_start][page_offset];
        if (&byte == &Page::get_dummy_byte())
        {
            // Byte access failed
            error = EIO;
            return -1;
        }

        return byte;
    }

    uint64_t MemorySpace::get_page_count()
    {
        return pages.size();
    }

    int MemorySpace::memcpy(uint64_t dest, uint64_t src, uint64_t size)
    {
        auto dest_page_start = get_page_start(dest);
        auto dest_page_offset = get_page_offset(dest);
        auto src_page_start = get_page_start(src);
        auto src_page_offset = get_page_offset(src);

        if (pages.find(dest_page_start) == pages.end())
        {
            // Destination page not found
            if (!allocate_page(dest_page_start, PERM_READ | PERM_WRITE | PERM_EXECUTE))
                return -1;

            // try again
            return memcpy(dest, src, size);
        }

        if (pages.find(src_page_start) == pages.end())
        {
            // Source page not found
            error = ENOENT;
            return -1;
        }

        if (!check_perm(dest_page_start, PERM_WRITE) || !check_perm(src_page_start, PERM_READ))
        {
            // Cannot write or read
            error = EACCES;
            return -1;
        }

        if (swap_in_page(dest_page_start) < 0 || swap_in_page(src_page_start) < 0)
            return -1;

        for (uint64_t i = 0; i < size; ++i)
        {
            pages[dest_page_start][dest_page_offset + i] = pages[src_page_start][src_page_offset + i];
        }

        return 0;
    }

    int MemorySpace::memcpy_to_buffer(uint8_t *dest, uint64_t src, uint64_t size)
    {
        auto src_page_start = get_page_start(src);
        auto src_page_offset = get_page_offset(src);

        if (pages.find(src_page_start) == pages.end())
        {
            // Source page not found
            error = ENOENT;
            return -1;
        }

        if (!check_perm(src_page_start, PERM_READ))
        {
            // Cannot read
            error = EACCES;
            return -1;
        }

        if (swap_in_page(src_page_start) < 0)
            return -1;

        for (uint64_t i = 0; i < size; ++i)
        {
            dest[i] = pages[src_page_start][src_page_offset + i];
        }

        return 0;
    }

    int MemorySpace::memcpy_from_buffer(uint64_t dest, const uint8_t *src, uint64_t size)
    {
        auto dest_page_start = get_page_start(dest);
        auto dest_page_offset = get_page_offset(dest);

        if (pages.find(dest_page_start) == pages.end())
        {
            // Destination page not found
            if (!allocate_page(dest_page_start, PERM_READ | PERM_WRITE | PERM_EXECUTE))
                return -1;

            // try again
            return memcpy_from_buffer(dest, src, size);
        }

        if (!check_perm(dest_page_start, PERM_WRITE))
        {
            // Cannot write
            error = EACCES;
            return -1;
        }

        if (swap_in_page(dest_page_start) < 0)
            return -1;

        for (uint64_t i = 0; i < size; ++i)
        {
            pages[dest_page_start][dest_page_offset + i] = src[i];
        }

        return 0;
    }

    int MemorySpace::memset(uint64_t addr, uint8_t value, uint64_t size)
    {
        auto page_start = get_page_start(addr);
        auto page_offset = get_page_offset(addr);

        if (pages.find(page_start) == pages.end())
        {
            // Page not found
            if (!allocate_page(page_start, PERM_READ | PERM_WRITE | PERM_EXECUTE))
                return -1;

            // try again
            return memset(addr, value, size);
        }

        if (!check_perm(page_start, PERM_WRITE))
        {
            // Cannot write
            error = EACCES;
            return -1;
        }

        if (swap_in_page(page_start) < 0)
            return -1;

        for (uint64_t i = 0; i < size; ++i)
        {
            pages[page_start][page_offset + i] = value;
        }

        return 0;
    }

    int MemorySpace::set_page_perms(uint64_t addr, uint8_t perms)
    {
        auto page_start = get_page_start(addr);

        if (pages.find(page_start) == pages.end())
        {
            // Page not found
            error = ENOENT;
            return -1;
        }

        pages[page_start].get_flags() &= ~PERM_READ & ~PERM_WRITE & ~PERM_EXECUTE;
        pages[page_start].get_flags() |= perms & (PERM_READ | PERM_WRITE | PERM_EXECUTE);
        return 0;
    }

    bool MemorySpace::is_allocated(uint64_t addr, uint64_t size)
    {
        for (uint64_t i = get_page_start(addr); i <= get_page_start(addr + size); i += HAMSTER_PAGE_SIZE)
        {
            if (pages.find(i) == pages.end())
            {
                return false;
            }
        }
        return true;
    }

    int MemorySpace::allocate_page(uint64_t addr, uint8_t perms)
    {
        addr = get_page_start(addr);
        if (is_allocated(addr))
        {
            // Page already allocated
            error = EEXIST;
            return -1;
        }
        assert(addr % HAMSTER_PAGE_SIZE == 0);

        // Default-construct the page
        pages[addr];

        if (swap_in_page(addr) < 0)
            return -1;

        return set_page_perms(addr, perms);
    }

    int MemorySpace::allocate_range(uint64_t addr, uint64_t size, uint8_t perms)
    {
        if (is_allocated(addr, size))
        {
            // Range already allocated
            error = EEXIST;
            return -1;
        }
        for (uint64_t i = get_page_start(addr); i < get_page_start(addr + size); i += HAMSTER_PAGE_SIZE)
        {
            if (allocate_page(i, perms) < 0)
            {
                // Skip if some pages are already allocated
                if (error == EEXIST)
                    continue;
                // Failed to allocate page
                return -1;
            }
        }
        return 0;
    }

    int MemorySpace::deallocate_page(uint64_t addr)
    {
        addr = get_page_start(addr);
        if (pages.find(addr) == pages.end())
        {
            // Page not found
            error = ENOENT;
            return -1;
        }

        if (clear_page_from_swap_queue(addr) < 0)
            return -1;

        pages.erase(addr);
        return 0;
    }

    int MemorySpace::swap_off_all()
    {
        for (auto &swapped_on_page : swapped_on_pages)
        {
            if (pages[swapped_on_page].swap_out() < 0)
                return -1;
        }
        swapped_on_pages.clear();
        return 0;
    }

    bool MemorySpace::check_perm(uint64_t addr, uint8_t perms)
    {
        auto page_start = get_page_start(addr);
        if (pages.find(page_start) == pages.end())
        {
            // Page not found
            return false;
        }
        perms &= (PERM_READ | PERM_WRITE | PERM_EXECUTE);
        return (pages[page_start].get_flags() & perms) == perms;
    }

    int MemorySpace::swap_in_page(uint64_t addr)
    {
        if (pages[addr].swap_in() < 0)
            return -1;
        clear_page_from_swap_queue(addr);
        swapped_on_pages.push_back(addr);

        if (swapped_on_pages.size() > HAMSTER_CONCUR_PAGES)
        {
            // Swap out the oldest page
            auto page = swapped_on_pages.front();
            if (pages[page].swap_out() < 0)
                return -1;
            swapped_on_pages.pop_front();
        }
        return 0;
    }

    int MemorySpace::clear_page_from_swap_queue(uint64_t addr)
    {
        auto it = std::find(swapped_on_pages.begin(), swapped_on_pages.end(), addr);
        while (it != swapped_on_pages.end())
        {
            swapped_on_pages.erase(it);
            it = std::find(swapped_on_pages.begin(), swapped_on_pages.end(), addr);
        }
        return 0;
    }
} // namespace Hamster

