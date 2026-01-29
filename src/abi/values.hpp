/**
 * This file contains constants from the RISC-V linux headers
 *
 * The values here were transformed from marcos from linux's include/asm and include/asm-generic into
 * `inline constexpr int`s
 *
 * The names were also altered, as to not conflict with any host C library
 */

#pragma once

#include <cstdint>

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

    /* c_cc characters */
    inline constexpr int H_VINTR = 0;
    inline constexpr int H_VQUIT = 1;
    inline constexpr int H_VERASE = 2;
    inline constexpr int H_VKILL = 3;
    inline constexpr int H_VEOF = 4;
    inline constexpr int H_VTIME = 5;
    inline constexpr int H_VMIN = 6;
    inline constexpr int H_VSWTC = 7;
    inline constexpr int H_VSTART = 8;
    inline constexpr int H_VSTOP = 9;
    inline constexpr int H_VSUSP = 10;
    inline constexpr int H_VEOL = 11;
    inline constexpr int H_VREPRINT = 12;
    inline constexpr int H_VDISCARD = 13;
    inline constexpr int H_VWERASE = 14;
    inline constexpr int H_VLNEXT = 15;
    inline constexpr int H_VEOL2 = 16;

    /* c_iflag bits */
    inline constexpr int H_IUCLC = 0x0200;
    inline constexpr int H_IXON = 0x0400;
    inline constexpr int H_IXOFF = 0x1000;
    inline constexpr int H_IMAXBEL = 0x2000;
    inline constexpr int H_IUTF8 = 0x4000;

    /* c_oflag bits */
    inline constexpr int H_OLCUC = 0x00002;
    inline constexpr int H_ONLCR = 0x00004;
    inline constexpr int H_NLDLY = 0x00100;
    inline constexpr int H_NL0 = 0x00000;
    inline constexpr int H_NL1 = 0x00100;
    inline constexpr int H_CRDLY = 0x00600;
    inline constexpr int H_CR0 = 0x00000;
    inline constexpr int H_CR1 = 0x00200;
    inline constexpr int H_CR2 = 0x00400;
    inline constexpr int H_CR3 = 0x00600;
    inline constexpr int H_TABDLY = 0x01800;
    inline constexpr int H_TAB0 = 0x00000;
    inline constexpr int H_TAB1 = 0x00800;
    inline constexpr int H_TAB2 = 0x01000;
    inline constexpr int H_TAB3 = 0x01800;
    inline constexpr int H_XTABS = 0x01800;
    inline constexpr int H_BSDLY = 0x02000;
    inline constexpr int H_BS0 = 0x00000;
    inline constexpr int H_BS1 = 0x02000;
    inline constexpr int H_VTDLY = 0x04000;
    inline constexpr int H_VT0 = 0x00000;
    inline constexpr int H_VT1 = 0x04000;
    inline constexpr int H_FFDLY = 0x08000;
    inline constexpr int H_FF0 = 0x00000;
    inline constexpr int H_FF1 = 0x08000;

    /* c_cflag bit meaning */
    inline constexpr int H_CBAUD = 0x0000100f;
    inline constexpr int H_CSIZE = 0x00000030;
    inline constexpr int H_CS5 = 0x00000000;
    inline constexpr int H_CS6 = 0x00000010;
    inline constexpr int H_CS7 = 0x00000020;
    inline constexpr int H_CS8 = 0x00000030;
    inline constexpr int H_CSTOPB = 0x00000040;
    inline constexpr int H_CREAD = 0x00000080;
    inline constexpr int H_PARENB = 0x00000100;
    inline constexpr int H_PARODD = 0x00000200;
    inline constexpr int H_HUPCL = 0x00000400;
    inline constexpr int H_CLOCAL = 0x00000800;
    inline constexpr int H_CBAUDEX = 0x00001000;
    inline constexpr int H_BOTHER = 0x00001000;
    inline constexpr int H_B57600 = 0x00001001;
    inline constexpr int H_B115200 = 0x00001002;
    inline constexpr int H_B230400 = 0x00001003;
    inline constexpr int H_B460800 = 0x00001004;
    inline constexpr int H_B500000 = 0x00001005;
    inline constexpr int H_B576000 = 0x00001006;
    inline constexpr int H_B921600 = 0x00001007;
    inline constexpr int H_B1000000 = 0x00001008;
    inline constexpr int H_B1152000 = 0x00001009;
    inline constexpr int H_B1500000 = 0x0000100a;
    inline constexpr int H_B2000000 = 0x0000100b;
    inline constexpr int H_B2500000 = 0x0000100c;
    inline constexpr int H_B3000000 = 0x0000100d;
    inline constexpr int H_B3500000 = 0x0000100e;
    inline constexpr int H_B4000000 = 0x0000100f;
    inline constexpr int H_CIBAUD = 0x100f0000; /* input baud rate */

    /* c_lflag bits */
    inline constexpr int H_ISIG = 0x00001;
    inline constexpr int H_ICANON = 0x00002;
    inline constexpr int H_XCASE = 0x00004;
    inline constexpr int H_ECHO = 0x00008;
    inline constexpr int H_ECHOE = 0x00010;
    inline constexpr int H_ECHOK = 0x00020;
    inline constexpr int H_ECHONL = 0x00040;
    inline constexpr int H_NOFLSH = 0x00080;
    inline constexpr int H_TOSTOP = 0x00100;
    inline constexpr int H_ECHOCTL = 0x00200;
    inline constexpr int H_ECHOPRT = 0x00400;
    inline constexpr int H_ECHOKE = 0x00800;
    inline constexpr int H_FLUSHO = 0x01000;
    inline constexpr int H_PENDIN = 0x04000;
    inline constexpr int H_IEXTEN = 0x08000;
    inline constexpr int H_EXTPROC = 0x10000;

    /* tcsetattr uses these */
    inline constexpr int H_TCSANOW = 0;
    inline constexpr int H_TCSADRAIN = 1;
    inline constexpr int H_TCSAFLUSH = 2;

    /* c_iflag bits */
    inline constexpr int H_IGNBRK = 0x001; /* Ignore break condition */
    inline constexpr int H_BRKINT = 0x002; /* Signal interrupt on break */
    inline constexpr int H_IGNPAR = 0x004; /* Ignore characters with parity errors */
    inline constexpr int H_PARMRK = 0x008; /* Mark parity and framing errors */
    inline constexpr int H_INPCK = 0x010;  /* Enable input parity check */
    inline constexpr int H_ISTRIP = 0x020; /* Strip 8th bit off characters */
    inline constexpr int H_INLCR = 0x040;  /* Map NL to CR on input */
    inline constexpr int H_IGNCR = 0x080;  /* Ignore CR */
    inline constexpr int H_ICRNL = 0x100;  /* Map CR to NL on input */
    inline constexpr int H_IXANY = 0x800;  /* Any character will restart after stop */

    /* c_oflag bits */
    inline constexpr int H_OPOST = 0x01; /* Perform output processing */
    inline constexpr int H_OCRNL = 0x08;
    inline constexpr int H_ONOCR = 0x10;
    inline constexpr int H_ONLRET = 0x20;
    inline constexpr int H_OFILL = 0x40;

    inline constexpr int FILE_DUPFD = 0;            /* dup */
    inline constexpr int FILE_DUPFD_CLOEXEC = 1030; /* dup with close-on-exec */
    inline constexpr int FILE_GETFD = 1;            /* get close_on_exec */
    inline constexpr int FILE_SETFD = 2;            /* set/clear close_on_exec */
    inline constexpr int FILE_GETFL = 3;            /* get file->f_flags */
    inline constexpr int FILE_SETFL = 4;            /* set file->f_flags */

    inline constexpr int H_FD_CLOEXEC = 1; // Close-on-exec flag

    inline constexpr int H_AT_FDCWD = -100;      // Special value for current working directory
    inline constexpr int H_AT_REMOVEDIR = 0x200; // unlinkat(2) flag to remove directories instead of files
    inline constexpr int H_AT_EACCESS = 0x200;   // faccessat2(2) flag to use EUID/EGID instead of UID/GID for permission checking

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

    inline constexpr int H_SIG_DFL = 0;
    inline constexpr int H_SIG_IGN = 1;

    inline constexpr int H_SIG_BLOCK = 0;
    inline constexpr int H_SIG_UNBLOCK = 1;
    inline constexpr int H_SIG_SETMASK = 2;

    inline constexpr int H_SA_NOCLDSTOP = 0x00000001; /* Don't send SIGCHLD when children stop.  */
    inline constexpr int H_SA_NOCLDWAIT = 0x00000002; /* Don't create zombie on child death.  */
    inline constexpr int H_SA_SIGINFO = 0x00000004;
    inline constexpr int H_SA_RESTORER = 0x04000000;
    inline constexpr int H_SA_ONSTACK = 0x08000000;   /* Use signal stack by using `sa_restorer'. */
    inline constexpr int H_SA_RESTART = 0x10000000;   /* Restart syscall on signal return.  */
    inline constexpr int H_SA_NODEFER = 0x40000000;   /* Don't automatically block the signal when its handler is being executed.  */
    inline constexpr int H_SA_RESETHAND = 0x80000000; /* Reset to default handler on signal return. */

    inline constexpr int H_PROT_READ = 0x1;             /* page can be read */
    inline constexpr int H_PROT_WRITE = 0x2;            /* page can be written */
    inline constexpr int H_PROT_EXEC = 0x4;             /* page can be executed */
    inline constexpr int H_PROT_SEM = 0x8;              /* page may be used for atomic ops */
    inline constexpr int H_PROT_NONE = 0x0;             /* page can not be accessed */
    inline constexpr int H_PROT_GROWSDOWN = 0x01000000; /* mprotect flag: extend change to start of growsdown vma */
    inline constexpr int H_PROT_GROWSUP = 0x02000000;   /* mprotect flag: extend change to end of growsup vma */

    inline constexpr int H_MAP_TYPE = 0x0f;      /* Mask for type of mapping */
    inline constexpr int H_MAP_FIXED = 0x10;     /* Interpret addr exactly */
    inline constexpr int H_MAP_ANONYMOUS = 0x20; /* don't use a file */

    inline constexpr int H_MAP_POPULATE = 0x008000;        /* populate (prefault) pagetables */
    inline constexpr int H_MAP_NONBLOCK = 0x010000;        /* do not block on IO */
    inline constexpr int H_MAP_STACK = 0x020000;           /* give out an address that is best suited for process/thread stacks */
    inline constexpr int H_MAP_HUGETLB = 0x040000;         /* create a huge page mapping */
    inline constexpr int H_MAP_SYNC = 0x080000;            /* perform synchronous page faults for the mapping */
    inline constexpr int H_MAP_FIXED_NOREPLACE = 0x100000; /* MAP_FIXED which doesn't unmap underlying mapping */
    inline constexpr int H_MAP_UNINITIALIZED = 0x4000000;  /* For anonymous mmap, memory could be
                                                            * uninitialized */
    inline constexpr int H_MAP_SHARED = 0x01;              /* Share changes */
    inline constexpr int H_MAP_PRIVATE = 0x02;             /* Changes are private */
    inline constexpr int H_MAP_SHARED_VALIDATE = 0x03;     /* share + validate extension flags */
    inline constexpr int H_MAP_DROPPABLE = 0x08;           /* Zero memory under memory pressure. */

    inline constexpr int H_CLOCK_REALTIME = 0;
    inline constexpr int H_CLOCK_MONOTONIC = 1;

    inline constexpr int H_TIMER_ABSTIME = 1;

    inline constexpr int H_DT_UNKNOWN = 0;
    inline constexpr int H_DT_FIFO = 1;
    inline constexpr int H_DT_CHR = 2;
    inline constexpr int H_DT_DIR = 4;
    inline constexpr int H_DT_BLK = 6;
    inline constexpr int H_DT_REG = 8;
    inline constexpr int H_DT_LNK = 10;
    inline constexpr int H_DT_SOCK = 12;
    inline constexpr int H_DT_WHT = 14;

    inline constexpr int H_EPERM = 1;         /* Operation not permitted */
    inline constexpr int H_ENOENT = 2;        /* No such file or directory */
    inline constexpr int H_ESRCH = 3;         /* No such process */
    inline constexpr int H_EINTR = 4;         /* Interrupted system call */
    inline constexpr int H_EIO = 5;           /* I/O error */
    inline constexpr int H_ENXIO = 6;         /* No such device or address */
    inline constexpr int H_E2BIG = 7;         /* Argument list too long */
    inline constexpr int H_ENOEXEC = 8;       /* Exec format error */
    inline constexpr int H_EBADF = 9;         /* Bad file number */
    inline constexpr int H_ECHILD = 10;       /* No child processes */
    inline constexpr int H_EAGAIN = 11;       /* Try again */
    inline constexpr int H_ENOMEM = 12;       /* Out of memory */
    inline constexpr int H_EACCES = 13;       /* Permission denied */
    inline constexpr int H_EFAULT = 14;       /* Bad address */
    inline constexpr int H_ENOTBLK = 15;      /* Block device required */
    inline constexpr int H_EBUSY = 16;        /* Device or resource busy */
    inline constexpr int H_EEXIST = 17;       /* File exists */
    inline constexpr int H_EXDEV = 18;        /* Cross-device link */
    inline constexpr int H_ENODEV = 19;       /* No such device */
    inline constexpr int H_ENOTDIR = 20;      /* Not a directory */
    inline constexpr int H_EISDIR = 21;       /* Is a directory */
    inline constexpr int H_EINVAL = 22;       /* Invalid argument */
    inline constexpr int H_ENFILE = 23;       /* File table overflow */
    inline constexpr int H_EMFILE = 24;       /* Too many open files */
    inline constexpr int H_ENOTTY = 25;       /* Not a typewriter */
    inline constexpr int H_ETXTBSY = 26;      /* Text file busy */
    inline constexpr int H_EFBIG = 27;        /* File too large */
    inline constexpr int H_ENOSPC = 28;       /* No space left on device */
    inline constexpr int H_ESPIPE = 29;       /* Illegal seek */
    inline constexpr int H_EROFS = 30;        /* Read-only file system */
    inline constexpr int H_EMLINK = 31;       /* Too many links */
    inline constexpr int H_EPIPE = 32;        /* Broken pipe */
    inline constexpr int H_EDOM = 33;         /* Math argument out of domain of func */
    inline constexpr int H_ERANGE = 34;       /* Math result not representable */
    inline constexpr int H_EDEADLK = 35;      /* Resource deadlock would occur */
    inline constexpr int H_ENAMETOOLONG = 36; /* File name too long */
    inline constexpr int H_ENOLCK = 37;       /* No record locks available */
    inline constexpr int H_ENOSYS = 38;       /* Invalid system call number */
    inline constexpr int H_ENOTEMPTY = 39;    /* Directory not empty */
    inline constexpr int H_ELOOP = 40;        /* Too many symbolic links encountered */
    inline constexpr int H_EWOULDBLOCK = 11;  /* Operation would block */
    inline constexpr int H_ENOMSG = 42;       /* No message of desired type */
    inline constexpr int H_EIDRM = 43;        /* Identifier removed */
    inline constexpr int H_ECHRNG = 44;       /* Channel number out of range */
    inline constexpr int H_EL2NSYNC = 45;     /* Level 2 not synchronized */
    inline constexpr int H_EL3HLT = 46;       /* Level 3 halted */
    inline constexpr int H_EL3RST = 47;       /* Level 3 reset */
    inline constexpr int H_ELNRNG = 48;       /* Link number out of range */
    inline constexpr int H_EUNATCH = 49;      /* Protocol driver not attached */
    inline constexpr int H_ENOCSI = 50;       /* No CSI structure available */
    inline constexpr int H_EL2HLT = 51;       /* Level 2 halted */
    inline constexpr int H_EBADE = 52;        /* Invalid exchange */
    inline constexpr int H_EBADR = 53;        /* Invalid request descriptor */
    inline constexpr int H_EXFULL = 54;       /* Exchange full */
    inline constexpr int H_ENOANO = 55;       /* No anode */
    inline constexpr int H_EBADRQC = 56;      /* Invalid request code */
    inline constexpr int H_EBADSLT = 57;      /* Invalid slot */
    inline constexpr int H_EDEADLOCK = 35;
    inline constexpr int H_EBFONT = 59;           /* Bad font file format */
    inline constexpr int H_ENOSTR = 60;           /* Device not a stream */
    inline constexpr int H_ENODATA = 61;          /* No data available */
    inline constexpr int H_ETIME = 62;            /* Timer expired */
    inline constexpr int H_ENOSR = 63;            /* Out of streams resources */
    inline constexpr int H_ENONET = 64;           /* Machine is not on the network */
    inline constexpr int H_ENOPKG = 65;           /* Package not installed */
    inline constexpr int H_EREMOTE = 66;          /* Object is remote */
    inline constexpr int H_ENOLINK = 67;          /* Link has been severed */
    inline constexpr int H_EADV = 68;             /* Advertise error */
    inline constexpr int H_ESRMNT = 69;           /* Srmount error */
    inline constexpr int H_ECOMM = 70;            /* Communication error on send */
    inline constexpr int H_EPROTO = 71;           /* Protocol error */
    inline constexpr int H_EMULTIHOP = 72;        /* Multihop attempted */
    inline constexpr int H_EDOTDOT = 73;          /* RFS specific error */
    inline constexpr int H_EBADMSG = 74;          /* Not a data message */
    inline constexpr int H_EOVERFLOW = 75;        /* Value too large for defined data type */
    inline constexpr int H_ENOTUNIQ = 76;         /* Name not unique on network */
    inline constexpr int H_EBADFD = 77;           /* File descriptor in bad state */
    inline constexpr int H_EREMCHG = 78;          /* Remote address changed */
    inline constexpr int H_ELIBACC = 79;          /* Can not access a needed shared library */
    inline constexpr int H_ELIBBAD = 80;          /* Accessing a corrupted shared library */
    inline constexpr int H_ELIBSCN = 81;          /* .lib section in a.out corrupted */
    inline constexpr int H_ELIBMAX = 82;          /* Attempting to link in too many shared libraries */
    inline constexpr int H_ELIBEXEC = 83;         /* Cannot exec a shared library directly */
    inline constexpr int H_EILSEQ = 84;           /* Illegal byte sequence */
    inline constexpr int H_ERESTART = 85;         /* Interrupted system call should be restarted */
    inline constexpr int H_ESTRPIPE = 86;         /* Streams pipe error */
    inline constexpr int H_EUSERS = 87;           /* Too many users */
    inline constexpr int H_ENOTSOCK = 88;         /* Socket operation on non-socket */
    inline constexpr int H_EDESTADDRREQ = 89;     /* Destination address required */
    inline constexpr int H_EMSGSIZE = 90;         /* Message too long */
    inline constexpr int H_EPROTOTYPE = 91;       /* Protocol wrong type for socket */
    inline constexpr int H_ENOPROTOOPT = 92;      /* Protocol not available */
    inline constexpr int H_EPROTONOSUPPORT = 93;  /* Protocol not supported */
    inline constexpr int H_ESOCKTNOSUPPORT = 94;  /* Socket type not supported */
    inline constexpr int H_EOPNOTSUPP = 95;       /* Operation not supported on transport endpoint */
    inline constexpr int H_ENOTSUP = 95;          /* Operation not supported */
    inline constexpr int H_EPFNOSUPPORT = 96;     /* Protocol family not supported */
    inline constexpr int H_EAFNOSUPPORT = 97;     /* Address family not supported by protocol */
    inline constexpr int H_EADDRINUSE = 98;       /* Address already in use */
    inline constexpr int H_EADDRNOTAVAIL = 99;    /* Cannot assign requested address */
    inline constexpr int H_ENETDOWN = 100;        /* Network is down */
    inline constexpr int H_ENETUNREACH = 101;     /* Network is unreachable */
    inline constexpr int H_ENETRESET = 102;       /* Network dropped connection because of reset */
    inline constexpr int H_ECONNABORTED = 103;    /* Software caused connection abort */
    inline constexpr int H_ECONNRESET = 104;      /* Connection reset by peer */
    inline constexpr int H_ENOBUFS = 105;         /* No buffer space available */
    inline constexpr int H_EISCONN = 106;         /* Transport endpoint is already connected */
    inline constexpr int H_ENOTCONN = 107;        /* Transport endpoint is not connected */
    inline constexpr int H_ESHUTDOWN = 108;       /* Cannot send after transport endpoint shutdown */
    inline constexpr int H_ETOOMANYREFS = 109;    /* Too many references: cannot splice */
    inline constexpr int H_ETIMEDOUT = 110;       /* Connection timed out */
    inline constexpr int H_ECONNREFUSED = 111;    /* Connection refused */
    inline constexpr int H_EHOSTDOWN = 112;       /* Host is down */
    inline constexpr int H_EHOSTUNREACH = 113;    /* No route to host */
    inline constexpr int H_EALREADY = 114;        /* Operation already in progress */
    inline constexpr int H_EINPROGRESS = 115;     /* Operation now in progress */
    inline constexpr int H_ESTALE = 116;          /* Stale file handle */
    inline constexpr int H_EUCLEAN = 117;         /* Structure needs cleaning */
    inline constexpr int H_ENOTNAM = 118;         /* Not a XENIX named type file */
    inline constexpr int H_ENAVAIL = 119;         /* No XENIX semaphores available */
    inline constexpr int H_EISNAM = 120;          /* Is a named type file */
    inline constexpr int H_EREMOTEIO = 121;       /* Remote I/O error */
    inline constexpr int H_EDQUOT = 122;          /* Quota exceeded */
    inline constexpr int H_ENOMEDIUM = 123;       /* No medium found */
    inline constexpr int H_EMEDIUMTYPE = 124;     /* Wrong medium type */
    inline constexpr int H_ECANCELED = 125;       /* Operation Canceled */
    inline constexpr int H_ENOKEY = 126;          /* Required key not available */
    inline constexpr int H_EKEYEXPIRED = 127;     /* Key has expired */
    inline constexpr int H_EKEYREVOKED = 128;     /* Key has been revoked */
    inline constexpr int H_EKEYREJECTED = 129;    /* Key was rejected by service */
    inline constexpr int H_EOWNERDEAD = 130;      /* Owner died */
    inline constexpr int H_ENOTRECOVERABLE = 131; /* State not recoverable */
    inline constexpr int H_ERFKILL = 132;         /* Operation not possible due to RF-kill */
    inline constexpr int H_EHWPOISON = 133;       /* Memory page has hardware error */

    inline constexpr int H_FUTEX_WAIT = 0;
    inline constexpr int H_FUTEX_WAKE = 1;
    inline constexpr int H_FUTEX_FD = 2;
    inline constexpr int H_FUTEX_REQUEUE = 3;
    inline constexpr int H_FUTEX_CMP_REQUEUE = 4;
    inline constexpr int H_FUTEX_WAKE_OP = 5;
    inline constexpr int H_FUTEX_LOCK_PI = 6;
    inline constexpr int H_FUTEX_UNLOCK_PI = 7;
    inline constexpr int H_FUTEX_TRYLOCK_PI = 8;
    inline constexpr int H_FUTEX_WAIT_BITSET = 9;
    inline constexpr int H_FUTEX_WAKE_BITSET = 10;
    inline constexpr int H_FUTEX_WAIT_REQUEUE_PI = 11;
    inline constexpr int H_FUTEX_CMP_REQUEUE_PI = 12;
    inline constexpr int H_FUTEX_LOCK_PI2 = 13;
    inline constexpr int H_FUTEX_WAITERS = 0x80000000;
    inline constexpr int H_FUTEX_OWNER_DIED = 0x40000000;
    inline constexpr int H_FUTEX_TID_MASK = 0x3fffffff;
    inline constexpr int H_FUTEX_OP_SET = 0;         /* *(int *)UADDR2 = OPARG; */
    inline constexpr int H_FUTEX_OP_ADD = 1;         /* *(int *)UADDR2 += OPARG; */
    inline constexpr int H_FUTEX_OP_OR = 2;          /* *(int *)UADDR2 |= OPARG; */
    inline constexpr int H_FUTEX_OP_ANDN = 3;        /* *(int *)UADDR2 &= ~OPARG; */
    inline constexpr int H_FUTEX_OP_XOR = 4;         /* *(int *)UADDR2 ^= OPARG; */
    inline constexpr int H_FUTEX_OP_OPARG_SHIFT = 8; /* Use (1 << OPARG) instead of OPARG.  */
    inline constexpr int H_FUTEX_OP_CMP_EQ = 0;      /* if (oldval == CMPARG) wake */
    inline constexpr int H_FUTEX_OP_CMP_NE = 1;      /* if (oldval != CMPARG) wake */
    inline constexpr int H_FUTEX_OP_CMP_LT = 2;      /* if (oldval < CMPARG) wake */
    inline constexpr int H_FUTEX_OP_CMP_LE = 3;      /* if (oldval <= CMPARG) wake */
    inline constexpr int H_FUTEX_OP_CMP_GT = 4;      /* if (oldval > CMPARG) wake */
    inline constexpr int H_FUTEX_OP_CMP_GE = 5;      /* if (oldval >= CMPARG) wake */
    inline constexpr int H_FUTEX_PRIVATE_FLAG = 128;
    inline constexpr int H_FUTEX_CLOCK_REALTIME = 256;
    inline constexpr int H_FUTEX_CMD_MASK = ~(H_FUTEX_PRIVATE_FLAG | H_FUTEX_CLOCK_REALTIME);

    inline constexpr int H_POLLIN = 0x0001;
    inline constexpr int H_POLLPRI = 0x0002;
    inline constexpr int H_POLLOUT = 0x0004;
    inline constexpr int H_POLLERR = 0x0008;
    inline constexpr int H_POLLHUP = 0x0010;
    inline constexpr int H_POLLNVAL = 0x0020;

    inline constexpr int H_SYS_RISCV_FLUSH_ICACHE_LOCAL = 1;

    inline constexpr int H_SIOCPROTOPRIVATE = 0x89E0;

    inline constexpr int H_SHUT_RD = 0;
    inline constexpr int H_SHUT_WR = 1;
    inline constexpr int H_SHUT_RDWR = 2;
    inline constexpr int H_SOCK_STREAM = 1;
    inline constexpr int H_SOCK_DGRAM = 2;
    inline constexpr int H_SOCK_RAW = 3;
    inline constexpr int H_SOCK_RDM = 4;
    inline constexpr int H_SOCK_SEQPACKET = 5;
    inline constexpr int H_SOCK_DCCP = 6;
    inline constexpr int H_SOCK_PACKET = 10;
    inline constexpr int H_SOCK_CLOEXEC = 02000000;
    inline constexpr int H_SOCK_NONBLOCK = 04000;
    inline constexpr int H_PF_UNSPEC = 0;
    inline constexpr int H_PF_LOCAL = 1;
    inline constexpr int H_PF_UNIX = H_PF_LOCAL;
    inline constexpr int H_PF_FILE = H_PF_LOCAL;
    inline constexpr int H_PF_INET = 2;
    inline constexpr int H_PF_AX25 = 3;
    inline constexpr int H_PF_IPX = 4;
    inline constexpr int H_PF_APPLETALK = 5;
    inline constexpr int H_PF_NETROM = 6;
    inline constexpr int H_PF_BRIDGE = 7;
    inline constexpr int H_PF_ATMPVC = 8;
    inline constexpr int H_PF_X25 = 9;
    inline constexpr int H_PF_INET6 = 10;
    inline constexpr int H_PF_ROSE = 11;
    inline constexpr int H_PF_DECnet = 12;
    inline constexpr int H_PF_NETBEUI = 13;
    inline constexpr int H_PF_SECURITY = 14;
    inline constexpr int H_PF_KEY = 15;
    inline constexpr int H_PF_NETLINK = 16;
    inline constexpr int H_PF_ROUTE = H_PF_NETLINK;
    inline constexpr int H_PF_PACKET = 17;
    inline constexpr int H_PF_ASH = 18;
    inline constexpr int H_PF_ECONET = 19;
    inline constexpr int H_PF_ATMSVC = 20;
    inline constexpr int H_PF_RDS = 21;
    inline constexpr int H_PF_SNA = 22;
    inline constexpr int H_PF_IRDA = 23;
    inline constexpr int H_PF_PPPOX = 24;
    inline constexpr int H_PF_WANPIPE = 25;
    inline constexpr int H_PF_LLC = 26;
    inline constexpr int H_PF_IB = 27;
    inline constexpr int H_PF_MPLS = 28;
    inline constexpr int H_PF_CAN = 29;
    inline constexpr int H_PF_TIPC = 30;
    inline constexpr int H_PF_BLUETOOTH = 31;
    inline constexpr int H_PF_IUCV = 32;
    inline constexpr int H_PF_RXRPC = 33;
    inline constexpr int H_PF_ISDN = 34;
    inline constexpr int H_PF_PHONET = 35;
    inline constexpr int H_PF_IEEE802154 = 36;
    inline constexpr int H_PF_CAIF = 37;
    inline constexpr int H_PF_ALG = 38;
    inline constexpr int H_PF_NFC = 39;
    inline constexpr int H_PF_VSOCK = 40;
    inline constexpr int H_PF_KCM = 41;
    inline constexpr int H_PF_QIPCRTR = 42;
    inline constexpr int H_PF_SMC = 43;
    inline constexpr int H_PF_XDP = 44;
    inline constexpr int H_PF_MAX = 45;
    inline constexpr int H_AF_UNSPEC = H_PF_UNSPEC;
    inline constexpr int H_AF_LOCAL = H_PF_LOCAL;
    inline constexpr int H_AF_UNIX = H_AF_LOCAL;
    inline constexpr int H_AF_FILE = H_AF_LOCAL;
    inline constexpr int H_AF_INET = H_PF_INET;
    inline constexpr int H_AF_AX25 = H_PF_AX25;
    inline constexpr int H_AF_IPX = H_PF_IPX;
    inline constexpr int H_AF_APPLETALK = H_PF_APPLETALK;
    inline constexpr int H_AF_NETROM = H_PF_NETROM;
    inline constexpr int H_AF_BRIDGE = H_PF_BRIDGE;
    inline constexpr int H_AF_ATMPVC = H_PF_ATMPVC;
    inline constexpr int H_AF_X25 = H_PF_X25;
    inline constexpr int H_AF_INET6 = H_PF_INET6;
    inline constexpr int H_AF_ROSE = H_PF_ROSE;
    inline constexpr int H_AF_DECnet = H_PF_DECnet;
    inline constexpr int H_AF_NETBEUI = H_PF_NETBEUI;
    inline constexpr int H_AF_SECURITY = H_PF_SECURITY;
    inline constexpr int H_AF_KEY = H_PF_KEY;
    inline constexpr int H_AF_NETLINK = H_PF_NETLINK;
    inline constexpr int H_AF_ROUTE = H_PF_ROUTE;
    inline constexpr int H_AF_PACKET = H_PF_PACKET;
    inline constexpr int H_AF_ASH = H_PF_ASH;
    inline constexpr int H_AF_ECONET = H_PF_ECONET;
    inline constexpr int H_AF_ATMSVC = H_PF_ATMSVC;
    inline constexpr int H_AF_RDS = H_PF_RDS;
    inline constexpr int H_AF_SNA = H_PF_SNA;
    inline constexpr int H_AF_IRDA = H_PF_IRDA;
    inline constexpr int H_AF_PPPOX = H_PF_PPPOX;
    inline constexpr int H_AF_WANPIPE = H_PF_WANPIPE;
    inline constexpr int H_AF_LLC = H_PF_LLC;
    inline constexpr int H_AF_IB = H_PF_IB;
    inline constexpr int H_AF_MPLS = H_PF_MPLS;
    inline constexpr int H_AF_CAN = H_PF_CAN;
    inline constexpr int H_AF_TIPC = H_PF_TIPC;
    inline constexpr int H_AF_BLUETOOTH = H_PF_BLUETOOTH;
    inline constexpr int H_AF_IUCV = H_PF_IUCV;
    inline constexpr int H_AF_RXRPC = H_PF_RXRPC;
    inline constexpr int H_AF_ISDN = H_PF_ISDN;
    inline constexpr int H_AF_PHONET = H_PF_PHONET;
    inline constexpr int H_AF_IEEE802154 = H_PF_IEEE802154;
    inline constexpr int H_AF_CAIF = H_PF_CAIF;
    inline constexpr int H_AF_ALG = H_PF_ALG;
    inline constexpr int H_AF_NFC = H_PF_NFC;
    inline constexpr int H_AF_VSOCK = H_PF_VSOCK;
    inline constexpr int H_AF_KCM = H_PF_KCM;
    inline constexpr int H_AF_QIPCRTR = H_PF_QIPCRTR;
    inline constexpr int H_AF_SMC = H_PF_SMC;
    inline constexpr int H_AF_XDP = H_PF_XDP;
    inline constexpr int H_AF_MAX = H_PF_MAX;
    inline constexpr int H_SO_DEBUG = 1;
    inline constexpr int H_SO_REUSEADDR = 2;
    inline constexpr int H_SO_TYPE = 3;
    inline constexpr int H_SO_ERROR = 4;
    inline constexpr int H_SO_DONTROUTE = 5;
    inline constexpr int H_SO_BROADCAST = 6;
    inline constexpr int H_SO_SNDBUF = 7;
    inline constexpr int H_SO_RCVBUF = 8;
    inline constexpr int H_SO_KEEPALIVE = 9;
    inline constexpr int H_SO_OOBINLINE = 10;
    inline constexpr int H_SO_NO_CHECK = 11;
    inline constexpr int H_SO_PRIORITY = 12;
    inline constexpr int H_SO_LINGER = 13;
    inline constexpr int H_SO_BSDCOMPAT = 14;
    inline constexpr int H_SO_REUSEPORT = 15;
    inline constexpr int H_SO_PASSCRED = 16;
    inline constexpr int H_SO_PEERCRED = 17;
    inline constexpr int H_SO_RCVLOWAT = 18;
    inline constexpr int H_SO_SNDLOWAT = 19;
    inline constexpr int H_SO_ACCEPTCONN = 30;
    inline constexpr int H_SO_PEERSEC = 31;
    inline constexpr int H_SO_SNDBUFFORCE = 32;
    inline constexpr int H_SO_RCVBUFFORCE = 33;
    inline constexpr int H_SO_PROTOCOL = 38;
    inline constexpr int H_SO_DOMAIN = 39;
    inline constexpr int H_SO_RCVTIMEO = 66;
    inline constexpr int H_SO_SNDTIMEO = 67;
    inline constexpr int H_SO_TIMESTAMP = 63;
    inline constexpr int H_SO_TIMESTAMPNS = 64;
    inline constexpr int H_SO_TIMESTAMPING = 65;
    inline constexpr int H_SO_SECURITY_AUTHENTICATION = 22;
    inline constexpr int H_SO_SECURITY_ENCRYPTION_TRANSPORT = 23;
    inline constexpr int H_SO_SECURITY_ENCRYPTION_NETWORK = 24;
    inline constexpr int H_SO_BINDTODEVICE = 25;
    inline constexpr int H_SO_ATTACH_FILTER = 26;
    inline constexpr int H_SO_DETACH_FILTER = 27;
    inline constexpr int H_SO_GET_FILTER = H_SO_ATTACH_FILTER;
    inline constexpr int H_SO_PEERNAME = 28;
    inline constexpr int H_SCM_TIMESTAMP = H_SO_TIMESTAMP;
    inline constexpr int H_SO_PASSSEC = 34;
    inline constexpr int H_SCM_TIMESTAMPNS = H_SO_TIMESTAMPNS;
    inline constexpr int H_SO_MARK = 36;
    inline constexpr int H_SCM_TIMESTAMPING = H_SO_TIMESTAMPING;
    inline constexpr int H_SO_RXQ_OVFL = 40;
    inline constexpr int H_SO_WIFI_STATUS = 41;
    inline constexpr int H_SCM_WIFI_STATUS = H_SO_WIFI_STATUS;
    inline constexpr int H_SO_PEEK_OFF = 42;
    inline constexpr int H_SO_NOFCS = 43;
    inline constexpr int H_SO_LOCK_FILTER = 44;
    inline constexpr int H_SO_SELECT_ERR_QUEUE = 45;
    inline constexpr int H_SO_BUSY_POLL = 46;
    inline constexpr int H_SO_MAX_PACING_RATE = 47;
    inline constexpr int H_SO_BPF_EXTENSIONS = 48;
    inline constexpr int H_SO_INCOMING_CPU = 49;
    inline constexpr int H_SO_ATTACH_BPF = 50;
    inline constexpr int H_SO_DETACH_BPF = H_SO_DETACH_FILTER;
    inline constexpr int H_SO_ATTACH_REUSEPORT_CBPF = 51;
    inline constexpr int H_SO_ATTACH_REUSEPORT_EBPF = 52;
    inline constexpr int H_SO_CNX_ADVICE = 53;
    inline constexpr int H_SCM_TIMESTAMPING_OPT_STATS = 54;
    inline constexpr int H_SO_MEMINFO = 55;
    inline constexpr int H_SO_INCOMING_NAPI_ID = 56;
    inline constexpr int H_SO_COOKIE = 57;
    inline constexpr int H_SCM_TIMESTAMPING_PKTINFO = 58;
    inline constexpr int H_SO_PEERGROUPS = 59;
    inline constexpr int H_SO_ZEROCOPY = 60;
    inline constexpr int H_SO_TXTIME = 61;
    inline constexpr int H_SCM_TXTIME = H_SO_TXTIME;
    inline constexpr int H_SO_BINDTOIFINDEX = 62;
    inline constexpr int H_SO_DETACH_REUSEPORT_BPF = 68;
    inline constexpr int H_SO_PREFER_BUSY_POLL = 69;
    inline constexpr int H_SO_BUSY_POLL_BUDGET = 70;
    inline constexpr int H_SOL_SOCKET = 1;
    inline constexpr int H_SOL_IP = 0;
    inline constexpr int H_SOL_IPV6 = 41;
    inline constexpr int H_SOL_ICMPV6 = 58;
    inline constexpr int H_SOL_RAW = 255;
    inline constexpr int H_SOL_DECNET = 261;
    inline constexpr int H_SOL_X25 = 262;
    inline constexpr int H_SOL_PACKET = 263;
    inline constexpr int H_SOL_ATM = 264;
    inline constexpr int H_SOL_AAL = 265;
    inline constexpr int H_SOL_IRDA = 266;
    inline constexpr int H_SOL_NETBEUI = 267;
    inline constexpr int H_SOL_LLC = 268;
    inline constexpr int H_SOL_DCCP = 269;
    inline constexpr int H_SOL_NETLINK = 270;
    inline constexpr int H_SOL_TIPC = 271;
    inline constexpr int H_SOL_RXRPC = 272;
    inline constexpr int H_SOL_PPPOL2TP = 273;
    inline constexpr int H_SOL_BLUETOOTH = 274;
    inline constexpr int H_SOL_PNPIPE = 275;
    inline constexpr int H_SOL_RDS = 276;
    inline constexpr int H_SOL_IUCV = 277;
    inline constexpr int H_SOL_CAIF = 278;
    inline constexpr int H_SOL_ALG = 279;
    inline constexpr int H_SOL_NFC = 280;
    inline constexpr int H_SOL_KCM = 281;
    inline constexpr int H_SOL_TLS = 282;
    inline constexpr int H_SOL_XDP = 283;
    inline constexpr int H_SOMAXCONN = 128;
    inline constexpr int H_MSG_OOB = 0x0001;
    inline constexpr int H_MSG_PEEK = 0x0002;
    inline constexpr int H_MSG_DONTROUTE = 0x0004;
    inline constexpr int H_MSG_CTRUNC = 0x0008;
    inline constexpr int H_MSG_PROXY = 0x0010;
    inline constexpr int H_MSG_TRUNC = 0x0020;
    inline constexpr int H_MSG_DONTWAIT = 0x0040;
    inline constexpr int H_MSG_EOR = 0x0080;
    inline constexpr int H_MSG_WAITALL = 0x0100;
    inline constexpr int H_MSG_FIN = 0x0200;
    inline constexpr int H_MSG_SYN = 0x0400;
    inline constexpr int H_MSG_CONFIRM = 0x0800;
    inline constexpr int H_MSG_RST = 0x1000;
    inline constexpr int H_MSG_ERRQUEUE = 0x2000;
    inline constexpr int H_MSG_NOSIGNAL = 0x4000;
    inline constexpr int H_MSG_MORE = 0x8000;
    inline constexpr int H_MSG_WAITFORONE = 0x10000;
    inline constexpr int H_MSG_BATCH = 0x40000;
    inline constexpr int H_MSG_ZEROCOPY = 0x4000000;
    inline constexpr int H_MSG_FASTOPEN = 0x20000000;
    inline constexpr int H_MSG_CMSG_CLOEXEC = 0x40000000;

    // clang-format off
    inline constexpr uint8_t H_SIGHAND_TRAMPOLINE[] = {
        0x93, 0x08, 0xb0, 0x08, // li a7,139 # SyscallID::RT_SIGRETURN
        0x73, 0x00, 0x00, 0x00, // ecall
        0x00, 0x00, 0x00, 0x00, // unreachable
    };
    // clang-format on

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

    inline uint32_t make_device_id(uint16_t major, uint32_t minor) { return (major << 20) | (minor & 0xFFFFF); }
} // namespace Hamster
