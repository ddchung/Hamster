// ABI values

#pragma once

namespace Hamster
{
    constexpr int STAT_IFDIR = 0040000;  // Directory
    constexpr int STAT_IFCHR = 0020000;  // Character device
    constexpr int STAT_IFBLK = 0060000;  // Block device
    constexpr int STAT_IFREG = 0100000;  // Regular file
    constexpr int STAT_IFIFO = 0010000;  // FIFO
    constexpr int STAT_IFLNK = 0120000;  // Symbolic link
    constexpr int STAT_IFSOCK = 0140000; // Socket

    inline bool is_directory(int mode) { return (mode & 0170000) == STAT_IFDIR; }
    inline bool is_character_device(int mode) { return (mode & 0170000) == STAT_IFCHR; }
    inline bool is_block_device(int mode) { return (mode & 0170000) == STAT_IFBLK; }
    inline bool is_regular_file(int mode) { return (mode & 0170000) == STAT_IFREG; }
    inline bool is_fifo(int mode) { return (mode & 0170000) == STAT_IFIFO; }
    inline bool is_symbolic_link(int mode) { return (mode & 0170000) == STAT_IFLNK; }
    inline bool is_socket(int mode) { return (mode & 0170000) == STAT_IFSOCK; }
} // namespace Hamster

