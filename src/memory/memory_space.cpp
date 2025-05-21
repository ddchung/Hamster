// Hamster memory space

#include <memory/page_manager.hpp>
#include <memory/memory_space.hpp>
#include <memory/allocator.hpp>
#include <platform/config.hpp>
#include <errno/errno.h>
#include <cstdint>
#include <cassert>


namespace Hamster
{
    namespace
    {
        uint64_t get_page_start(uint64_t addr)
        {
            return addr & ~(HAMSTER_PAGE_SIZE - 1);
        }

        uint64_t get_page_offset(uint64_t addr)
        {
            return addr & (HAMSTER_PAGE_SIZE - 1);
        }
    } // namespace
    
    uint8_t &MemorySpace::operator[](uint64_t addr)
    {
        if (ensure_page(addr) < 0)
            return Page::get_dummy_byte();
        
        return pages[get_page_start(addr)][get_page_offset(addr)];
    }

    int MemorySpace::memcpy(uint64_t addr, const void *buffer, size_t size)
    {
        if (!check_permissions(addr, 02, size)) // 02 = write
        {
            error = EACCES;
            return -1;
        }

        // TODO: More efficient implementation
        for (size_t i = 0; i < size; i++)
        {
            uint8_t &byte = (*this)[addr + i];
            if (&byte == &Page::get_dummy_byte())
                return -1;
            byte = ((uint8_t *)buffer)[i];
        }
        return 0;
    }

    int MemorySpace::memcpy(void *buffer, uint64_t addr, size_t size)
    {
        if (!check_permissions(addr, 04, size)) // 04 = read
        {
            error = EACCES;
            return -1;
        }
        
        // TODO: More efficient implementation
        for (size_t i = 0; i < size; i++)
        {
            uint8_t &byte = (*this)[addr + i];
            if (&byte == &Page::get_dummy_byte())
                return -1;
            ((uint8_t *)buffer)[i] = byte;
        }
        return 0;
    }

    int MemorySpace::memcpy(uint64_t src, uint64_t dst, size_t size)
    {
        if (!check_permissions(src, 04, size)) // 04 = read
        {
            error = EACCES;
            return -1;
        }
        if (!check_permissions(dst, 02, size)) // 02 = write
        {
            error = EACCES;
            return -1;
        }
        
        // TODO: More efficient implementation
        for (size_t i = 0; i < size; i++)
        {
            uint8_t &src_byte = (*this)[src + i];
            uint8_t &dst_byte = (*this)[dst + i];
            if (&src_byte == &Page::get_dummy_byte() || &dst_byte == &Page::get_dummy_byte())
                return -1;
            dst_byte = src_byte;
        }
        return 0;
    }

    int MemorySpace::memset(uint64_t addr, uint8_t value, size_t size)
    {
        if (!check_permissions(addr, 02, size)) // 02 = write
        {
            error = EACCES;
            return -1;
        }
        
        for (size_t i = 0; i < size; i++)
        {
            uint8_t &byte = (*this)[addr + i];
            if (&byte == &Page::get_dummy_byte())
                return -1;
            byte = value;
        }
        return 0;
    }

    char *MemorySpace::get_string(uint64_t addr)
    {
        if (!check_permissions(addr, 04, 1)) // 04 = read
        {
            error = EACCES;
            return nullptr;
        }

        // Get the string size
        size_t size = 0;
        uint64_t tmp_addr = addr;
        for (; (*this)[tmp_addr] != '\0'; tmp_addr++)
        {
            if (&(*this)[tmp_addr] == &Page::get_dummy_byte())
                return nullptr;
            size++;
        }

        // Copy the string
        char *str = alloc<char>(size + 1);
        
        if (this->memcpy(str, addr, size) < 0)
        {
            dealloc(str);
            return nullptr;
        }

        str[size] = '\0';
        return str;
    }

    bool MemorySpace::is_allocated(uint64_t addr)
    {
        return pages.find(get_page_start(addr)) != pages.end();
    }

    int MemorySpace::deallocate_page(uint64_t addr)
    {
        auto it = pages.find(get_page_start(addr));
        if (it == pages.end())
        {
            error = EINVAL;
            return -1;
        }

        pages.erase(it);

        // also remove from swapped on pages, if any
        swapped_on_pages.remove(get_page_start(addr));
        return 0;
    }

    int MemorySpace::set_permissions(uint64_t addr, uint8_t mode, size_t size)
    {
        if (mode > 07)
        {
            error = EINVAL;
            return -1;
        }

        for (uint64_t it = get_page_start(addr); it <= get_page_start(addr + size); it += HAMSTER_PAGE_SIZE)
        {
            if (ensure_page(it) < 0)
                return -1;
            auto page_it = pages.find(it);
            assert(page_it != pages.end());
            page_it->second.get_flags() = mode;
        }
        return 0;
    }

    bool MemorySpace::check_permissions(uint64_t addr, uint8_t req_perms, size_t size)
    {
        for (uint64_t it = get_page_start(addr); it <= get_page_start(addr + size); it += HAMSTER_PAGE_SIZE)
        {
            auto page_it = pages.find(it);
            if (page_it == pages.end())
                continue; //< Default permissions when new page is created is 07 (rwx)
            if ((page_it->second.get_flags() & req_perms) != req_perms)
                return false;
        }
        return true;
    }

    int MemorySpace::swap_out_all()
    {
        for (auto it = pages.begin(); it != pages.end();)
        {
            if (it->second.is_swapped())
            {
                it++;
                continue;
            }
            if (it->second.swap_out() < 0)
            {
                error = EIO;
                return -1;
            }
        }
        swapped_on_pages.clear();
        return 0;
    }

    int MemorySpace::ensure_page(uint64_t addr)
    {
        auto it = pages.find(get_page_start(addr));
        if (it == pages.end())
        {
            // create a new page

            // default construct
            pages[get_page_start(addr)].get_flags() = 07;

            it = pages.find(get_page_start(addr));

            assert(it != pages.end());
        }

        if (it->second.is_swapped() && it->second.swap_in() < 0)
        {
            error = EIO;
            return -1;
        }

        swapped_on_pages.remove(get_page_start(addr));
        swapped_on_pages.push_back(get_page_start(addr));

        while (swapped_on_pages.size() > HAMSTER_CONCUR_PAGES)
        {
            // Need to swap out pages
            uint64_t page_addr = swapped_on_pages.front();
            it = pages.find(page_addr);

            swapped_on_pages.pop_front();

            if (it == pages.end())
                continue;
            
            it->second.swap_out();
        }

        return 0;
    }
} // namespace Hamster

