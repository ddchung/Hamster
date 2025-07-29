// ABI values

#pragma once

namespace Hamster
{
    inline constexpr int STAT_IFDIR = 0040000;  // Directory
    inline constexpr int STAT_IFCHR = 0020000;  // Character device
    inline constexpr int STAT_IFBLK = 0060000;  // Block device
    inline constexpr int STAT_IFREG = 0100000;  // Regular file
    inline constexpr int STAT_IFIFO = 0010000;  // FIFO
    inline constexpr int STAT_IFLNK = 0120000;  // Symbolic link
    inline constexpr int STAT_IFSOCK = 0140000; // Socket
    inline constexpr int STAT_IFMT = 0170000;   // File type mask

    inline constexpr int OPEN_ACCMODE = 00000003;  // Mask for file access modes
    inline constexpr int OPEN_RDONLY = 00000000;   // Read-only mode
    inline constexpr int OPEN_WRONLY = 00000001;   // Write-only mode
    inline constexpr int OPEN_RDWR = 00000002;     // Read-write mode
    inline constexpr int OPEN_CREAT = 00000100;    // Create file if it does not exist
    inline constexpr int OPEN_EXCL = 00000200;     // Exclusive use, fail if file exists
    inline constexpr int OPEN_NOCTTY = 00000400;   // Do not assign controlling terminal
    inline constexpr int OPEN_TRUNC = 00001000;    // Truncate file to zero length
    inline constexpr int OPEN_APPEND = 00002000;   // Append mode
    inline constexpr int OPEN_NONBLOCK = 00004000; // Non-blocking mode
    inline constexpr int OPEN_SYNC = 04010000;     // Synchronous writes
    inline constexpr int OPEN_FSYNC = OPEN_SYNC;   // Alias for OPEN_SYNC
    inline constexpr int OPEN_ASYNC = 020000;      // Enable signal-driven I/O
    inline constexpr int OPEN_DIRECTORY = 0200000; // Open directory
    inline constexpr int OPEN_NOFOLLOW = 0400000;  // Do not follow symbolic links
    inline constexpr int OPEN_CLOEXEC = 02000000;  // Close-on-exec flag

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

    inline constexpr int FILE_DUPFD = 0;            /* dup */
    inline constexpr int FILE_DUPFD_CLOEXEC = 1030; /* dup with close-on-exec */
    inline constexpr int FILE_GETFD = 1;            /* get close_on_exec */
    inline constexpr int FILE_SETFD = 2;            /* set/clear close_on_exec */
    inline constexpr int FILE_GETFL = 3;            /* get file->f_flags */
    inline constexpr int FILE_SETFL = 4;            /* set file->f_flags */

    inline constexpr int H_FD_CLOEXEC = 1; // Close-on-exec flag

    inline constexpr int H_AT_FDCWD = -100; // Special value for current working directory

    inline constexpr int H_SIGHUP = 1;
    inline constexpr int H_SIGINT = 2;
    inline constexpr int H_SIGQUIT = 3;
    inline constexpr int H_SIGILL = 4;
    inline constexpr int H_SIGTRAP = 5;
    inline constexpr int H_SIGABRT = 6;
    inline constexpr int H_SIGIOT = 6;
    inline constexpr int H_SIGBUS = 7;
    inline constexpr int H_SIGFPE = 8;
    inline constexpr int H_SIGKILL = 9;
    inline constexpr int H_SIGUSR1 = 10;
    inline constexpr int H_SIGSEGV = 11;
    inline constexpr int H_SIGUSR2 = 12;
    inline constexpr int H_SIGPIPE = 13;
    inline constexpr int H_SIGALRM = 14;
    inline constexpr int H_SIGTERM = 15;
    inline constexpr int H_SIGSTKFLT = 16;
    inline constexpr int H_SIGCHLD = 17;
    inline constexpr int H_SIGCONT = 18;
    inline constexpr int H_SIGSTOP = 19;
    inline constexpr int H_SIGTSTP = 20;
    inline constexpr int H_SIGTTIN = 21;
    inline constexpr int H_SIGTTOU = 22;
    inline constexpr int H_SIGURG = 23;
    inline constexpr int H_SIGXCPU = 24;
    inline constexpr int H_SIGXFSZ = 25;
    inline constexpr int H_SIGVTALRM = 26;
    inline constexpr int H_SIGPROF = 27;
    inline constexpr int H_SIGWINCH = 28;
    inline constexpr int H_SIGIO = 29;
    inline constexpr int H_SIGPOLL = H_SIGIO;
    inline constexpr int H_SIGPWR = 30;
    inline constexpr int H_SIGSYS = 31;
    inline constexpr int H_SIGUNUSED = 31;
    inline constexpr int H_SIGRTMIN = 32;
    inline constexpr int H_SIGRTMAX = 64;

    inline constexpr int H_CLONE_SIGNAL = 0x000000ff;         /* signal mask to be sent at exit */
    inline constexpr int H_CLONE_VM = 0x00000100;             /*set if VM shared between processes */
    inline constexpr int H_CLONE_FS = 0x00000200;             /*set if fs info shared between processes */
    inline constexpr int H_CLONE_FILES = 0x00000400;          /*set if open files shared between processes */
    inline constexpr int H_CLONE_SIGHAND = 0x00000800;        /*set if signal handlers and blocked signals shared */
    inline constexpr int H_CLONE_PIDFD = 0x00001000;          /*set if a pidfd should be placed in parent */
    inline constexpr int H_CLONE_PTRACE = 0x00002000;         /*set if we want to let tracing continue on the child too */
    inline constexpr int H_CLONE_VFORK = 0x00004000;          /*set if the parent wants the child to wake it up on mm_release */
    inline constexpr int H_CLONE_PARENT = 0x00008000;         /*set if we want to have the same parent as the cloner */
    inline constexpr int H_CLONE_THREAD = 0x00010000;         /*Same thread group? */
    inline constexpr int H_CLONE_NEWNS = 0x00020000;          /*New mount namespace group */
    inline constexpr int H_CLONE_SYSVSEM = 0x00040000;        /*share system V SEM_UNDO semantics */
    inline constexpr int H_CLONE_SETTLS = 0x00080000;         /*create a new TLS for the child */
    inline constexpr int H_CLONE_PARENT_SETTID = 0x00100000;  /*set the TID in the parent */
    inline constexpr int H_CLONE_CHILD_CLEARTID = 0x00200000; /*clear the TID in the child */
    inline constexpr int H_CLONE_DETACHED = 0x00400000;       /*Unused, ignored */
    inline constexpr int H_CLONE_UNTRACED = 0x00800000;       /*set if the tracing process can't force CLONE_PTRACE on this clone */
    inline constexpr int H_CLONE_CHILD_SETTID = 0x01000000;   /*set the TID in the child */
    inline constexpr int H_CLONE_NEWCGROUP = 0x02000000;      /*New cgroup namespace */
    inline constexpr int H_CLONE_NEWUTS = 0x04000000;         /*New utsname namespace */
    inline constexpr int H_CLONE_NEWIPC = 0x08000000;         /*New ipc namespace */
    inline constexpr int H_CLONE_NEWUSER = 0x10000000;        /*New user namespace */
    inline constexpr int H_CLONE_NEWPID = 0x20000000;         /*New pid namespace */
    inline constexpr int H_CLONE_NEWNET = 0x40000000;         /*New network namespace */
    inline constexpr int H_CLONE_IO = 0x80000000;             /*Clone io context */

    inline constexpr int H_SI_USER = 0;      /* sent by kill, sigsend, raise */
    inline constexpr int H_SI_KERNEL = 0x80; /* sent by the kernel from somewhere */
    inline constexpr int H_SI_QUEUE = -1;    /* sent by sigqueue */
    inline constexpr int H_SI_TIMER = -2;    /* sent by timer expiration */
    inline constexpr int H_SI_MESGQ = -3;    /* sent by real time mesq state change */
    inline constexpr int H_SI_ASYNCIO = -4;  /* sent by AIO completion */
    inline constexpr int H_SI_SIGIO = -5;    /* sent by queued SIGIO */
    inline constexpr int H_SI_TKILL = -6;    /* sent by tkill system call */
    inline constexpr int H_SI_DETHREAD = -7; /* sent by execve() killing subsidiary threads */
    inline constexpr int H_SI_ASYNCNL = -60; /* sent by glibc async name lookup completion */

    inline constexpr int H_ILL_ILLOPC = 1;   /* illegal opcode */
    inline constexpr int H_ILL_ILLOPN = 2;   /* illegal operand */
    inline constexpr int H_ILL_ILLADR = 3;   /* illegal addressing mode */
    inline constexpr int H_ILL_ILLTRP = 4;   /* illegal trap */
    inline constexpr int H_ILL_PRVOPC = 5;   /* privileged opcode */
    inline constexpr int H_ILL_PRVREG = 6;   /* privileged register */
    inline constexpr int H_ILL_COPROC = 7;   /* coprocessor error */
    inline constexpr int H_ILL_BADSTK = 8;   /* internal stack error */
    inline constexpr int H_ILL_BADIADDR = 9; /* unimplemented instruction address */
    inline constexpr int H_ILL_BREAK = 10;   /* illegal break */
    inline constexpr int H_ILL_BNDMOD = 11;  /* bundle-update (modification) in progress */
    inline constexpr int H_NSIGILL = 11;

    inline constexpr int H_FPE_INTDIV = 1;    /* integer divide by zero */
    inline constexpr int H_FPE_INTOVF = 2;    /* integer overflow */
    inline constexpr int H_FPE_FLTDIV = 3;    /* floating point divide by zero */
    inline constexpr int H_FPE_FLTOVF = 4;    /* floating point overflow */
    inline constexpr int H_FPE_FLTUND = 5;    /* floating point underflow */
    inline constexpr int H_FPE_FLTRES = 6;    /* floating point inexact result */
    inline constexpr int H_FPE_FLTINV = 7;    /* floating point invalid operation */
    inline constexpr int H_FPE_FLTSUB = 8;    /* subscript out of range */
    inline constexpr int H_FPE_DECOVF = 9;    /* decimal overflow */
    inline constexpr int H_FPE_DECDIV = 10;   /* decimal division by zero */
    inline constexpr int H_FPE_DECERR = 11;   /* packed decimal error */
    inline constexpr int H_FPE_INVASC = 12;   /* invalid ASCII digit */
    inline constexpr int H_FPE_INVDEC = 13;   /* invalid decimal digit */
    inline constexpr int H_FPE_FLTUNK = 14;   /* undiagnosed floating-point exception */
    inline constexpr int H_FPE_CONDTRAP = 15; /* trap on condition */
    inline constexpr int H_NSIGFPE = 15;

    inline constexpr int H_SEGV_MAPERR = 1;  /* address not mapped to object */
    inline constexpr int H_SEGV_ACCERR = 2;  /* invalid permissions for mapped object */
    inline constexpr int H_SEGV_BNDERR = 3;  /* failed address bound checks */
    inline constexpr int H_SEGV_PKUERR = 4;  /* failed protection key checks */
    inline constexpr int H_SEGV_ACCADI = 5;  /* ADI not enabled for mapped object */
    inline constexpr int H_SEGV_ADIDERR = 6; /* Disrupting MCD error */
    inline constexpr int H_SEGV_ADIPERR = 7; /* Precise MCD exception */
    inline constexpr int H_SEGV_MTEAERR = 8; /* Asynchronous ARM MTE error */
    inline constexpr int H_SEGV_MTESERR = 9; /* Synchronous ARM MTE exception */
    inline constexpr int H_SEGV_CPERR = 10;  /* Control protection fault */
    inline constexpr int H_NSIGSEGV = 10;

    inline constexpr int H_BUS_ADRALN = 1;    /* invalid address alignment */
    inline constexpr int H_BUS_ADRERR = 2;    /* non-existent physical address */
    inline constexpr int H_BUS_OBJERR = 3;    /* object specific hardware error */
    inline constexpr int H_BUS_MCEERR_AR = 4; /* hardware memory error consumed on a machine check: action required */
    inline constexpr int H_BUS_MCEERR_AO = 5; /* hardware memory error detected in process but not consumed: action optional*/
    inline constexpr int H_NSIGBUS = 5;

    inline constexpr int H_TRAP_BRKPT = 1;  /* process breakpoint */
    inline constexpr int H_TRAP_TRACE = 2;  /* process trace trap */
    inline constexpr int H_TRAP_BRANCH = 3; /* process taken branch trap */
    inline constexpr int H_TRAP_HWBKPT = 4; /* hardware breakpoint/watchpoint */
    inline constexpr int H_TRAP_UNK = 5;    /* undiagnosed trap */
    inline constexpr int H_TRAP_PERF = 6;   /* perf event with sigtrap=1 */
    inline constexpr int H_NSIGTRAP = 6;

    inline constexpr int H_CLD_EXITED = 1;    /* child has exited */
    inline constexpr int H_CLD_KILLED = 2;    /* child was killed */
    inline constexpr int H_CLD_DUMPED = 3;    /* child terminated abnormally */
    inline constexpr int H_CLD_TRAPPED = 4;   /* traced child has trapped */
    inline constexpr int H_CLD_STOPPED = 5;   /* child has stopped */
    inline constexpr int H_CLD_CONTINUED = 6; /* stopped child has continued */
    inline constexpr int H_NSIGCHLD = 6;

    inline constexpr int H_WNOHANG = 0x00000001;
    inline constexpr int H_WUNTRACED = 0x00000002;
    inline constexpr int H_WSTOPPED = H_WUNTRACED;
    inline constexpr int H_WEXITED = 0x00000004;
    inline constexpr int H_WCONTINUED = 0x00000008;
    inline constexpr int H_WNOWAIT = 0x01000000;
    
    inline constexpr int H_P_ALL = 0;
    inline constexpr int H_P_PID = 1;
    inline constexpr int H_P_PGID = 2;
    inline constexpr int H_P_PIDFD = 3;

    inline constexpr int H_AT_EMPTY_PATH = 0x1000;
    inline constexpr int H_AT_SYMLINK_NOFOLLOW = 0x100;
    inline constexpr int H_STATX_BASIC_STATS = 0x000007ff;

    inline bool is_directory(int mode) { return (mode & STAT_IFMT) == STAT_IFDIR; }
    inline bool is_character_device(int mode) { return (mode & STAT_IFMT) == STAT_IFCHR; }
    inline bool is_block_device(int mode) { return (mode & STAT_IFMT) == STAT_IFBLK; }
    inline bool is_regular_file(int mode) { return (mode & STAT_IFMT) == STAT_IFREG; }
    inline bool is_fifo(int mode) { return (mode & STAT_IFMT) == STAT_IFIFO; }
    inline bool is_symbolic_link(int mode) { return (mode & STAT_IFMT) == STAT_IFLNK; }
    inline bool is_socket(int mode) { return (mode & STAT_IFMT) == STAT_IFSOCK; }

    inline uint16_t make_wait_exited(uint8_t exit_code) { return exit_code << 8; }
    inline uint16_t make_wait_stopped(uint8_t stop_signal) { return (stop_signal << 8) | 0x7f; }
    inline uint16_t make_wait_continued() { return 0xffff; }
    inline uint16_t make_wait_terminated(uint8_t term_signal) { return term_signal & 0x7f; }
    inline uint16_t make_wait_terminated_coredump(uint8_t term_signal) { return (term_signal & 0x7f) | 0x80; }
    inline bool is_wait_exited(uint16_t status) { return (status & 0x7f) == 0; }
    inline bool is_wait_stopped(uint16_t status) { return (status & 0xff) == 0x7f; }
    inline bool is_wait_continued(uint16_t status) { return status == 0xffff; }
    inline bool is_wait_terminated(uint16_t status) { return (status & 0x7f) != 0 && (status & 0x7f) != 0x7f; }
    inline bool is_wait_terminated_coredump(uint16_t status) { return is_wait_terminated(status) && (status & 0x80) != 0; }
} // namespace Hamster
