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
            uint32_t eviction_queue_count : 4;
            uint32_t swapped : 1; // Note: a lazy-loaded file mapping is considered swapped
            uint32_t dirty : 1;
            uint32_t perms : 3;
            uint32_t zero : 1;
            uint32_t shared : 1; // Whether this page is a shared mapping
        };
    public:

        // An optimized instruction fetch iterator that can traverse within a page
        // Has less functionality than your average iterator though
        class InstructionIterator
        {
            friend class PageManager;

            InstructionIterator(uint32_t *it, uint32_t *end)
                : it(it), end(end)
            {
            }

        public:
            
            uint32_t operator *()
            {
                return *it;
            }

            void operator++(int)
            {
                ++it;
            }

            void operator++()
            {
                ++it;
            }

            bool is_end()
            {
                return it == end;
            }
            
        private:
            uint32_t *it;
            uint32_t *end;
        };

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
         * @brief Try reading from a page
         * @param id The ID of the page
         * @param addr The address to read from, starting from the beginning of the page
         * @param buf The buffer to read data into
         * @param size How many bytes to read
         * @return The number of bytes read, or -1 on error
         * @note It may read less bytes than requested, such as when it is at the end of a page
         * @note This will fail if the page is swapped out
         */
        ssize_t try_read(uint32_t id, size_t addr, void *buf, size_t size)
        {
            PageEntry *entry = page_table[id];
            if (entry->swapped)
                return -1;
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
         * @brief Try writing to a page
         * @param id The ID of the page
         * @param addr The address to write to, starting from the beginning of the page
         * @param buf The buffer containing the data to write
         * @param size How many bytes to write
         * @return The number of bytes written, or -1 on error
         * @note It may write less bytes than requested, such as when it is at the end of a page
         * @note This will fail if the page is swapped out
         */
        ssize_t try_write(uint32_t id, size_t addr, const void *buf, size_t size)
        {
            PageEntry *entry = page_table[id];
            if (entry->swapped)
                return -1;
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

        // Same thing as try_{read,write} but automatically swaps in

        ssize_t read(uint32_t id, size_t addr, void *buf, size_t size)
        {
            ssize_t ret = try_read(id, addr, buf, size);
            if (__builtin_expect(ret < 0, 0))
            {
                swap_in(id);
                return try_read(id, addr, buf, size);
            }
            return ret;
        }

        ssize_t write(uint32_t id, size_t addr, const void *buf, size_t size)
        {
            ssize_t ret = try_write(id, addr, buf, size);
            if (__builtin_expect(ret < 0, 0))
            {
                swap_in(id);
                return try_write(id, addr, buf, size);
            }
            return ret;
        }

        /**
         * @brief Get an instruction iterator
         * @param id The id of the page
         * @param addr The relative offset within the page. Must be aligned to 4 bytes
         * @return The iterator
         * @note Swaps in the page if necessary
         * @note Page must be executable
         */
        InstructionIterator make_iterator(uint32_t id, size_t addr)
        {
            PageEntry *entry = page_table[id];
            if (entry->swapped)
                swap_in(id);
            assert(entry->data != nullptr);
            assert(addr < HAMSTER_PAGE_SIZE);
            assert(addr % 4 == 0);
            assert(entry->perms & PERM_EXEC);

            uint32_t *it, *end;

            end = (uint32_t *)(entry->data + HAMSTER_PAGE_SIZE);
            it = (uint32_t *)(entry->data + addr);
            
            return InstructionIterator(it, end);
        }

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

    private:
        Vector<PageEntry *> page_table;
        Deque<uint32_t> free_pages; // Free page IDs
        Deque<uint32_t> eviction_queue;

        int should_evict();
        void evict_pages();
    };

    extern PageManager page_manager;
} // namespace Hamster

