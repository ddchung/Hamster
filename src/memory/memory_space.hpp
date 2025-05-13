// Memory Space

#pragma once

#include <memory/stl_sequential.hpp>
#include <memory/stl_map.hpp>
#include <memory/page.hpp>
#include <cstdint>


namespace Hamster
{
    class MemorySpace
    {
    public:
        static constexpr uint8_t PERM_READ = 1 << 0;
        static constexpr uint8_t PERM_WRITE = 1 << 1;
        static constexpr uint8_t PERM_EXECUTE = 1 << 2;

        /**
         * @brief Set a byte in the memory space
         * @param addr The address to set
         * @param value The value to set
         * @return 0 on success, or -1 on error
         */
        int set(uint64_t addr, uint8_t value);

        /**
         * @brief Get a byte from the memory space
         * @param addr The address to get
         * @return The value at the address, or -1 on error
         */
        int16_t get(uint64_t addr);

        /**
         * @brief Get the number of pages in the memory space
         * @return The number of pages in the memory space
         */
        uint64_t get_page_count();

        /**
         * @brief Copy memory from one address to another
         * @param dest The destination address
         * @param src The source address
         * @param size The size of the memory to copy
         * @return 0 on success, or -1 on error
         */
        int memcpy(uint64_t dest, uint64_t src, uint64_t size);

        /**
         * @brief Copy memory from the memory space to a buffer
         * @param dest The destination buffer
         * @param src The source address
         * @param size The size of the memory to copy
         * @return 0 on success, or -1 on error
         */
        int memcpy_to_buffer(uint8_t *dest, uint64_t src, uint64_t size);

        /**
         * @brief Copy memory from a buffer to the memory space
         * @param dest The destination address
         * @param src The source buffer
         * @param size The size of the memory to copy
         * @return 0 on success, or -1 on error
         */
        int memcpy_from_buffer(uint64_t dest, const uint8_t *src, uint64_t size);

        /**
         * @brief Fill a range of memory with a value
         * @param addr The address to fill
         * @param value The value to fill with
         * @param size The size of the memory to fill
         * @return 0 on success, or -1 on error
         */
        int memset(uint64_t addr, uint8_t value, uint64_t size);

        /**
         * @brief Set the permissions for a page of memory
         * @param addr Any address within the page
         * @param perms The permissions to set
         * @return 0 on success, or -1 on error
         */
        int set_page_perms(uint64_t addr, uint8_t perms);

        /**
         * @brief Check if a range of virtual memory is allocated
         * @param addr The address to check
         * @param size The size of the memory to check, in bytes
         * @return `true` if the memory is allocated, `false` otherwise
         */
        bool is_allocated(uint64_t addr, uint64_t size = 1);

        /**
         * @brief Allocate a page of memory
         * @param addr The address to allocate
         * @param perms The permissions for the page
         * @return 0 on success, or -1 on error
         */
        int allocate_page(uint64_t addr, uint8_t perms = PERM_READ | PERM_WRITE | PERM_EXECUTE);

        /**
         * @brief Allocate a range of memory
         * @param addr The address to allocate
         * @param size The size of the memory to allocate, in bytes
         * @param perms The permissions for the memory
         * @return 0 on success, or -1 on error
         */
        int allocate_range(uint64_t addr, uint64_t size, uint8_t perms = PERM_READ | PERM_WRITE | PERM_EXECUTE);

        /**
         * @brief Deallocate a page of memory
         * @param addr The address to deallocate
         * @return 0 on success, or -1 on error
         */
        int deallocate_page(uint64_t addr);

        /**
         * @brief Manually swap off all pages
         * @return 0 on success, or -1 on error
         * @note There is no function to swap on all pages, as
         *      * the operations automatically swap in pages as needed
         */
        int swap_off_all();
        
        /**
         * @brief Check if a page has the given permissions
         * @param addr The address to check
         * @param perms The permissions to check
         * @return `true` if the page has at least the given permissions, `false` otherwise
         * @note It does not matter if the page is swapped in or out
         */
        bool check_perm(uint64_t addr, uint8_t perms);

    private:
        UnorderedMap<uint64_t, Page> pages;
        Deque<uint64_t> swapped_on_pages;

        int swap_in_page(uint64_t addr);
        int clear_page_from_swap_queue(uint64_t addr);
    };
} // namespace Hamster

