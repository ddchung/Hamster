// implementation of memory_space.hpp

#include <memory/memory_space.hpp>
#include <memory/allocator.hpp>
#include <memory/page.hpp>
#include <utility>

namespace Hamster
{
    uint64_t MemorySpace::get_addr_offset(uint64_t addr)
    {
        return addr & (HAMSTER_PAGE_SIZE - 1);
    }

    uint64_t MemorySpace::get_page_start(uint64_t addr)
    {
        return addr & ~(HAMSTER_PAGE_SIZE - 1);
    }
    MemorySpace::MemorySpace(MemorySpace &&other)
    {
        pages = std::move(other.pages);
    }

    MemorySpace &MemorySpace::operator=(MemorySpace &&other)
    {
        if (this != &other)
        {
            pages = std::move(other.pages);
        }
        return *this;
    }

    int MemorySpace::allocate_page(uint64_t addr)
    {
        if (pages.size() >= HAMSTER_MAX_PAGES)
        {
            return -1; // out of memory
        }

        addr = get_page_start(addr);

        if (pages.find(addr) != pages.end())
        {
            return -2; // page already allocated
        }

        if (queue(addr) < 0)
            return -3; // queue failed
        
        return pages[addr].get_page_id();
    }

    int MemorySpace::deallocate_page(uint64_t addr)
    {
        addr = get_page_start(addr);
        auto it = pages.find(addr);
        if (it == pages.end())
            return -1;
        
        pages.erase(it);
        swapped_on_pages.remove(addr);
        return 0;
    }

    uint8_t &MemorySpace::operator[](uint64_t addr)
    {
        uint64_t offset = get_addr_offset(addr);
        uint64_t page_start = get_page_start(addr);
        auto it = pages.find(page_start);
        if (it == pages.end())
            return Page::get_dummy(); 
        Page &page = it->second;

        if (page.is_swapped())
        {
            if (page.swap_in() < 0 || queue(page_start) < 0)
                return Page::get_dummy();
        }

        return page[offset];
    }

    uint8_t *MemorySpace::get_page_data(uint64_t addr)
    {
        uint64_t page_start = get_page_start(addr);
        auto it = pages.find(page_start);
        if (it == pages.end())
            return nullptr;
        
        Page &page = it->second;

        if (page.is_swapped())
        {
            if (page.swap_in() < 0 || queue(page_start) < 0)
                return nullptr;
        }

        return page.get_data();
    }

    bool MemorySpace::is_page_allocated(uint64_t addr)
    {
        addr = get_page_start(addr);
        return pages.find(addr) != pages.end();
    }

    bool MemorySpace::is_page_range_allocated(uint64_t addr, size_t size)
    {
        uint64_t page_start = get_page_start(addr);
        uint64_t page_end = get_page_start(addr + size - 1);

        for (uint64_t page = page_start; page <= page_end; page += HAMSTER_PAGE_SIZE)
        {
            if (pages.find(page) == pages.end())
                return false;
        }
        return true;
    }

    int MemorySpace::memcpy(uint64_t dest, uint64_t src, size_t size)
    {
        if (!is_page_range_allocated(dest, size) || !is_page_range_allocated(src, size))
            return -1;

        for (size_t i = 0; i < size; i++)
        {
            uint64_t dest_addr = dest + i;
            uint64_t src_addr = src + i;
            (*this)[dest_addr] = (*this)[src_addr];
        }
        return 0;
    }

    int MemorySpace::memcpy(uint64_t dest, uint8_t *src, size_t size)
    {
        if (!is_page_range_allocated(dest, size))
            return -1;

        for (size_t i = 0; i < size; i++)
        {
            uint64_t dest_addr = dest + i;
            (*this)[dest_addr] = src[i];
        }
        return 0;
    }

    int MemorySpace::memcpy(uint8_t *dest, uint64_t src, size_t size)
    {
        if (!is_page_range_allocated(src, size))
            return -1;

        for (size_t i = 0; i < size; i++)
        {
            uint64_t src_addr = src + i;
            dest[i] = (*this)[src_addr];
        }
        return 0;
    }

    int MemorySpace::memset(uint64_t dest, uint8_t value, size_t size)
    {
        if (!is_page_range_allocated(dest, size))
            return -1;

        for (size_t i = 0; i < size; i++)
        {
            uint64_t dest_addr = dest + i;
            (*this)[dest_addr] = value;
        }
        return 0;
    }

    int MemorySpace::queue(uint64_t page_start)
    {
        // ensure it is page start
        page_start = get_page_start(page_start);

        // remove all existing occurences
        swapped_on_pages.remove(page_start);

        // add
        swapped_on_pages.push_front(page_start);

        // clean
        return clean();
    }

    int MemorySpace::swap_out_pages()
    {
        for (auto &page : swapped_on_pages)
        {
            auto it = pages.find(page);
            if (it == pages.end())
                continue;
            it->second.swap_out();
        }
        return 0;
    }

    int MemorySpace::swap_in_pages()
    {
        for (auto &page : swapped_on_pages)
        {
            auto it = pages.find(page);
            if (it == pages.end())
                continue;
            it->second.swap_in();
        }
        return 0;
    }

    int MemorySpace::clean()
    {
        if (swapped_on_pages.size() <= HAMSTER_CONCUR_PAGES)
            return 0; // no need to clean
        
        uint64_t page_start = swapped_on_pages.back();
        page_start = get_page_start(page_start);
        
        auto it = pages.find(page_start);
        if (it == pages.end())
        {
            // not found
            swapped_on_pages.pop_back();
            return -1;
        }

        Page &page = it->second;

        // swap out
        int res = page.swap_out();
        swapped_on_pages.pop_back();

        // continue cleaning
        if (clean() < 0)
            res = -2;

        return res;
    }
} // namespace Hamster

