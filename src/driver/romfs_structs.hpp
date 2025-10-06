// romfs data structures

#pragma once

#include <cstdint>

namespace Hamster
{
    // Note: romfs multi-byte integers are always big-endian

    uint32_t round_up_16(uint32_t val)
    { return (val + 15) & ~15; }

    uint32_t round_down_16(uint32_t val)
    { return val & ~15; }

    struct romfs_struct_fs_header
    {
        uint8_t magic[8];
        uint32_t full_size;
        uint32_t checksum;

        void init()
        {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            full_size = __builtin_bswap32(full_size);
            checksum = __builtin_bswap32(checksum);
#endif
        }
    };

    inline constexpr uint8_t romfs_magic[8] = { '-', 'r', 'o', 'm', '1', 'f', 's', '-' }; // "-rom1fs-"

    struct romfs_struct_file
    {
        union
        {
            uint32_t _next_file;

// Workaround for anonymous structs
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
            struct
            {
                uint32_t file_type : 3;
                uint32_t is_executable : 1;
            };
#pragma GCC diagnostic pop
        };

        uint32_t info;
        uint32_t size;
        uint32_t checksum;
        // File data comes after file name, aligned to 16 bits

        void init()
        {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            _next_file = __builtin_bswap32(_next_file);
            info = __builtin_bswap32(info);
            size = __builtin_bswap32(size);
            checksum = __builtin_bswap32(checksum);
#endif
        }

        uint32_t next_file() const
        {
            return _next_file & ~0xF;
        }
    };

    enum RomFsFileType : uint32_t
    {
        romfs_type_hardlink = 0,
        romfs_type_directory = 1,
        romfs_type_regular = 2,
        romfs_type_symlink = 3,
        romfs_type_block = 4,
        romfs_type_char = 5,
        romfs_type_sock = 6,
        romfs_type_fifo = 7,
    };
} // namespace Hamster

