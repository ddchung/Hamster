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

    uint32_t PageManager::allocate_page(PageEntry *&out, uint8_t perms)
    {
        uint32_t id;
        if (free_pages.empty())
        {
            id = page_table.size();
            page_table.push_back(nullptr);
        }
        else
        {
            id = *free_pages.begin();
            free_pages.pop_front();
        }

        // We can't use `out` because we need the reference
        // to bind to the vector entry
        PageEntry *&entry = page_table[id];
        out = entry;

        // Initialize the page entry
        assert(entry == nullptr);
        entry = alloc<PageEntry>();
        entry->data = nullptr;
        entry->fd = -1;
        entry->eviction_queue_count = 0;
        entry->swapped = 1;
        entry->dirty = 0;
        entry->perms = perms;
        return id;
    }

    uint32_t PageManager::mmap_private(PageEntry *&entry, int fd, int64_t offset, uint8_t perms)
    {
        // Check if the file descriptor is valid
        if (vfs.seek(fd, offset, H_SEEK_SET) < 0)
            return -1;

        uint32_t id = allocate_page(entry, perms);
        if (id == -1)
            return -1;

        entry->fd = fd;
        entry->offset = offset;

        return id;
    }

    PageEntry *PageManager::get_page(uint32_t id)
    {
        return page_table[id];
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
            dealloc(entry);
            if (entry->fd != -1)
                vfs.close(entry->fd);
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

        // Check readability
        if ((entry->perms & PERM_READ) == 0)
        {
            error = EACCES;
            return -1;
        }

        // Read from the page
        ssize_t bytes_read = 0;
        if (addr + size > HAMSTER_PAGE_SIZE)
            size = HAMSTER_PAGE_SIZE - addr;

        memcpy(buf, entry->data + addr, size);
        return bytes_read;
    }

    ssize_t PageManager::try_write(uint32_t id, size_t addr, const void *buf, size_t size)
    {
        PageEntry *entry = page_table[id];
        if (entry->swapped)
            return -1;
        assert(entry->data != nullptr);

        if ((entry->perms & PERM_WRITE) == 0)
        {
            error = EACCES;
            return -1;
        }

        // Write to the page
        ssize_t bytes_written = 0;
        if (addr + size > HAMSTER_PAGE_SIZE)
            size = HAMSTER_PAGE_SIZE - addr;
        
        if (size > 0)
            mark_page_dirty(id);

        memcpy(entry->data + addr, buf, size);
        return bytes_written;
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
            if (vfs.read(entry->fd, entry->data, HAMSTER_PAGE_SIZE) != HAMSTER_PAGE_SIZE)
                return -1;
            // Convert to anonymous mapping, since it behaves like one now
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

        if (_swap_out(id, entry->data) < 0)
            return -1;
        
        entry->swapped = 1;
        _free(entry->data);
        entry->data = nullptr;

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
            PageEntry *new_entry = alloc<PageEntry>(1, entry);

            // copy data
            new_entry->data = (uint8_t *)_malloc(HAMSTER_PAGE_SIZE);
            memcpy(new_entry->data, entry->data, HAMSTER_PAGE_SIZE);

            // set refcount
            new_entry->refcount = 1;

            entry = new_entry;
        }

        entry->dirty = 1;
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
