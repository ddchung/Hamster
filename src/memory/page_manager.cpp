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
        entry->swapped = 1;
        entry->dirty = 0;
        entry->perms = perms;
        entry->refcount = 1;
        entry->zero = 1;
        entry->shared = 0;
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

    void PageManager::make_shared(uint32_t id)
    {
        PageEntry *entry = page_table[id];

        entry->shared = 1;

        // Note: clear swapped on shared file mappings since operations
        //       simply get forwarded to the file. It also won't ever
        //       get swapped out since we didn't put it in the queue,
        //       and this is good because swapping out would be meaningless
        if (entry->fd)
            entry->swapped = 0;
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

            // Remove any futex waiters still on the page
            futex_waiters.remove_if([=](const FutexWaiter &waiter){
                return waiter.page == entry;
            });

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

        eviction_queue.push_back(id);

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
            assert(entry->shared == 0);
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

        // Don't split shared pages to properly implement the sharing
        if (entry->refcount > 1 && !entry->shared)
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

    bool PageManager::is_id_valid(uint32_t id) const
    {
        return id < page_table.size() && page_table[id] != nullptr;
    }

    int PageManager::futex_wait(uint32_t id, size_t addr, void (*callback)())
    {
        PageEntry *entry = page_table[id];
        assert(entry);
        assert(addr < HAMSTER_PAGE_SIZE);

        // Shared file mappings are not supported by futexes
        if (entry->fd && entry->shared)
        {
            error = EPERM;
            return -1;
        }

        if (futex_waiters.size() >= HAMSTER_MAX_FUTEXES)
        {
            error = ENOMEM;
            return -1;
        }

        futex_waiters.emplace_back(callback, entry, addr);

        return 0;
    }

    int PageManager::futex_wake(uint32_t id, size_t addr, uint32_t count)
    {
        PageEntry *entry = page_table[id];
        assert(entry);
        assert(addr < HAMSTER_PAGE_SIZE);

        if (entry->fd && entry->shared)
        {
            error = EPERM;
            return -1;
        }

        size_t woken = 0;
        futex_waiters.remove_if([entry, addr, count, &woken](const FutexWaiter &waiter){
            if (waiter.page == entry && waiter.offset == addr && woken++ < count)
            {
                waiter.callback();
                return true;
            }
            return false;
        });

        return woken;
    }

    int PageManager::futex_requeue(uint32_t wake_id, size_t wake_addr, uint32_t wake_count, uint32_t requeue_id, uint32_t requeue_addr, uint32_t requeue_count)
    {
        PageEntry *entry = page_table[wake_id];
        assert(entry);
        assert(wake_addr < HAMSTER_PAGE_SIZE);

        PageEntry *requeue_entry = page_table[requeue_id];
        assert(requeue_entry);
        assert(requeue_addr < HAMSTER_PAGE_SIZE);

        if ((entry->fd && entry->shared) || (requeue_entry->fd && requeue_entry->shared))
        {
            error = EPERM;
            return -1;
        }

        int processed = 0;

        for (auto it = futex_waiters.begin(); it != futex_waiters.end();)
        {
            FutexWaiter &waiter = *it;

            if (waiter.page != entry || waiter.offset != wake_addr)
                continue;
            
            if ((uint32_t)processed < wake_count)
            {
                // Wake up
                waiter.callback();

                futex_waiters.erase(it++);
            }
            else if (processed - wake_count < requeue_count)
            {
                // Requeue
                waiter.page = requeue_entry;
                waiter.offset = requeue_addr;
                
                // move to end
                futex_waiters.splice(futex_waiters.end(), futex_waiters, it++);
            }
            else
            {
                // done
                break;
            }
        }

        return processed;
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
            if (!entry)
                continue;

            // Evict the page
            swap_out(victim);
        }
    }
} // namespace Hamster
