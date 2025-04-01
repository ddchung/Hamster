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

        if (pages.find(addr) != pages.end())
        {
            return -2; // page already allocated
        }

        pages.emplace(decltype(pages)::value_type(addr, Page()));

        return pages[addr].get_swap_index();
    }

    int MemorySpace::deallocate_page(uint64_t addr)
    {
        auto it = pages.find(addr);
        if (it == pages.end())
            return -1;
        
        pages.erase(it);
        return 0;
    }

    uint8_t &MemorySpace::operator[](uint64_t addr)
    {
        uint64_t offset = get_addr_offset(addr);
        uint64_t page_start = get_page_start(addr);
        auto it = pages.find(page_start);
        if (it == pages.end())
            Page::get_dummy();
        return it->second[offset];
    }

    uint8_t *MemorySpace::get_page_data(uint64_t addr)
    {
        uint64_t page_start = get_page_start(addr);
        auto it = pages.find(page_start);
        if (it == pages.end())
            return nullptr;
        return it->second.get_data();
    }
} // namespace Hamster

