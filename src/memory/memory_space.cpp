// Hamster memory space

#include <memory/page_manager.hpp>
#include <memory/memory_space.hpp>
#include <memory/allocator.hpp>
#include <filesystem/vfs.hpp>
#include <platform/config.hpp>
#include <errno/errno.h>
#include <cstdint>
#include <cassert>


namespace Hamster
{
    namespace
    {
        uint64_t get_page_start(uint64_t addr)
        {
            return addr & ~(HAMSTER_PAGE_SIZE - 1);
        }

        uint64_t get_page_offset(uint64_t addr)
        {
            return addr & (HAMSTER_PAGE_SIZE - 1);
        }
    } // namespace

    MemorySpace::MemorySpace(const MemorySpace &other)
        : MemorySpace()
    {
        // Deep-copy all pages
        for (const auto &page_pair : other.pages)
        {
            const uint64_t page_start = page_pair.first;
            const Page &page = page_pair.second;
            pages[page_start] = Page(page); // Copy the page
        }
        swapped_on_pages = other.swapped_on_pages; // Copy the list of swapped pages
    }

    MemorySpace::~MemorySpace()
    {
        // Clear all pages
        pages.clear();
        swapped_on_pages.clear();

        // Close all memory-mapped files
        for (const auto &entry : mappings)
        {
            if (entry.fd < 0)
                continue; // Skip anonymous mappings
            vfs.close(entry.fd);
        }
    }

    MemorySpace &MemorySpace::operator=(const MemorySpace &other)
    {
        if (this != &other)
        {
            // Clear current pages
            pages.clear();
            swapped_on_pages.clear();

            // Deep-copy all pages
            for (const auto &page_pair : other.pages)
            {
                const uint64_t page_start = page_pair.first;
                const Page &page = page_pair.second;
                pages[page_start] = Page(page); // Copy the page
            }
            swapped_on_pages = other.swapped_on_pages; // Copy the list of swapped pages
        }
        return *this;
    }

    MemorySpace::MemorySpace(MemorySpace &&other)
        : pages(std::move(other.pages)), swapped_on_pages(std::move(other.swapped_on_pages))
    {
        other.pages.clear();
        other.swapped_on_pages.clear();
    }

    MemorySpace &MemorySpace::operator=(MemorySpace &&other)
    {
        if (this != &other)
        {
            pages = std::move(other.pages);
            swapped_on_pages = std::move(other.swapped_on_pages);
            other.pages.clear();
            other.swapped_on_pages.clear();
        }
        return *this;
    }

    int MemorySpace::write_byte(uint64_t addr, uint8_t value)
    {
        // try to find a mapping for the address
        
        MmapEntry *mapping = nullptr;
        for (auto &entry : mappings)
        {
            if (addr >= entry.addr && addr < entry.addr + entry.size &&
                entry.fd >= 0) // Don't consider anonymous mappings, as they will be handled just like
                               // normal memory
            {
                mapping = &entry;
                break;
            }
        }

        if (mapping)
        {
            if (!(mapping->perms & 02)) // 02 = write
            {
                error = EACCES;
                return -1;
            }

            uint64_t offset = addr - mapping->addr + mapping->offset;
            if (vfs.seek(mapping->fd, offset, SEEK_SET) < 0)
            {
                error = EIO;
                return -1;
            }

            if (vfs.write(mapping->fd, &value, 1) < 0)
            {
                error = EIO;
                return -1;
            }

            return 0;
        }
        else
        {
            if (!check_permissions(addr, 02)) // 02 = write
            {
                error = EACCES;
                return -1;
            }

            // No mapping found, write to the page directly
            if (ensure_page(addr) < 0)
            {
                return -1; // Error already set in ensure_page
            }

            pages[get_page_start(addr)][get_page_offset(addr)] = value;
            return 0;
        }
    }

    int MemorySpace::read_byte(uint64_t addr, uint8_t &out)
    {
        // try to find a mapping for the address
        
        MmapEntry *mapping = nullptr;
        for (auto &entry : mappings)
        {
            if (addr >= entry.addr && addr < entry.addr + entry.size &&
                entry.fd >= 0) // Don't consider anonymous mappings, as they will be handled just like
                               // normal memory
            {
                mapping = &entry;
                break;
            }
        }

        if (mapping)
        {
            if (!(mapping->perms & 04)) // 04 = read
            {
                error = EACCES;
                return -1;
            }

            uint64_t offset = addr - mapping->addr + mapping->offset;
            if (vfs.seek(mapping->fd, offset, SEEK_SET) < 0)
            {
                error = EIO;
                return -1;
            }

            if (vfs.read(mapping->fd, &out, 1) < 0)
            {
                error = EIO;
                return -1;
            }

            return 0;
        }
        else
        {
            if (!check_permissions(addr, 04)) // 04 = read
            {
                error = EACCES;
                return -1;
            }

            // No mapping found, read from the page directly
            if (ensure_page(addr) < 0)
            {
                return -1; // Error already set in ensure_page
            }

            out = pages[get_page_start(addr)][get_page_offset(addr)];
            return 0;
        }
    }

    int MemorySpace::memcpy(uint64_t addr, const void *buffer, size_t size)
    {
        if (!check_permissions(addr, 02, size)) // 02 = write
        {
            error = EACCES;
            return -1;
        }

        // TODO: More efficient implementation
        for (size_t i = 0; i < size; i++)
        {
            if (write_byte(addr + i, ((const uint8_t *)buffer)[i]) < 0)
            {
                return -1; // Error already set in write_byte
            }
        }
        return 0;
    }

    int MemorySpace::memcpy(void *buffer, uint64_t addr, size_t size)
    {
        if (!check_permissions(addr, 04, size)) // 04 = read
        {
            error = EACCES;
            return -1;
        }
        
        // TODO: More efficient implementation
        for (size_t i = 0; i < size; i++)
        {
            if (read_byte(addr + i, ((uint8_t *)buffer)[i]) < 0)
            {
                return -1; // Error already set in read_byte
            }
        }
        return 0;
    }

    int MemorySpace::memcpy(uint64_t src, uint64_t dst, size_t size)
    {
        if (!check_permissions(src, 04, size)) // 04 = read
        {
            error = EACCES;
            return -1;
        }
        if (!check_permissions(dst, 02, size)) // 02 = write
        {
            error = EACCES;
            return -1;
        }
        
        // TODO: More efficient implementation
        for (size_t i = 0; i < size; i++)
        {
            uint8_t value;
            if (read_byte(src + i, value) < 0)
            {
                return -1; // Error already set in read_byte
            }
            if (write_byte(dst + i, value) < 0)
            {
                return -1; // Error already set in write_byte
            }
        }
        return 0;
    }

    int MemorySpace::memset(uint64_t addr, uint8_t value, size_t size)
    {
        if (!check_permissions(addr, 02, size)) // 02 = write
        {
            error = EACCES;
            return -1;
        }
        
        for (size_t i = 0; i < size; i++)
        {
            if (write_byte(addr + i, value) < 0)
            {
                return -1; // Error already set in write_byte
            }
        }
        return 0;
    }

    char *MemorySpace::get_string(uint64_t addr)
    {
        if (!check_permissions(addr, 04, 1)) // 04 = read
        {
            error = EACCES;
            return nullptr;
        }

        size_t length = 0;

        uint8_t byte;

        for (uint64_t it = addr; read_byte(it, byte) == 0 && byte != '\0'; it++)
            ++length;
        
        char *str = alloc<char>(length + 1);

        str[length] = '\0'; // Null-terminate the string

        int res = memcpy(str, addr, length);
        if (res < 0)
        {
            dealloc(str);
            return nullptr;
        }
        return str;
    }

    bool MemorySpace::is_allocated(uint64_t addr)
    {
        return pages.find(get_page_start(addr)) != pages.end();
    }

    int MemorySpace::deallocate_page(uint64_t addr)
    {
        auto it = pages.find(get_page_start(addr));
        if (it == pages.end())
        {
            error = EINVAL;
            return -1;
        }

        pages.erase(it);

        // also remove from swapped on pages, if any
        swapped_on_pages.remove(get_page_start(addr));
        return 0;
    }

    int MemorySpace::set_permissions(uint64_t addr, uint8_t mode, size_t size)
    {
        for (uint64_t it = get_page_start(addr); it <= get_page_start(addr + size); it += HAMSTER_PAGE_SIZE)
            if (set_permissions(it, mode) < 0)
            {
                return -1; // Error already set in set_permissions
            }
        return 0;
    }

    int MemorySpace::set_permissions(uint64_t addr, uint8_t mode)
    {
        // Try to find a mapping for the address

        MmapEntry *mapping = nullptr;
        for (auto &entry : mappings)
        {
            if (addr >= entry.addr && addr < entry.addr + entry.size &&
                entry.fd >= 0) // Don't consider anonymous mappings, as they will be handled just like
                               // normal memory
            {
                mapping = &entry;
                break;
            }
        }

        if (mapping)
        {
            mapping->perms = mode & 07;
            return 0;
        }
        else
        {
            auto it = pages.find(get_page_start(addr));
            if (it == pages.end())
            {
                // Not allocated
                error = EFAULT;
                return -1;
            }

            it->second.get_flags() &= ~07; // Clear the permissions
            it->second.get_flags() |= (mode & 07); // Set the new permissions
            return 0;
        }
    }

    bool MemorySpace::check_permissions(uint64_t addr, uint8_t req_perms, size_t size)
    {
        for (uint64_t it = get_page_start(addr); it <= get_page_start(addr + size); it += HAMSTER_PAGE_SIZE)
            if (!check_permissions(it, req_perms))
            {
                return false; // If any page does not have the required permissions, return false
            }
        return true;
    }

    bool MemorySpace::check_permissions(uint64_t addr, uint8_t req_perms)
    {
        // Try to find a mapping for the address

        MmapEntry *mapping = nullptr;
        for (auto &entry : mappings)
        {
            if (addr >= entry.addr && addr < entry.addr + entry.size &&
                entry.fd >= 0) // Don't consider anonymous mappings, as they will be handled just like
                               // normal memory
            {
                mapping = &entry;
                break;
            }
        }

        if (mapping)
        {
            return (mapping->perms & req_perms) == req_perms; // Check if the required permissions are set
        }
        else
        {
            auto it = pages.find(get_page_start(addr));
            if (it == pages.end())
            {
                return true; // Not allocated, so it has the default permissions of 0b00000rwx
            }

            return (it->second.get_flags() & req_perms) == req_perms; // Check the page's permissions
        }
    }

    int MemorySpace::swap_out_all()
    {
        for (auto it = pages.begin(); it != pages.end(); ++it)
        {
            if (it->second.is_swapped())
            {
                continue;
            }
            if (it->second.swap_out() < 0)
            {
                error = EIO;
                return -1;
            }
        }
        swapped_on_pages.clear();
        return 0;
    }

    int MemorySpace::ensure_page(uint64_t addr)
    {
        auto it = pages.find(get_page_start(addr));
        if (it == pages.end())
        {
            // create a new page

            // default construct
            pages[get_page_start(addr)].get_flags() = 07;

            it = pages.find(get_page_start(addr));

            assert(it != pages.end());
        }

        if (it->second.is_swapped() && it->second.swap_in() < 0)
        {
            error = EIO;
            return -1;
        }

        swapped_on_pages.remove(get_page_start(addr));
        swapped_on_pages.push_back(get_page_start(addr));

        while (swapped_on_pages.size() > HAMSTER_CONCUR_PAGES)
        {
            // Need to swap out pages
            uint64_t page_addr = swapped_on_pages.front();
            it = pages.find(page_addr);

            swapped_on_pages.pop_front();

            if (it == pages.end())
                continue;
            
            it->second.swap_out();
        }

        return 0;
    }

    int MemorySpace::mmap(uint64_t addr, uint64_t size, uint8_t perms, int flags, int fd, uint64_t offset)
    {
        // Ensure that no other mapping overlaps with this one
        for (const auto &entry : mappings)
        {
            if (addr < entry.addr + entry.size && entry.addr < addr + size)
            {
                error = EADDRINUSE;
                return -1; // Overlapping mapping found
            }
        }

        if (fd < 0)
        {
            error = EINVAL;
            return -1;
        }
        else if (flags & MAP_ANONYMOUS)
        {
            // Copy the mapping to memory
            if (vfs.seek(fd, offset, SEEK_SET) < 0)
            {
                error = EIO;
                return -1;
            }
            for (uint64_t it = addr; it < addr + size; it += HAMSTER_PAGE_SIZE)
            {
                uint8_t byte = 0;
                if (vfs.read(fd, &byte, 1) < 0)
                {
                    error = EIO;
                    return -1; // Error reading from file
                }
                if (write_byte(it, byte) < 0)
                {
                    error = EFAULT;
                    return -1; // Error writing to memory
                }
            }
        }

        mappings.push_back(MmapEntry{addr, size, offset, fd, (uint8_t)(perms & 07), (flags & MAP_SHARED) != 0});
        return 0;
    }

    int MemorySpace::munmap(uint64_t addr, uint64_t size)
    {
        for (auto it = mappings.begin(); it != mappings.end(); ++it)
        {
            MmapEntry &mapping = *it;
            // Check if it overlaps
            if (addr < mapping.addr + mapping.size && mapping.addr < addr + size)
            {
                // Unmap the overlapping region

                uint64_t start_off = mapping.addr < addr ? addr - mapping.addr : 0;

                // reverse offset
                uint64_t r_end_off = mapping.addr + mapping.size > addr + size ? 
                    mapping.addr + mapping.size - (addr + size) : 0;

                if (start_off == 0 && r_end_off == 0)
                {
                    // Unmap the whole mapping
                    if (mapping.fd >= 0)
                    {
                        vfs.close(mapping.fd);
                    }
                    
                    it = mappings.erase(it);
                }
                else if (start_off > 0 && r_end_off == 0)
                {
                    // Unmap the end of the mapping
                    mapping.size = start_off;
                }
                else if (start_off == 0 && r_end_off > 0)
                {
                    // Unmap the start of the mapping
                    mapping.addr = mapping.addr + mapping.size - r_end_off;
                    mapping.size = r_end_off;
                    mapping.offset = mapping.offset + mapping.size - r_end_off;
                }
                else
                {
                    // Unmap the middle of the mapping
                    MmapEntry new_mapping;

                    // new_mapping is the higher segment (addresses bigger)

                    new_mapping.addr = mapping.addr + mapping.size - r_end_off;
                    new_mapping.size = r_end_off;
                    new_mapping.offset = mapping.offset + mapping.size - r_end_off;
                    new_mapping.fd = vfs.dup(mapping.fd);
                    new_mapping.shared = mapping.shared;
                    new_mapping.perms = mapping.perms;

                    mapping.size = start_off;

                    it = mappings.insert(it, new_mapping);
                    it += 1; // `it` pointed to the newly inserted entry, which was inserted
                    // before the current one, so we need to increment it to point to the current one again
                }
            }
        }

        return 0;
    }
} // namespace Hamster

