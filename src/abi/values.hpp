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

    constexpr int OPEN_ACCMODE = 00000003; // Mask for file access modes
    constexpr int OPEN_RDONLY = 00000000;  // Read-only mode
    constexpr int OPEN_WRONLY = 00000001;  // Write-only mode
    constexpr int OPEN_RDWR = 00000002;    // Read-write mode
    constexpr int OPEN_CREAT = 00000100;   // Create file if it does not exist
    constexpr int OPEN_EXCL = 00000200;    // Exclusive use, fail if file exists
    constexpr int OPEN_NOCTTY = 00000400;  // Do not assign controlling terminal
    constexpr int OPEN_TRUNC = 00001000;   // Truncate file to zero length
    constexpr int OPEN_APPEND = 00002000;  // Append mode
    constexpr int OPEN_NONBLOCK = 00004000; // Non-blocking mode
    constexpr int OPEN_SYNC = 04010000; // Synchronous writes
    constexpr int OPEN_FSYNC = OPEN_SYNC; // Alias for OPEN_SYNC
    constexpr int OPEN_ASYNC = 020000; // Enable signal-driven I/O
    constexpr int OPEN_DIRECTORY = 0200000; // Open directory
    constexpr int OPEN_NOFOLLOW = 0400000; // Do not follow symbolic links

    constexpr int H_SEEK_SET = 0;    // Set file offset relative to start of file
    constexpr int H_SEEK_CUR = 1;    // Set file offset relative to current position
    constexpr int H_SEEK_END = 2;    // Set file offset relative to end of file

    inline bool is_directory(int mode) { return (mode & 0170000) == STAT_IFDIR; }
    inline bool is_character_device(int mode) { return (mode & 0170000) == STAT_IFCHR; }
    inline bool is_block_device(int mode) { return (mode & 0170000) == STAT_IFBLK; }
    inline bool is_regular_file(int mode) { return (mode & 0170000) == STAT_IFREG; }
    inline bool is_fifo(int mode) { return (mode & 0170000) == STAT_IFIFO; }
    inline bool is_symbolic_link(int mode) { return (mode & 0170000) == STAT_IFLNK; }
    inline bool is_socket(int mode) { return (mode & 0170000) == STAT_IFSOCK; }
} // namespace Hamster

