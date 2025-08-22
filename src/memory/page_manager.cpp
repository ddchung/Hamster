// Hamster page manager implementation

#include <memory/page_manager.hpp>
#include <memory/allocator.hpp>
#include <filesystem/vfs.hpp>
#include <platform/platform.hpp>
#include <errno/errno.h>
#include <cstring>

namespace Hamster
{
    PageManager::~PageManager()
    {
        for (PageEntry *entry : page_table)
        {
            if (entry && --entry->refcount == 0)
            {
                _free(entry->data);
                dealloc(entry);
            }
        }
        page_table.clear();
    }

    uint32_t PageManager::allocate_page(uint8_t perms)
    {
        uint32_t id;
        if (free_pages.empty())
        {
            id = page_table.size();
            page_table.push_back(nullptr);
        }
        else
        {
            id = free_pages.front();
            free_pages.pop_front();
        }

        PageEntry *&entry = page_table[id];
        
        // Initialize the page entry
        assert(entry == nullptr);
        entry = alloc<PageEntry>();
        entry->data = nullptr;
        entry->fd = -1;
        entry->eviction_queue_count = 0;
        entry->swapped = 1;
        entry->dirty = 0;
        entry->perms = perms;
        entry->refcount = 1;
        entry->zero = 1;
        return id;
    }

    uint32_t PageManager::mmap_private(int fd, int64_t offset, uint8_t perms)
    {
        // Check if the file descriptor is valid
        if (vfs.seek(fd, offset, H_SEEK_SET) < 0)
            return 0;

        uint32_t id = allocate_page(perms);
        PageEntry *entry = page_table[id];
        entry->fd = fd;
        entry->offset = offset;

        // clear zero, since this is a file mapping, 
        // and swap_in skips everything else if `zero` is set
        entry->zero = 0;

        return id;
    }

    void PageManager::free_page(uint32_t id)
    {
        if (id >= page_table.size())
            return;

        // Free the page entry
        PageEntry *&entry = page_table[id];

        if (--entry->refcount == 0)
        {
            _free(entry->data);
            if (entry->fd != -1)
                vfs.close(entry->fd);
            dealloc(entry);
        }
        entry = nullptr;
        free_pages.push_back(id);
    }

    ssize_t PageManager::try_read(uint32_t id, size_t addr, void *buf, size_t size)
    {
        PageEntry *entry = page_table[id];
        if (entry->swapped)
            return -1;
        assert(entry->data != nullptr);
        assert(addr < HAMSTER_PAGE_SIZE);

        // Check readability
        if ((entry->perms & PERM_READ) == 0)
        {
            error = EACCES;
            return -1;
        }

        // Read from the page
        if (addr + size > HAMSTER_PAGE_SIZE)
            size = HAMSTER_PAGE_SIZE - addr;

        memcpy(buf, entry->data + addr, size);
        return size;
    }

    ssize_t PageManager::try_write(uint32_t id, size_t addr, const void *buf, size_t size)
    {
        PageEntry *entry = page_table[id];
        if (entry->swapped)
            return -1;
        assert(entry->data != nullptr);
        assert(addr < HAMSTER_PAGE_SIZE);

        if ((entry->perms & PERM_WRITE) == 0)
        {
            error = EACCES;
            return -1;
        }

        // Write to the page
        if (addr + size > HAMSTER_PAGE_SIZE)
            size = HAMSTER_PAGE_SIZE - addr;
        
        if (size > 0)
        {
            mark_page_dirty(id);
            
            // re-fetch the entry, as `make_page_dirty` may split COW pages
            entry = page_table[id];
        }

        memcpy(entry->data + addr, buf, size);
        return size;
    }

    uint32_t PageManager::copy(uint32_t id)
    {
        PageEntry *entry = page_table[id];
        entry->refcount++;

        // Find a new id
        
        uint32_t new_id;
        if (free_pages.empty())
        {
            new_id = page_table.size();
            page_table.push_back(nullptr);
        }
        else
        {
            new_id = free_pages.front();
            free_pages.pop_front();
        }

        page_table[new_id] = entry;
        return new_id;
    }

    int PageManager::swap_in(uint32_t id)
    {
        PageEntry *entry = page_table[id];
        if (!entry->swapped)
            return -1;
        
        // Swap out pages if more memory is needed
        if (should_evict() == 1)
            evict_pages();

        // Swap in the page

        assert(entry->data == nullptr);

        // Ensure that if it is a file mapping, the file descriptor is OK
        if (entry->fd != -1 && vfs.seek(entry->fd, entry->offset, H_SEEK_SET) < 0)
            return -1;

        // use _malloc instead of alloc<uint8_t> to avoid
        // the allocator's overhead
        entry->data = (uint8_t *)_malloc(HAMSTER_PAGE_SIZE);
        entry->swapped = 0;

        if (entry->eviction_queue_count <= 3)
        {
            eviction_queue.push_back(id);
            ++entry->eviction_queue_count;
        }

        if (entry->zero)
        {
            entry->zero = 0;
            memset(entry->data, 0, HAMSTER_PAGE_SIZE);
            return 0;
        }

        if (entry->fd == -1)
        {
            // Anonymous mapping
            // Swap in the page
            _swap_in(id, entry->data);
            return 0;
        }
        else
        {
            // Private file mapping
            if (vfs.read(entry->fd, entry->data, HAMSTER_PAGE_SIZE) < 0)
                return -1;
            // Convert to anonymous mapping, since it behaves like one now
            vfs.close(entry->fd);
            entry->fd = -1;
            return 0;
        }
    }

    int PageManager::swap_out(uint32_t id)
    {
        PageEntry *entry = page_table[id];
        if (entry->swapped)
            return -1;
        
        assert(entry->data != nullptr);

        if (entry->dirty && _swap_out(id, entry->data) < 0)
            return -1;
        
        entry->swapped = 1;
        entry->dirty = 0;
        _free(entry->data);
        entry->data = nullptr;

        return 0;
    }

    int PageManager::set_permissions(uint32_t id, uint8_t perms)
    {
        page_table[id]->perms = perms;
        return 0;
    }

    void PageManager::mark_page_dirty(uint32_t id)
    {
        PageEntry *&entry = page_table[id];
        assert(!entry->swapped);

        if (entry->refcount > 1)
        {
            // copy the page
            entry->refcount--;

            // copy-construct
            PageEntry *new_entry = alloc<PageEntry>(1, *entry);

            // copy data
            new_entry->data = (uint8_t *)_malloc(HAMSTER_PAGE_SIZE);
            memcpy(new_entry->data, entry->data, HAMSTER_PAGE_SIZE);

            // set refcount
            new_entry->refcount = 1;

            entry = new_entry;
        }

        entry->dirty = 1;
    }

    uint8_t PageManager::get_permissions(uint32_t id) const
    {
        return page_table[id]->perms;
    }

    bool PageManager::is_id_valid(uint32_t id) const
    {
        return id < page_table.size() && page_table[id] != nullptr;
    }

    int PageManager::should_evict()
    {
        return _get_free_memory() < HAMSTER_TARGET_FREE_RAM ? 1 : 0;
    }

    void PageManager::evict_pages()
    {
        while (should_evict())
        {
            if (eviction_queue.empty())
                break;

            uint32_t victim = eviction_queue.front();
            eviction_queue.pop_front();

            assert(victim < page_table.size());

            PageEntry *entry = page_table[victim];
            if (!entry || --entry->eviction_queue_count > 0)
                continue;

            // Evict the page
            swap_out(victim);
        }
    }
} // namespace Hamster
