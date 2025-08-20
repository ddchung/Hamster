// Hamster memory space implementation

#include <memory/memory_space.hpp>
#include <memory/page_manager.hpp>
#include <memory/allocator.hpp>
#include <filesystem/vfs.hpp>
#include <errno/errno.h>
#include <algorithm>
#include <cassert>

// Helpers

#define ROUND_DOWN_PAGE(addr) ((addr) & ~(HAMSTER_PAGE_SIZE - 1))
#define ROUND_UP_PAGE(addr) (ROUND_DOWN_PAGE((addr) + HAMSTER_PAGE_SIZE - 1))

namespace Hamster
{   
    MemorySpace::~MemorySpace()
    {
        unmap_all();
    }

    MemorySpace::MemorySpace(const MemorySpace &other)
    {
        page_table.reserve(other.page_table.size());
        for (const auto &[id, entry] : other.page_table)
        {
            page_table[id] = page_manager.copy(entry);
        }
    }

    MemorySpace &MemorySpace::operator=(const MemorySpace &other)
    {
        if (this != &other)
        {
            // Clean up current pages
            for (auto &[id, entry] : page_table)
            {
                page_manager.free_page(id);
            }
            page_table.clear();

            // Copy pages from other
            page_table.reserve(other.page_table.size());
            for (const auto &[id, entry] : other.page_table)
            {
                page_table[id] = page_manager.copy(entry);
            }
        }
        return *this;
    }

    MemorySpace::MemorySpace(MemorySpace &&other)
    {
        page_table = std::move(other.page_table);
        other.page_table.clear();
    }

    MemorySpace &MemorySpace::operator=(MemorySpace &&other)
    {
        if (this != &other)
        {
            // Clean up current pages
            for (auto &[id, entry] : page_table)
            {
                page_manager.free_page(id);
            }
            page_table.clear();

            // Move pages from other
            page_table = std::move(other.page_table);
            other.page_table.clear();
        }
        return *this;
    }

    int MemorySpace::memcpy(void *dest, uint32_t src, uint32_t len)
    {
        assert(dest);

        for (uint32_t addr = src; addr < src + len; addr = ROUND_UP_PAGE(addr + 1))
        {
            uint32_t chunk_size = std::min<uint32_t>(HAMSTER_PAGE_SIZE, src + len - addr);
            if (do_read(addr, (uint8_t *)dest + (addr - src), chunk_size) < 0)
                return -1;
        }

        return 0;
    }

    int MemorySpace::memcpy(uint32_t dest, const void *src, uint32_t len)
    {
        assert(src);

        for (uint32_t addr = dest; addr < dest + len; addr = ROUND_UP_PAGE(addr + 1))
        {
            uint32_t chunk_size = std::min<uint32_t>(HAMSTER_PAGE_SIZE, dest + len - addr);
            if (do_write(addr, (const uint8_t *)src + (addr - dest), chunk_size) < 0)
                return -1;
        }

        return 0;
    }

    int MemorySpace::memcpy_alloc(uint32_t dest, const void *src, uint32_t len)
    {
        if (is_mapped(dest, len) == 0)
        {
            if (map_anonymous(dest, len, PERM_READ | PERM_WRITE) < 0)
                return -1;
        }
        return memcpy(dest, src, len);
    }

    int MemorySpace::memset(uint32_t loc, uint8_t byte, uint32_t len)
    {
        // TODO: better implementation
        for (uint32_t addr = loc; addr < loc + len; addr += 1)
        {
            if (do_write(addr, &byte, 1) < 0)
                return -1;
        }
        return 0;
    }

    int MemorySpace::memset_alloc(uint32_t addr, uint8_t value, uint32_t len)
    {
        if (is_mapped(addr, len) == 0)
        {
            if (map_anonymous(addr, len, PERM_READ | PERM_WRITE) < 0)
                return -1;
        }
        return memset(addr, value, len);
    }

    int MemorySpace::is_mapped(uint32_t loc, uint32_t size) const
    {
        loc = ROUND_DOWN_PAGE(loc);
        size = ROUND_UP_PAGE(size);
        for (uint32_t addr = loc; addr < loc + size; addr += HAMSTER_PAGE_SIZE)
        {
            if (page_table.find(addr) == page_table.end())
                return 0;
        }
        return 1;
    }

    char *MemorySpace::read_until_zero(uint32_t addr)
    {
        size_t len = 0;

        for (uint32_t it = addr;; ++it)
        {
            char c;
            if (do_read(it, &c, 1) != 1)
                return nullptr;
            ++len;
            if (c == '\0')
                break;
        }

        char *result = alloc<char>(len);

        result[len - 1] = '\0';

        if (memcpy(result, addr, len) < 0)
        {
            dealloc(result);
            return nullptr;
        }

        return result;
    }

    ssize_t MemorySpace::how_many_mapped(uint32_t loc, uint32_t size) const
    {
        loc = ROUND_DOWN_PAGE(loc);
        size = ROUND_UP_PAGE(size);
        ssize_t count = 0;
        for (uint32_t addr = loc; addr < loc + size; addr += HAMSTER_PAGE_SIZE)
        {
            if (page_table.find(addr) != page_table.end())
                count++;
        }
        return count;
    }

    int MemorySpace::map_anonymous(uint32_t loc, uint32_t size, uint8_t perms)
    {
        // Round up size to nearest page size
        size = ROUND_UP_PAGE(size);
        loc = ROUND_DOWN_PAGE(loc);
        next_mmap = std::max<uint32_t>(next_mmap, loc + size);

        for (uint32_t addr = loc; addr <= loc + size; addr += HAMSTER_PAGE_SIZE)
        {
            if (page_table.find(addr) != page_table.end())
                continue;
            page_table[addr] = page_manager.allocate_page(perms);
        }
        return 0;
    }

    int MemorySpace::map_private_file(uint32_t loc, int fd, uint32_t offset, uint32_t size, uint8_t perms)
    {
        size = ROUND_UP_PAGE(size + (offset & (HAMSTER_PAGE_SIZE - 1)));
        offset = ROUND_DOWN_PAGE(offset);
        loc = ROUND_DOWN_PAGE(loc);
        next_mmap = std::max<uint32_t>(next_mmap, loc + size);

        for (uint32_t addr = loc; addr <= loc + size; addr += HAMSTER_PAGE_SIZE)
        {
            auto it = page_table.find(addr);
            if (it != page_table.end())
                page_manager.free_page(it->second);
            int cloned = vfs.dup(fd);
            if (cloned < 0)
                return -1;
            page_table[addr] = page_manager.mmap_private(cloned, offset + (addr - loc), perms);
        }
        return 0;
    }

    int MemorySpace::unmap(uint32_t loc, uint32_t size)
    {
        loc = ROUND_DOWN_PAGE(loc);
        size = ROUND_UP_PAGE(size);

        uint32_t free_range_start = loc;
        uint32_t free_range_size = 0;

        for (uint32_t addr = loc; addr < loc + size; addr += HAMSTER_PAGE_SIZE)
        {
            auto it = page_table.find(addr);
            if (it == page_table.end())
            {
                if (free_range_size)
                    deallocate(free_range_start, free_range_size);
                free_range_start = addr + HAMSTER_PAGE_SIZE;
                free_range_size = 0;
            }

            free_range_size += HAMSTER_PAGE_SIZE;

            page_manager.free_page(it->second);
            page_table.erase(it);
        }

        if (free_range_size)
            deallocate(free_range_start, free_range_size);

        return 0;
    }

    int MemorySpace::unmap_all()
    {
        for (auto &[_, page_id] : page_table)
        {
            page_manager.free_page(page_id);
        }
        page_table.clear();
        return 0;
    }

    int MemorySpace::mprotect(uint32_t loc, uint32_t size, uint8_t perms)
    {
        loc = ROUND_DOWN_PAGE(loc);
        size = ROUND_UP_PAGE(size);

        for (uint32_t addr = loc; addr < loc + size; addr += HAMSTER_PAGE_SIZE)
        {
            auto it = page_table.find(addr);
            if (it == page_table.end())
                continue;

            if (page_manager.set_permissions(it->second, perms) < 0)
                return -1;
        }
        return 0;
    }

    uint32_t MemorySpace::allocate(uint32_t size)
    {
        size = ROUND_UP_PAGE(size);

        for (uint32_t i = 0; i < free_ranges.size(); ++i)
        {
            FreeRange &range = free_ranges.front();
            assert(range.size % HAMSTER_PAGE_SIZE == 0);
            assert(range.addr % HAMSTER_PAGE_SIZE == 0);
            if (range.size >= size)
            {
                range.size -= size;
                uint32_t addr = range.addr;
                range.addr += size;

                if (range.size == 0)
                    free_ranges.pop();

                return addr;
            }
            free_ranges.advance();
        }

        assert(next_mmap % HAMSTER_PAGE_SIZE == 0);
        uint32_t addr = next_mmap;
        next_mmap += size;
        return addr;
    }

    int8_t MemorySpace::get_permissions(uint32_t loc, uint32_t size)
    {
        loc = ROUND_DOWN_PAGE(loc);
        size = ROUND_UP_PAGE(size);

        uint8_t perms = 07; // Start with all

        for (uint32_t addr = loc; addr < loc + size; addr += HAMSTER_PAGE_SIZE)
        {
            auto it = page_table.find(addr);
            if (it == page_table.end())
                return -1;

            int8_t page_perms = page_manager.get_permissions(it->second);
            if (page_perms < 0)
                return -1;

            perms &= page_perms;
        }
        return perms;
    }

    void MemorySpace::deallocate(uint32_t addr, uint32_t size)
    {
        assert(addr % HAMSTER_PAGE_SIZE == 0);
        assert(size % HAMSTER_PAGE_SIZE == 0);

        for (size_t i = 0; i < free_ranges.size(); ++i)
        {
            // Attempt to merge into an existing range if possible
            FreeRange &range = free_ranges.front();
            free_ranges.advance();
            assert(range.size % HAMSTER_PAGE_SIZE == 0);
            assert(range.addr % HAMSTER_PAGE_SIZE == 0);
            if (range.addr + range.size == addr)
            {
                range.size += size;
                return;
            }
            if (addr + size == range.addr)
            {
                range.addr = addr;
                range.size += size;
                return;
            }
        }
        free_ranges.push(FreeRange{addr, size});
    }

    ssize_t MemorySpace::do_read(uint32_t addr, void *buf, size_t len)
    {
        assert(buf != nullptr);

        if (len == 0)
            return 0;

        auto it = page_table.find(ROUND_DOWN_PAGE(addr));
        if (it == page_table.end())
        {
            error = EFAULT;
            return -1;
        }

        return page_manager.read(it->second, addr & (HAMSTER_PAGE_SIZE - 1), buf, len);
    }

    ssize_t MemorySpace::do_write(uint32_t addr, const void *buf, size_t len)
    {
        assert(buf != nullptr);

        if (len == 0)
            return 0;

        auto it = page_table.find(ROUND_DOWN_PAGE(addr));
        if (it == page_table.end())
        {
            error = EFAULT;
            return -1;
        }

        return page_manager.write(it->second, addr & (HAMSTER_PAGE_SIZE - 1), buf, len);
    }
} // namespace Hamster

