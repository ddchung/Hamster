// Hamster block device cache

#pragma once

#include <memory/allocator.hpp>
#include <memory/stl_map.hpp>
#include <memory/stl_sequential.hpp>
#include <kscheduler/kscheduler.hpp>
#include <platform/platform.hpp>
#include <errno/errno.h>
#include <cstdint>
#include <utility>
#include <cassert>

namespace Hamster
{
    /*
    The block device cache requires a backend that provides at least this functionality,
    with similar signatures, that can be called as-if it were as written here:

    struct
    {
        // Get the log[2]( block size )
        // shouldn't fail, but if it does, return -1
        int64_t get_block_size_log2();

        // Read from a specific block
        // Returns 0 on success, -1 on failure
        int read_block(uint64_t block_number, void* buffer);

        // Write to a specific block
        // Returns 0 on success, -1 on failure
        // NOTE: If you want to have something that is read only, fail with H_EROFS and return -1
        int write_block(uint64_t block_number, const void* buffer);

        // Check if the device is read-only
        bool is_read_only();
    };

    In turn, the cache provides this functionality:

    struct
    {
        // Get the block size
        int64_t get_block_size();

        // Note: The read/write functions here operate on the byte level, not the block level
        //       and uses our caching to optimize performance

        ssize_t read(uint64_t location, void* buffer, size_t size);
        ssize_t write(uint64_t location, const void* buffer, size_t size);
    };
     */


    template <class Backend>
    class BlockDeviceCache : public Backend
    {
        struct CacheEntry
        {
            uint8_t *data;
            uint32_t eviction_queue_count : 31;
            uint32_t dirty : 1;
        };

    public:
        // constructor forwards all arguments to the Backend constructor
        template <typename... Args>
        BlockDeviceCache(Args&&... args);
        ~BlockDeviceCache();
        BlockDeviceCache(const BlockDeviceCache &other) = delete;
        BlockDeviceCache& operator=(const BlockDeviceCache &other) = delete;

        int64_t get_block_size();
        ssize_t read(uint64_t location, void* buffer, size_t size);
        ssize_t write(uint64_t location, const void* buffer, size_t size);

    private:
        UnorderedMap<uint64_t, CacheEntry> cache_map; // block no. -> CacheEntry
        Deque<uint64_t> eviction_queue; // LRU eviction queue

        void access_block(uint64_t block_number);

        // Unload a block and write it to disk if dirty, no-op if not cached
        void unload_block(uint64_t block_number);

        // Unloads blocks if memory pressure is too high
        void unload_if_needed();
    };

    template <class Backend>
    template <typename... Args>
    BlockDeviceCache<Backend>::BlockDeviceCache(Args&&... args)
        : Backend(std::forward<Args>(args)...)
    {
    }

    template <class Backend>
    BlockDeviceCache<Backend>::~BlockDeviceCache()
    {
    }

    template <class Backend>
    int64_t BlockDeviceCache<Backend>::get_block_size()
    {
        return 1 << Backend::get_block_size_log2();
    }

    template <class Backend>
    ssize_t BlockDeviceCache<Backend>::read(uint64_t location, void* buffer, size_t size)
    {
        assert(buffer);

        uint64_t log2_blksz = Backend::get_block_size_log2();

        uint64_t loc_start = location >> log2_blksz;
        uint64_t loc_end = (location + size - 1) >> log2_blksz;

        size_t original_size = size;

        // Read each block into the buffer
        for (uint64_t block = loc_start; block <= loc_end; block++)
        {
            access_block(block);

            auto it = cache_map.find(block);
            if (it == cache_map.end())
                break; // EOF

            uint64_t segment_off = location & (get_block_size() - 1);
            uint64_t segment_size = std::min<uint64_t>(size, get_block_size() - segment_off);
            uint8_t *block_data = it->second.data;
            memcpy(buffer, block_data + segment_off, segment_size);

            // Increment
            location += segment_size;
            size -= segment_size;
            buffer = (uint8_t *)buffer + segment_size;
        }

        // If no bytes were read, return error
        if (original_size == size)
            return -1;

        // Return number of bytes read
        return original_size - size;
    }

    template <class Backend>
    ssize_t BlockDeviceCache<Backend>::write(uint64_t location, const void* buffer, size_t size)
    {
        assert(buffer);

        if (Backend::is_read_only())
        {
            error = H_EROFS;
            return -1;
        }

        uint64_t log2_blksz = Backend::get_block_size_log2();

        uint64_t loc_start = location >> log2_blksz;
        uint64_t loc_end = (location + size - 1) >> log2_blksz;

        size_t original_size = size;

        // Write each block from the buffer
        for (uint64_t block = loc_start; block <= loc_end; block++)
        {
            access_block(block);

            auto it = cache_map.find(block);
            if (it == cache_map.end())
                break;
            
            // Mark block as dirty
            it->second.dirty = 1;

            uint64_t segment_off = location & (get_block_size() - 1);
            uint64_t segment_size = std::min(size, get_block_size() - segment_off);
            uint8_t *block_data = it->second.data();
            memcpy(block_data + (original_size - size), buffer, segment_size);

            // Increment
            location += segment_size;
            size -= segment_size;
            buffer = (const uint8_t *)buffer + segment_size;
        }

        // If no bytes were written, return error
        if (original_size == size)
            return -1;

        // Return number of bytes written
        return original_size - size;
    }

    template <class Backend>
    void BlockDeviceCache<Backend>::access_block(uint64_t block_number)
    {
        if (cache_map.find(block_number) != cache_map.end())
        {
            // Block is already loaded, put into eviction queue
            if (eviction_queue.back() != block_number)
            {
                eviction_queue.push_back(block_number);
                cache_map[block_number].eviction_queue_count++;
            }
            return;
        }
        
        // Load the block

        CacheEntry &entry = cache_map[block_number];

        unload_if_needed();

        entry.data = (uint8_t *)_malloc(get_block_size());
        assert(entry.data);

        if (Backend::read_block(block_number, entry.data) < 0)
        {
            // Failed to read, probably past the end
            _free(entry.data);
            cache_map.erase(block_number);
        }

        entry.eviction_queue_count = 1;
        entry.dirty = 0;
        eviction_queue.push_back(block_number);
    }

    template <class Backend>
    void BlockDeviceCache<Backend>::unload_block(uint64_t block_number)
    {
        auto it = cache_map.find(block_number);
        if (it == cache_map.end())
            return;
        
        // Write if dirty
        if (it->second.dirty)
            Backend::write_block(block_number, it->second.data);
        
        // Deallocate
        _free(it->second.data);
        cache_map.erase(it);
    }

    template <class Backend>
    void BlockDeviceCache<Backend>::unload_if_needed()
    {
        while (_get_free_memory() < HAMSTER_DISK_FREE_RAM)
        {
            if (eviction_queue.empty())
                break;

            uint32_t id = eviction_queue.front();
            eviction_queue.pop_front();

            auto it = cache_map.find(id);
            if (it == cache_map.end())
                continue;

            if (--it->second.eviction_queue_count == 0)
                unload_block(id);
        }
    }
} // namespace Hamster

