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
        for (auto &entry : page_table)
        {
            _free(entry.data);
        }
        page_table.clear();
    }

    uint32_t PageManager::allocate_page(PageEntry *&entry)
    {
        uint32_t id;
        if (free_pages.empty())
        {
            id = page_table.size();
            page_table.emplace_back();
        }
        else
        {
            id = *free_pages.begin();
            free_pages.pop_front();
        }

        entry = &page_table[id];

        // Initialize the page entry
        entry->data = nullptr;
        entry->fd = -1;
        entry->eviction_queue_count = 0;
        entry->swapped = 1;
        entry->dirty = 0;
        entry->used = 1;
        entry->cow = 0;
        return id;
    }

    uint32_t PageManager::mmap_private(PageEntry *&entry, int fd, int64_t offset)
    {
        // Check if the file descriptor is valid
        if (vfs.seek(fd, offset, H_SEEK_SET) < 0)
            return -1;

        uint32_t id = allocate_page(entry);
        if (id == -1)
            return -1;

        entry->fd = fd;
        entry->offset = offset;

        return id;
    }

    PageEntry *PageManager::get_page(uint32_t id)
    {
        if (id >= page_table.size())
            return nullptr;
        return &page_table[id];
    }

    void PageManager::free_page(uint32_t id)
    {
        if (id >= page_table.size())
            return;

        // Free the page entry
        auto &entry = page_table[id];
        if (!entry.used)
            return;

        _free(entry.data);
        if (entry.fd != -1)
            vfs.close(entry.fd);
        entry = {.used = 0};
        free_pages.push_back(id);
    }

    ssize_t PageManager::try_read(uint32_t id, size_t addr, void *buf, size_t size)
    {
        if (id >= page_table.size())
            return -1;

        auto &entry = page_table[id];
        if (!entry.used || entry.swapped)
            return -1;
        assert(entry.data != nullptr);

        // Read from the page
        ssize_t bytes_read = 0;
        if (addr + size > HAMSTER_PAGE_SIZE)
            size = HAMSTER_PAGE_SIZE - addr;

        memcpy(buf, entry.data + addr, size);
        return bytes_read;
    }

    ssize_t PageManager::try_write(uint32_t id, size_t addr, const void *buf, size_t size)
    {
        if (id >= page_table.size())
            return -1;

        auto &entry = page_table[id];
        if (!entry.used || entry.swapped)
            return -1;
        assert(entry.data != nullptr);

        // Write to the page
        ssize_t bytes_written = 0;
        if (addr + size > HAMSTER_PAGE_SIZE)
            size = HAMSTER_PAGE_SIZE - addr;
        
        if (size > 0)
            entry.dirty = 1; // Mark the page as dirty if we write to it

        memcpy(entry.data + addr, buf, size);
        return bytes_written;
    }

    int PageManager::swap_in(uint32_t id)
    {
        if (id >= page_table.size())
            return -1;

        auto &entry = page_table[id];
        if (!entry.used || !entry.swapped)
            return -1;
        
        // Swap out pages if more memory is needed
        if (should_evict() == 1)
            evict_pages();

        // Swap in the page

        assert(entry.data == nullptr);

        // Ensure that if it is a file mapping, the file descriptor is OK
        if (entry.fd != -1 && vfs.seek(entry.fd, entry.offset, H_SEEK_SET) < 0)
            return -1;

        // use _malloc instead of alloc<uint8_t> to avoid
        // the allocator's overhead
        entry.data = (uint8_t *)_malloc(HAMSTER_PAGE_SIZE);
        entry.swapped = 0;

        if (entry.eviction_queue_count <= 3)
        {
            eviction_queue.push_back(id);
            ++entry.eviction_queue_count;
        }

        if (entry.fd == -1)
        {
            // Anonymous mapping
            // Swap in the page
            _swap_in(id, entry.data);
            return 0;
        }
        else if (!entry.shared)
        {
            // Private file mapping
            if (vfs.read(entry.fd, entry.data, HAMSTER_PAGE_SIZE) != HAMSTER_PAGE_SIZE)
                return -1;
            return 0;
        }
        else
        {
            // Shared file mapping
            // Not supported yet
            error = ENOTSUP;
            return -1;
        }
    }

    bool PageManager::is_id_valid(uint32_t id) const
    {
        return id < page_table.size() && page_table[id].used;
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
            if (--page_table[victim].eviction_queue_count == 0)
                free_page(victim);
        }
    }
} // namespace Hamster
