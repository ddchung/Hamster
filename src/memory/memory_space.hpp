// Memory Space

#pragma once

#include <memory/page.hpp>
#include <memory/stl_map.hpp>
#include <memory/stl_sequential.hpp>
#include <cstdint>
#include <cstddef>

namespace Hamster
{
    class MemorySpace
    {
    public:
        /**
         * @brief Get a reference to a byte at the given address.
         * @return A reference to the byte at the given address, or a reference to a dummy on error
         * @warning This is NOT a reference to a byte in an array, so do not touch any neighboring bytes
         * @note Also see: Page::get_dummy_byte()
         */
        uint8_t &operator[](uint64_t addr);

        /**
         * @brief Copy a block of memory from the given address to the given buffer.
         * @param addr The address to copy from
         * @param buffer The buffer to copy to
         * @param size The size of the buffer
         * @return 0 on success, -1 on error
         */
        int memcpy(void *buffer, uint64_t addr, size_t size);

        /**
         * @brief Copy a block of memory from the given buffer to the given address.
         * @param addr The address to copy to
         * @param buffer The buffer to copy from
         * @param size The size of the buffer
         * @return 0 on success, -1 on error
         */
        int memcpy(uint64_t addr, const void *buffer, size_t size);

        /**
         * @brief Copy a one block of memory to another
         * @param src The source address
         * @param dst The destination address
         * @param size The size of the block
         * @return 0 on success, -1 on error
         */
        int memcpy(uint64_t src, uint64_t dst, size_t size);

        /**
         * @brief Fill a block of memory with the given value.
         * @param addr The address to fill
         * @param value The value to fill with
         * @param size The size of the block
         * @return 0 on success, -1 on error
         */
        int memset(uint64_t addr, uint8_t value, size_t size);

        /**
         * @brief Get a string from the given address.
         * @param addr The address to get the string from
         * @return A newly allocated string with the same contents, or nullptr on error
         * @note Remember to free the string when done
         * @warning Ensure that the string isn't too long, or the program might run out of memory
         */
        char *get_string(uint64_t addr);

        /**
         * @brief Check if the given address is allocated
         * @param addr The address to check
         * @return true if the address is allocated, false otherwise
         * @note ALL addresses are valid, but they are allocated the first time they are accessed
         */
        bool is_allocated(uint64_t addr);

        /**
         * @brief Deallocate a page at the given address
         * @param addr The address to deallocate
         * @return 0 on success, -1 on error
         * @note This will delete the page and all its contents, but any accesses to the addresses will make a new page
         */
        int deallocate_page(uint64_t addr);

        /**
         * @brief Set permissions for a page range
         * @param addr Any address in the first page of the range
         * @param mode A bitmask of permissions 0b00000rwx
         * @param size The size of the range, in bytes
         * @return 0 on success, -1 on error
         * @note This will change the permissions for all pages that the range occupies
         */
        int set_permissions(uint64_t addr, uint8_t mode, size_t size = 0);

        /**
         * @brief Check if a page range has at least the given permissions
         * @param addr Any address in the first page of the range
         * @param req_perms The required permissions, in a bitmask 0b00000rwx
         * @param size The size of the range, in bytes
         * @return true if all the pages in the range have at least the given permissions, false otherwise
         */
        bool check_permissions(uint64_t addr, uint8_t req_perms, size_t size = 0);

        /**
         * @brief Swap out all the currently swapped in pages
         * @return 0 on success, -1 on error
         */
        int swap_out_all();

    private:
        UnorderedMap<uint64_t, Page> pages;
        List<uint64_t> swapped_on_pages;

        int ensure_page(uint64_t addr);
    };
} // namespace Hamster

