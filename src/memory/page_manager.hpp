// Hamster page manager

#pragma once

#include <memory/stl_sequential.hpp>
#include <memory/stl_map.hpp>
#include <memory/stl_set.hpp>
#include <sys/types.h>
#include <cstdint>
#include <cstddef>

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
        ssize_t try_read(uint32_t id, size_t addr, void *buf, size_t size);

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
        ssize_t try_write(uint32_t id, size_t addr, const void *buf, size_t size);

        // Fast read of 4 bytes from a page, that checks for both read and exec permissions
        // Also swaps in the page if necessary
        int fast_fetch_aligned(uint32_t id, uint32_t addr, uint32_t &out);

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
        uint8_t get_permissions(uint32_t id) const;

    private:
        Vector<PageEntry *> page_table;
        Deque<uint32_t> free_pages; // Free page IDs
        Deque<uint32_t> eviction_queue;

        int should_evict();
        void evict_pages();
    };

    extern PageManager page_manager;
} // namespace Hamster

