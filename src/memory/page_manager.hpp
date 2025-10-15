// Hamster page manager

#pragma once

#include <memory/stl_sequential.hpp>
#include <memory/stl_map.hpp>
#include <memory/stl_set.hpp>
#include <filesystem/vfs.hpp>
#include <errno/errno.h>
#include <sys/types.h>
#include <cstdint>
#include <cstddef>
#include <cassert>
#include <cstring>

namespace Hamster
{
    constexpr int PERM_READ = 0b100;
    constexpr int PERM_WRITE = 0b010;
    constexpr int PERM_EXEC = 0b001;

    struct FileMappingFD
    {
        int fd;
        uint32_t refcount;
    };

    class PageManager
    {
        struct PageEntry
        {
            int64_t offset;
            uint8_t *data;
            FileMappingFD *fd;
            uint32_t refcount : 4;
            uint32_t swapped : 1; // Note: a lazy-loaded file mapping is considered swapped
            uint32_t dirty : 1;
            uint32_t perms : 3;
            uint32_t zero : 1;
            uint32_t shared : 1; // Whether this page is a shared mapping
        };

        struct FutexWaiter
        {
            void (*callback)();
            PageEntry *page;
            uint16_t offset;
        };
    public:
        PageManager()
        { page_table.reserve(0xFFFF); }
        ~PageManager();
        PageManager(const PageManager &) = delete;
        PageManager &operator=(const PageManager &) = delete;
        PageManager(PageManager &&) = delete;
        PageManager &operator=(PageManager &&) = delete;

        /**
         * @brief Get a new page
         * @param perms The permissions of the page. Defaults to RW
         * @return The ID of the new page
         */
        uint32_t allocate_page(uint8_t perms = PERM_READ | PERM_WRITE);

        /**
         * @brief Get a new page that has a private file mapping
         * @param fd The file descriptor of the file to map. Takes ownership.
         * @param offset The offset within the file to map
         * @param perms The permissions of the page. Defaults to RW
         * @return The ID of the new page
         * @note This takes ownership of the file descriptor
         */
        uint32_t mmap_private(FileMappingFD *fd, int64_t offset, uint8_t perms = PERM_READ | PERM_WRITE);

        /**
         * @brief Make a page shared
         * @param id The id of the page to convert to shared mapping
         * @note This must be called on a page right after it has been created, through `allocate_page` or `mmap_private`
         */
        void make_shared(uint32_t id);

        /**
         * @brief Copy a page with copy-on-write management
         * @param id The ID of the page to copy
         * @return The ID of the new page
         * @note Usage is the same, but the page manager will automatically
         *     * keep track of the copy-and-write state. It is possible to
         *     * have up to 16 references to the same page.
         * @note If the page has a private file mapping, we will
         */
        uint32_t copy(uint32_t id);

        /**
         * @brief Check whether a page ID is valid
         * @param id The ID of the page
         * @return true if the page ID is valid, false otherwise
         */
        bool is_id_valid(uint32_t id) const;

        /**
         * @brief Free a page by ID
         * @param id The ID of the page
         */
        void free_page(uint32_t id);

        /**
         * @brief Read from the page
         * @param id The id of the page to read
         * @param addr The offset in the page
         * @param buf The buffer to read into
         * @param size How many bytes to read
         * @return The number of bytes read. might be less than size
         */
        ssize_t read(uint32_t id, size_t addr, void *buf, size_t size)
        {
            PageEntry *entry = page_table[id];
            if (entry->swapped)
                swap_in(id);
            assert(addr < HAMSTER_PAGE_SIZE);
            
            // Check readability
            if ((entry->perms & PERM_READ) == 0)
            {
                error = H_EACCES;
                return -1;
            }
            
            // Read from the page
            if (addr + size > HAMSTER_PAGE_SIZE)
            size = HAMSTER_PAGE_SIZE - addr;
            
            if HAMSTER_UNLIKELY(entry->fd)
            {
                // Note: private file mappings get captured and converted to anon mappings
                //       by swap_in
                assert(entry->shared);
                vfs.seek(entry->fd->fd, entry->offset + addr, H_SEEK_SET);
                return vfs.read(entry->fd->fd, buf, size);
            }
            else
            {
                assert(entry->data != nullptr);
                memcpy(buf, entry->data + addr, size);
            }
            return size;
        }

        /**
         * @brief Write to the page
         * @param id The id of the page
         * @param addr The offset in the page
         * @param buf The data to write
         * @param size How many bytes to write
         * @return Number of bytes read. Might be less than size
         */
        ssize_t write(uint32_t id, size_t addr, const void *buf, size_t size)
        {
            PageEntry *entry = page_table[id];
            if (entry->swapped)
                swap_in(id);
            assert(addr < HAMSTER_PAGE_SIZE);

            if ((entry->perms & PERM_WRITE) == 0)
            {
                error = H_EACCES;
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

            if HAMSTER_UNLIKELY(entry->fd)
            {
                // Note: private file mappings get captured and converted to anon mappings
                //       by swap_in
                assert(entry->shared);
                vfs.seek(entry->fd->fd, entry->offset + addr, H_SEEK_SET);
                return vfs.write(entry->fd->fd, buf, size);
            }
            else
            {
                assert(entry->data != nullptr);
                memcpy(entry->data + addr, buf, size);
            }
            return size;
        }

        /**
         * @brief Get an iterator (4-byte fast version)
         * @param id The id of the page
         * @param addr The relative offset within the page. Must be aligned to 4 bytes
         * @return The iterator
         * @note Swaps in the page if necessary
         * @note Page must be executable or readable
         * @note Page must not be a shared file mapping
         */
        uint32_t *make_iterator_fast(uint32_t id, size_t addr)
        {
            PageEntry *entry = page_table[id];
            if (entry->fd && entry->shared)
                return nullptr;
            if (entry->swapped)
                swap_in(id);
            assert(entry->data != nullptr);
            assert(addr < HAMSTER_PAGE_SIZE);
            assert(addr % 4 == 0);
            assert(entry->perms & (PERM_EXEC | PERM_READ));

            return (uint32_t *)(entry->data + addr);
        }

        /**
         * @brief Make a single byte iterator
         * @param id The id of the page
         * @param addr The address within the page
         * @return The iterator, or -1 on error and set `error`
         * @note Page must not be a shared file mapping
         * @note Page must be readable
         * @note Swaps in the page if needed
         */
        uint8_t *make_iterator_1(uint32_t id, size_t addr);

        /**
         * @brief Set the permissions of a page
         * @param id The ID of the page
         * @param perms The new permissions for the page
         * @return 0 on success, or -1 on error
         */
        int set_permissions(uint32_t id, uint8_t perms);

        /**
         * @brief Swap in a page
         * @param id The ID of the page
         * @return 0 on success, or -1 on error
         * @note This will also evict the least recently used page if necessary
         */
        int swap_in(uint32_t id);
        
        /**
         * @brief Swap out a page
         * @param id The ID of the page
         * @return 0 on success, -1 on error
         */
        int swap_out(uint32_t id);

        /**
         * @brief Mark a page as dirty
         * @param id The ID of the page
         */
        void mark_page_dirty(uint32_t id);

        /**
         * @brief Get the permissions of a page
         * @return The permissions of a page, bitmask of PERM_READ, PERM_WRITE, PERM_EXEC
         */
        uint8_t get_permissions(uint32_t id) const
        {
            return page_table[id]->perms;
        }

        /**
         * @brief Perform a futex wait operation
         * @param id The id of the page
         * @param addr The offset in the page
         * @param callback The callback to call when woken up
         * @return 0 on success, -1 on error and set `error`
         * @warning Shared file mappings do not support futexes
         */
        int futex_wait(uint32_t id, size_t addr, void (*callback)());

        /**
         * @brief Perform a futex wake operation
         * @param id The id of the page
         * @param addr The offset in the page
         * @param count How many waiters to ake
         * @note This will wake up (call the callback) of at most `count`
         *       waiters waiting on the specified futex word
         * @return The number of waiters woken up, -1 on error and set `error`
         * @warning Shared file mappings do not support futexes
         */
        int futex_wake(uint32_t id, size_t addr, uint32_t count);

        /**
         * @brief Do a futex requeue
         * @param wake_id The id of the initial futex
         * @param wake_addr The offset in that page
         * @param wake_count How many waiters to wake
         * @param requeue_id The id of the futex to requeue to
         * @param requeue_addr The offset in that page
         * @param requeue_count The amount of waiters to requeue
         * @note This will wake up at most `wake_count` waiters on the wake futex.
         *       If there are still more waiters, a maximum of `requeue_count` remaining
         *       waiters will be requeued to the new futex
         * @return The number of waiters waked up or requeued, or -1 on error and set `error`
         */
        int futex_requeue(uint32_t wake_id, size_t wake_addr, uint32_t wake_count, uint32_t requeue_id, uint32_t requeue_addr, uint32_t requeue_count);

    private:
        Vector<PageEntry *> page_table;
        Deque<uint32_t> free_pages; // Free page IDs
        Deque<uint32_t> eviction_queue;
        List<FutexWaiter> futex_waiters;

        int should_evict();
        void evict_pages();
    };

    extern PageManager page_manager;
} // namespace Hamster

