// Hamster memory space

#pragma once

#include <memory/page_table.hpp>
#include <memory/page_manager.hpp> // for PERM_*
#include <memory/circular_buffer.hpp>
#include <sys/types.h>
#include <cstdint>
#include <cstddef>

namespace Hamster
{
    class MemorySpace
    {
        struct FreeRange
        {
            uint32_t addr, size;
        };
    public:
        MemorySpace() = default;
        MemorySpace(const MemorySpace &other);
        MemorySpace &operator=(const MemorySpace &other);
        MemorySpace(MemorySpace &&other) = default;
        MemorySpace &operator=(MemorySpace &&other) = default;
        ~MemorySpace() = default;
        // Copy to/from external buffers into the virtual memory
        // These will fail if the memory is not mapped

        int memcpy(void *dest, uint32_t src, uint32_t len);
        int memcpy(uint32_t dest, const void *src, uint32_t len);

        // backwards compatibility
        int memset(uint32_t addr, uint8_t value, uint32_t len);

        // copy, but if it doesn't exist, map an anonymous page
        int memcpy_alloc(uint32_t dest, const void *src, uint32_t len);

        int memset_alloc(uint32_t addr, uint8_t value, uint32_t len);

        // faster read/write that require small, aligned objects
        int fast_read_aligned(uint32_t addr, void *buf, size_t size);
        int fast_write_aligned(uint32_t addr, const void *buf, size_t size);

        // fast read of 4 aligned bytes, that checks for both read and exec permissions
        int fast_fetch_aligned(uint32_t addr, uint32_t &out);

        /**
         * @brief Read from a memory region, up until, and including, a zero byte
         * @param addr The address of the memory region to read from
         * @return A newly allocated buffer containing the data on success, or nullptr on failure and set `error`
         * @note May be used to get a C-string
         */
        char *read_until_zero(uint32_t addr);

        // backwards compatibility
        char *get_string(uint32_t addr)
        { return read_until_zero(addr); }

        /**
         * @brief Check if a memory region is mapped
         * @param loc The starting address of the memory region
         * @param size The size of the memory region
         * @return 1 if it is mapped, 0 if it's not, -1 on fail and set `error`
         * @note `loc` is rounded down to the page boundary, while size is rounded up to the page boundary
         */
        int is_mapped(uint32_t loc, uint32_t size) const;

        /**
         * @brief Check how many pages in a region are mapped
         * @param loc The starting address of the memory region
         * @param size The size of the memory region
         * @return The number of mapped pages, or -1 on failure
         * @note `loc` is rounded down to the page boundary, while size is rounded up to the page boundary
         */
        ssize_t how_many_mapped(uint32_t loc, uint32_t size) const;

        /**
         * @brief Map a new anonymous memory region, at the specified region
         * @param loc The starting address of the memory region to map
         * @param size The size of the memory region to map, rounded up to the nearest page size
         * @param perms The permissions for the memory region, composed by bitwise-ORing `PERM_*` flags
         * @return 0 on success, or -1 and set `error` on failiure
         * @note `loc`, if specified, is rounded down to the nearest page boundary
         */
        int map_anonymous(uint32_t loc, uint32_t size, uint8_t perms);

        /**
         * @brief Map a new private file, at the specified location
         * @param loc The starting address of the memory region to map
         * @param fd The file descriptor of the file to map
         * @param offset The offset within the file to map. Not rounded
         * @param size The size of the memory region to map, rounded up to the nearest page size
         * @param perms The permissions for the memory region, composed by bitwise-ORing `PERM_*` flags
         * @return 0 on success, or -1 and set `error` on failiure
         * @note `loc`, if specified, is rounded down to the nearest page boundary
         */
        int map_private_file(uint32_t loc, int fd, uint32_t offset, uint32_t size, uint8_t perms);

        /**
         * @brief Change the permissions of a region
         * @param loc The starting address of the memory region
         * @param size The size of the memory region, rounded up to the page boundary
         * @param perms The new permissions for the memory region, composed by bitwise-ORing `PERM_*` flags
         * @return 0 on success, or -1 and set `error` on failiure
         * @note If the region is not completely mapped, this will skip over the holes
         */
        int mprotect(uint32_t loc, uint32_t size, uint8_t perms);

        /**
         * @brief Unmap a region
         * @param loc The starting address of the memory region to unmap
         * @param size The size of the memory region to unmap, rounded up to the page boundary
         * @return 0 on success, or -1 and set `error` on failiure
         * @note This will still succeed if some, but not all, of the pages in the region aren't mapped already
         */
        int unmap(uint32_t loc, uint32_t size);

        /**
         * @brief Unmap all regions
         * @return 0 on success, or -1 and set `error` on failiure
         */
        int unmap_all();

        /**
         * @brief Set the next mmap address
         * @param addr The address to set
         * @note This will be used and incremented if all free ranges are exhausted
         */
        void set_next_mmap(uint32_t addr)
        { next_mmap = addr; }

        /**
         * @brief Get the permissions of a page range
         * @param loc The starting address of the memory region
         * @param size The size of the memory region
         * @return The permissions of the memory region, or -1 on failure
         * @note Permissions are a mask of `PERM_*` flags
         * @note The returned permissions is the conjunction of all page permissions in the range
         * @note This will fail if any of the pages in the range aren't mapped
         */
        int8_t get_permissions(uint32_t loc, uint32_t size = 1);

        /**
         * @brief Allocate a new free region
         * @param size The size of the region
         * @return The address of the newly allocated region
         * @note Be sure to map with the exact address returned, and the exact size specified,
         *     * so that on unmap, the resources can be properly freed.
         */
        uint32_t allocate(uint32_t size);

    private:
        PageTable page_table;
        CircularBuffer<FreeRange> free_ranges;
        uint32_t next_mmap = 0;

        ssize_t do_read(uint32_t addr, void *buf, size_t size);
        ssize_t do_write(uint32_t addr, const void *buf, size_t size);

        // Called on unmap
        void deallocate(uint32_t addr, uint32_t size);
    };
} // namespace Hamster

