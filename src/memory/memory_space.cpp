// Hamster memory space implementation

#include <memory/memory_space.hpp>
#include <memory/page_manager.hpp>
#include <memory/allocator.hpp>
#include <filesystem/vfs.hpp>
#include <errno/errno.h>
#include <algorithm>
#include <cassert>
#include <cstring>

// Helpers

#define ROUND_DOWN_PAGE(addr) ((addr) & ~(HAMSTER_PAGE_SIZE - 1))
#define ROUND_UP_PAGE(addr) (ROUND_DOWN_PAGE((addr) + HAMSTER_PAGE_SIZE - 1))

namespace Hamster
{
    MemorySpace::MemorySpace(const MemorySpace &other)
        : page_table(other.page_table), free_ranges(), next_mmap(other.next_mmap)
    {
    }

    MemorySpace &MemorySpace::operator=(const MemorySpace &other)
    {
        if (this != &other)
        {
            page_table = other.page_table;
            free_ranges.clear();
            next_mmap = other.next_mmap;
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
        static uint8_t buf[HAMSTER_PAGE_SIZE];
        ::memset(buf, byte, sizeof(buf));
        for (uint32_t addr = loc; addr < loc + len; addr += sizeof(buf))
        {
            size_t chunk_size = std::min<uint32_t>(sizeof(buf), loc + len - addr);
            if (do_write(addr, buf, chunk_size) < 0)
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
        uint32_t end = ROUND_UP_PAGE(loc + size) >> HAMSTER_PAGE_SIZE_BITS;
        loc >>= HAMSTER_PAGE_SIZE_BITS;

        for (; loc < end; ++loc)
        {
            if (page_table.get_page_direct(loc) == PageTable::PAGE_ID_UNUSED)
                return 0;
        }
        return 1;
    }

    ssize_t MemorySpace::how_many_mapped(uint32_t loc, uint32_t size) const
    {
        uint32_t end = ROUND_UP_PAGE(loc + size) >> HAMSTER_PAGE_SIZE_BITS;
        loc >>= HAMSTER_PAGE_SIZE_BITS;

        ssize_t count = 0;
        for (uint32_t i = loc; i < end; i++)
        {
            if (page_table.get_page_direct(i) != PageTable::PAGE_ID_UNUSED)
                count++;
        }
        return count;
    }

    int MemorySpace::map_anonymous(uint32_t loc, uint32_t size, uint8_t perms)
    {
        uint32_t end = ROUND_UP_PAGE(loc + size) >> HAMSTER_PAGE_SIZE_BITS;
        loc >>= HAMSTER_PAGE_SIZE_BITS;
        next_mmap = std::max<uint32_t>(next_mmap, end << HAMSTER_PAGE_SIZE_BITS);

        for (; loc < end; ++loc)
        {
            if (page_table.get_page_direct(loc) == PageTable::PAGE_ID_UNUSED)
                page_table.set_page_direct(loc, page_manager.allocate_page(perms));
        }
        return 0;
    }

    int MemorySpace::map_shared_anonymous(uint32_t loc, uint32_t size, uint8_t perms)
    {
        uint32_t end = ROUND_UP_PAGE(loc + size) >> HAMSTER_PAGE_SIZE_BITS;
        loc >>= HAMSTER_PAGE_SIZE_BITS;
        next_mmap = std::max<uint32_t>(next_mmap, end << HAMSTER_PAGE_SIZE_BITS);

        for (; loc < end; ++loc)
        {
            if (page_table.get_page_direct(loc) == PageTable::PAGE_ID_UNUSED)
            {
                uint32_t page = page_manager.allocate_page(perms);
                page_manager.make_shared(page);
                page_table.set_page_direct(loc, page);
            }
        }
        return 0;
    }

    int MemorySpace::map_private_file(uint32_t loc, int fd, uint32_t offset, uint32_t size, uint8_t perms)
    {
        uint32_t end = ROUND_UP_PAGE(loc + size) >> HAMSTER_PAGE_SIZE_BITS;
        loc >>= HAMSTER_PAGE_SIZE_BITS;

        // Correctly account for attempts to map in the middle of a page
        offset = ROUND_DOWN_PAGE(offset);

        next_mmap = std::max<uint32_t>(next_mmap, loc + size);

        FileMappingFD *fmfd = alloc<FileMappingFD>();
        fmfd->fd = vfs.dup(fd);
        fmfd->refcount = 0;

        if (fmfd->fd < 0)
        {
            dealloc(fmfd);
            return -1;
        }

        for (; loc < end; ++loc)
        {
            // note: set_page automatically frees an existing entry, if present
            page_table.set_page_direct(loc, page_manager.mmap_private(fmfd, offset, perms));
            offset += HAMSTER_PAGE_SIZE;
        }
        return 0;
    }

    int MemorySpace::map_shared_file(uint32_t loc, int fd, uint32_t offset, uint32_t size, uint8_t perms)
    {
        uint32_t end = ROUND_UP_PAGE(loc + size) >> HAMSTER_PAGE_SIZE_BITS;
        loc >>= HAMSTER_PAGE_SIZE_BITS;

        // Correctly account for attempts to map in the middle of a page
        offset = ROUND_DOWN_PAGE(offset);

        next_mmap = std::max<uint32_t>(next_mmap, loc + size);

        FileMappingFD *fmfd = alloc<FileMappingFD>();
        fmfd->fd = vfs.dup(fd);
        fmfd->refcount = 0;

        if (fmfd->fd < 0)
        {
            dealloc(fmfd);
            return -1;
        }

        for (; loc < end; ++loc)
        {
            // note: set_page automatically frees an existing entry, if present
            uint32_t page = page_manager.mmap_private(fmfd, offset, perms);
            page_manager.make_shared(page);
            page_table.set_page_direct(loc, page);
            offset += HAMSTER_PAGE_SIZE;
        }
        return 0;
    }

    int MemorySpace::unmap(uint32_t loc, uint32_t size)
    {
        uint32_t end = ROUND_UP_PAGE(loc + size) >> HAMSTER_PAGE_SIZE_BITS;
        loc >>= HAMSTER_PAGE_SIZE_BITS;

        uint32_t free_range_start = loc << HAMSTER_PAGE_SIZE_BITS;
        uint32_t free_range_size = 0;

        for (; loc < end; ++loc)
        {
            if (page_table.get_page_direct(loc) == PageTable::PAGE_ID_UNUSED)
            {
                if (free_range_size)
                    deallocate(free_range_start, free_range_size);
                free_range_start = (loc << HAMSTER_PAGE_SIZE_BITS) + HAMSTER_PAGE_SIZE;
                free_range_size = 0;
            }

            free_range_size += HAMSTER_PAGE_SIZE;

            page_table.set_page_direct(loc, PageTable::PAGE_ID_UNUSED);
        }

        if (free_range_size)
            deallocate(free_range_start, free_range_size);

        return 0;
    }

    int MemorySpace::unmap_all()
    {
        page_table.clear();
        return 0;
    }

    int MemorySpace::mprotect(uint32_t loc, uint32_t size, uint8_t perms)
    {
        uint32_t end = ROUND_UP_PAGE(loc + size) >> HAMSTER_PAGE_SIZE_BITS;
        loc >>= HAMSTER_PAGE_SIZE_BITS;

        for (; loc < end; ++loc)
        {
            if (page_table.get_page_direct(loc) == PageTable::PAGE_ID_UNUSED)
                continue;

            if (page_manager.set_permissions(page_table.get_page_direct(loc), perms) < 0)
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
        uint32_t end = ROUND_UP_PAGE(loc + size) >> HAMSTER_PAGE_SIZE_BITS;
        loc >>= HAMSTER_PAGE_SIZE_BITS;

        uint8_t perms = 07; // Start with all

        for (; loc < end; ++loc)
        {
            auto page_id = page_table.get_page_direct(loc);
            if (page_id == PageTable::PAGE_ID_UNUSED)
                continue;

            int8_t page_perms = page_manager.get_permissions(page_id);
            if (page_perms < 0)
                return -1;

            perms &= page_perms;
        }
        return perms;
    }

    void *MemorySpace::make_iterator(uint32_t addr)
    {
        uint32_t id = page_table.get_page(addr);
        if (id == PageTable::PAGE_ID_UNUSED)
        {
            error = H_EFAULT;
            return nullptr;
        }
        return page_manager.make_iterator(id, addr % HAMSTER_PAGE_SIZE);
    }

    const void *MemorySpace::make_iterator_read(uint32_t addr)
    {
        uint32_t id = page_table.get_page(addr);
        if (id == PageTable::PAGE_ID_UNUSED)
        {
            error = H_EFAULT;
            return nullptr;
        }
        return page_manager.make_iterator_read(id, addr % HAMSTER_PAGE_SIZE);
    }

    int MemorySpace::futex_wait(uint32_t addr, void (*callback)())
    {
        assert(addr % 4 == 0);

        uint32_t page_id = page_table.get_page(addr);
        uint16_t offset = addr % HAMSTER_PAGE_SIZE;

        if (page_id == PageTable::PAGE_ID_UNUSED)
        {
            error = H_EFAULT;
            return -1;
        }

        return page_manager.futex_wait(page_id, offset, callback);
    }

    int MemorySpace::futex_wake(uint32_t addr, uint32_t count)
    {
        assert(addr % 4 == 0);

        uint32_t page_id = page_table.get_page(addr);
        uint16_t offset = addr % HAMSTER_PAGE_SIZE;

        if (page_id == PageTable::PAGE_ID_UNUSED)
        {
            error = H_EFAULT;
            return -1;
        }

        return page_manager.futex_wake(page_id, offset, count);
    }

    int MemorySpace::futex_requeue(uint32_t wake_addr, uint32_t wake_count, uint32_t requeue_addr, uint32_t requeue_count)
    {
        assert(wake_addr % 4 == 0);
        assert(requeue_addr % 4 == 0);

        uint32_t wake_page_id = page_table.get_page(wake_addr);
        uint16_t wake_offset = wake_addr % HAMSTER_PAGE_SIZE;

        uint32_t requeue_page_id = page_table.get_page(requeue_addr);
        uint16_t requeue_offset = requeue_addr % HAMSTER_PAGE_SIZE;

        if (wake_page_id == PageTable::PAGE_ID_UNUSED ||
            requeue_page_id == PageTable::PAGE_ID_UNUSED)
        {
            error = H_EFAULT;
            return -1;
        }

        return page_manager.futex_requeue(wake_page_id, wake_offset, wake_count, requeue_page_id, requeue_offset, requeue_count);
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
} // namespace Hamster

