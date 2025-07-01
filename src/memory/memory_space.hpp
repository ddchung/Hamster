// Memory Space

#pragma once

#include <memory/page.hpp>
#include <memory/stl_map.hpp>
#include <memory/stl_sequential.hpp>
#include <cstdint>
#include <cstddef>
#include <unistd.h>

// Protection flags
#define PROT_NONE  0x0
#define PROT_READ  0x1
#define PROT_WRITE 0x2
#define PROT_EXEC  0x4

// Mapping flags
#define MAP_SHARED    0x01
#define MAP_PRIVATE   0x02
#define MAP_FIXED     0x10
#define MAP_ANONYMOUS 0x20

namespace Hamster
{
    class MemorySpace
    {
        // memory mapping
        struct MmapEntry
        {
            uint64_t addr;
            uint64_t size;
            uint64_t offset;
            int fd; // File descriptor, -1 if anonymous mapping
            uint8_t perms : 3; //0brwx
            bool shared : 1;
        };
    public:
        MemorySpace() = default;
        ~MemorySpace() = default;
        MemorySpace(const MemorySpace &);
        MemorySpace &operator=(const MemorySpace &);
        MemorySpace(MemorySpace &&);
        MemorySpace &operator=(MemorySpace &&);

        /**
         * @brief Write to a given byte
         * @param addr The address to write to
         * @param value The value to write
         * @return 0 on success, -1 on error
         */
        int write_byte(uint64_t addr, uint8_t value);

        /**
         * @brief Read a byte from the given address
         * @param addr The address to read from
         * @param out The output variable to store the read byte
         * @return 0 on success, -1 on error
         */
        int read_byte(uint64_t addr, uint8_t &out);

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
        int set_permissions(uint64_t addr, uint8_t mode, size_t size);

        /**
         * @brief Set the permissions for a single page
         * @param addr The address of the page
         * @param mode A bitmask of permissions 0b00000rwx
         * @return 0 on success, -1 on error
         * @note This will change the permissions for the page that the address belongs to
         */
        int set_permissions(uint64_t addr, uint8_t mode);

        /**
         * @brief Check if a page range has at least the given permissions
         * @param addr Any address in the first page of the range
         * @param req_perms The required permissions, in a bitmask 0b00000rwx
         * @param size The size of the range, in bytes
         * @return true if all the pages in the range have at least the given permissions, false otherwise
         */
        bool check_permissions(uint64_t addr, uint8_t req_perms, size_t size);

        /**
         * @brief Check if a single page has at least the given permissions
         * @param addr The address of the page
         * @param req_perms The required permissions, in a bitmask 0b00000rwx
         * @return true if the page has at least the given permissions, false otherwise
         * @note This will return true if the page is not allocated, as it has the default permissions of 0b00000rwx
         */
        bool check_permissions(uint64_t addr, uint8_t req_perms);

        /**
         * @brief Swap out all the currently swapped in pages
         * @return 0 on success, -1 on error
         */
        int swap_out_all();

        /**
         * @brief Map a file to a region of memory
         * @param addr The starting address
         * @param size The size of the region
         * @param perms The permissions of the region. bitmask of 0brwx
         * @param flags The type of mapping. One of MAP_PRIVATE MAP_SHARED MAP_ANONYMOUS
         * @param fd The file descriptor to map. It must support read/write/seek
         * @param offset The offset in the file
         * @return 0 on success, -1 on error
         * @note This takes ownership of `fd`
         * @note This will fail on overlapping map
         */
        int mmap(uint64_t addr, uint64_t size, uint8_t perms, int flags, int fd, uint64_t offset);
        
        /**
         * @brief Unmap a region of memory
         * @param addr The starting address of the region
         * @param size The size of the region
         * @return 0 on success, -1 on error
         * @note This allows partial and multiple unmaps, and reigons not mmap'd are skipped
         */
        int munmap(uint64_t addr, uint64_t size);

    private:
        UnorderedMap<uint64_t, Page> pages;
        List<uint64_t> swapped_on_pages;
        Vector<MmapEntry> mappings;

        int ensure_page(uint64_t addr);
    };
} // namespace Hamster

