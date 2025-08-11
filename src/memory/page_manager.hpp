// Hamster page manager

#pragma once

#include <memory/stl_sequential.hpp>
#include <memory/stl_map.hpp>
#include <memory/stl_set.hpp>
#include <cstdint>
#include <cstddef>

namespace Hamster
{
    struct PageEntry
    {
        int64_t offset;
        uint8_t *data;
        int fd;
        uint8_t eviction_queue_count : 4;
        uint8_t swapped : 1; // Note: a lazy-loaded file mapping is considered swapped
        uint8_t dirty : 1;
        uint8_t used : 1;
        uint8_t cow : 1;
        uint8_t shared : 1; // shared file mapping
    };

    class PageManager
    {
    public:
        PageManager() = default;
        ~PageManager();
        PageManager(const PageManager &) = delete;
        PageManager &operator=(const PageManager &) = delete;
        PageManager(PageManager &&) = delete;
        PageManager &operator=(PageManager &&) = delete;

        /**
         * @brief Get a new page
         * @param entry This pointer will be set to point to the new entry
         * @return The ID of the new page
         */
        uint32_t allocate_page(PageEntry *&entry);

        /**
         * @brief Get a new page that has a private file mapping
         * @param entry This pointer will be set to point to the new entry
         * @param fd The file descriptor of the file to map
         * @param offset The offset within the file to map
         * @return The ID of the new page
         * @note This takes ownership of the file descriptor
         */
        uint32_t mmap_private(PageEntry *&entry, int fd, int64_t offset);

        /**
         * @brief Check whether a page ID is valid
         * @param id The ID of the page
         * @return true if the page ID is valid, false otherwise
         */
        bool is_id_valid(uint32_t id) const;

        /**
         * @brief Get a page by ID
         * @param id The ID of the page
         * @return A pointer to the page entry, or nullptr if not found
         */
        PageEntry *get_page(uint32_t id);

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

        /**
         * @brief Swap in a page
         * @param id The ID of the page
         * @return 0 on success, or -1 on error
         * @note This will also evict the least recently used page if necessary
         */
        int swap_in(uint32_t id);

    private:
        Vector<PageEntry> page_table;
        Deque<uint32_t> free_pages; // Free page IDs
        Deque<uint32_t> eviction_queue;

        int should_evict();
        void evict_pages();
    };

    extern PageManager page_manager;
} // namespace Hamster

