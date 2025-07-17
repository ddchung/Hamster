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
    constexpr int STAT_IFMT = 0170000;  // File type mask

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

    inline constexpr int H_SEEK_SET = 0; // Set file offset relative to start of file
    inline constexpr int H_SEEK_CUR = 1; // Set file offset relative to current position
    inline constexpr int H_SEEK_END = 2; // Set file offset relative to end inline constexpr int

    inline constexpr int H_TCGETS = 0x5401;
    inline constexpr int H_TCSETS = 0x5402;
    inline constexpr int H_TCSETSW = 0x5403;
    inline constexpr int H_TCSETSF = 0x5404;
    inline constexpr int H_TCGETA = 0x5405;
    inline constexpr int H_TCSETA = 0x5406;
    inline constexpr int H_TCSETAW = 0x5407;
    inline constexpr int H_TCSETAF = 0x5408;
    inline constexpr int H_TCSBRK = 0x5409;
    inline constexpr int H_TCXONC = 0x540A;
    inline constexpr int H_TCFLSH = 0x540B;
    inline constexpr int H_TIOCEXCL = 0x540C;
    inline constexpr int H_TIOCNXCL = 0x540D;
    inline constexpr int H_TIOCSCTTY = 0x540E;
    inline constexpr int H_TIOCGPGRP = 0x540F;
    inline constexpr int H_TIOCSPGRP = 0x5410;
    inline constexpr int H_TIOCOUTQ = 0x5411;
    inline constexpr int H_TIOCSTI = 0x5412;
    inline constexpr int H_TIOCGWINSZ = 0x5413;
    inline constexpr int H_TIOCSWINSZ = 0x5414;
    inline constexpr int H_TIOCMGET = 0x5415;
    inline constexpr int H_TIOCMBIS = 0x5416;
    inline constexpr int H_TIOCMBIC = 0x5417;
    inline constexpr int H_TIOCMSET = 0x5418;
    inline constexpr int H_TIOCGSOFTCAR = 0x5419;
    inline constexpr int H_TIOCSSOFTCAR = 0x541A;
    inline constexpr int H_FIONREAD = 0x541B;
    inline constexpr int H_TIOCINQ = H_FIONREAD;
    inline constexpr int H_TIOCLINUX = 0x541C;
    inline constexpr int H_TIOCCONS = 0x541D;
    inline constexpr int H_TIOCGSERIAL = 0x541E;
    inline constexpr int H_TIOCSSERIAL = 0x541F;
    inline constexpr int H_TIOCPKT = 0x5420;
    inline constexpr int H_FIONBIO = 0x5421;
    inline constexpr int H_TIOCNOTTY = 0x5422;
    inline constexpr int H_TIOCSETD = 0x5423;
    inline constexpr int H_TIOCGETD = 0x5424;
    inline constexpr int H_TCSBRKP = 0x5425;
    inline constexpr int H_TIOCSBRK = 0x5427;
    inline constexpr int H_TIOCCBRK = 0x5428;
    inline constexpr int H_TIOCGSID = 0x5429;

    inline bool is_directory(int mode) { return (mode & STAT_IFMT) == STAT_IFDIR; }
    inline bool is_character_device(int mode) { return (mode & STAT_IFMT) == STAT_IFCHR; }
    inline bool is_block_device(int mode) { return (mode & STAT_IFMT) == STAT_IFBLK; }
    inline bool is_regular_file(int mode) { return (mode & STAT_IFMT) == STAT_IFREG; }
    inline bool is_fifo(int mode) { return (mode & STAT_IFMT) == STAT_IFIFO; }
    inline bool is_symbolic_link(int mode) { return (mode & STAT_IFMT) == STAT_IFLNK; }
    inline bool is_socket(int mode) { return (mode & STAT_IFMT) == STAT_IFSOCK; }
} // namespace Hamster

