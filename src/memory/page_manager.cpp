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
                if (entry->fd && --entry->fd->refcount == 0)
                {
                    // Note: we cannot close the file descriptors here
                    // since `vfs` might be destroyed before us
                    // but we still have to free the FileMappingFD's
                    dealloc(entry->fd);
                }
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
        entry->fd = nullptr;
        entry->eviction_queue_count = 0;
        entry->swapped = 1;
        entry->dirty = 0;
        entry->perms = perms;
        entry->refcount = 1;
        entry->zero = 1;
        return id;
    }

    uint32_t PageManager::mmap_private(FileMappingFD *fd, int64_t offset, uint8_t perms)
    {
        // Check if the file descriptor is valid
        if (vfs.seek(fd->fd, offset, H_SEEK_SET) < 0)
            return 0;

        uint32_t id = allocate_page(perms);
        PageEntry *entry = page_table[id];
        entry->fd = fd;
        fd->refcount++;
        entry->offset = offset;

        // clear zero, since this is a file mapping, 
        // and swap_in skips everything else if `zero` is set
        entry->zero = 0;

        return id;
    }

    void PageManager::free_page(uint32_t id)
    {
        // Free the page entry
        PageEntry *&entry = page_table[id];

        if (--entry->refcount == 0)
        {
            _free(entry->data);
            if (entry->fd && --entry->fd->refcount == 0)
            {
                vfs.close(entry->fd->fd);
                dealloc(entry->fd);
            }
            dealloc(entry);
        }
        entry = nullptr;
        free_pages.push_back(id);
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
        if (entry->fd && vfs.seek(entry->fd->fd, entry->offset, H_SEEK_SET) < 0)
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

        if (!entry->fd)
        {
            // Anonymous mapping
            // Swap in the page
            _swap_in(id, entry->data);
            return 0;
        }
        else
        {
            // Private file mapping
            if (vfs.read(entry->fd->fd, entry->data, HAMSTER_PAGE_SIZE) < 0)
                return -1;
            // Convert to anonymous mapping, since it behaves like one now

            if (--entry->fd->refcount == 0)
            {
                vfs.close(entry->fd->fd);
                dealloc(entry->fd);
            }
            entry->fd = nullptr;
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
