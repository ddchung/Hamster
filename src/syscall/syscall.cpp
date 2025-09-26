
#include <syscall/syscall.hpp>
#include <abi/syscall_id.hpp>
#include <process/scheduler.hpp>
#include <platform/platform.hpp>
#include <errno/errno.h>
#include <cstring>

#ifndef NTRACE
#include <cinttypes>
#include <tuple>
#endif

namespace Hamster
{
    namespace
    {
#ifndef NTRACE
        const char *error_names[] = {
            "No Error",
            "H_EPERM - Operation not permitted",
            "H_ENOENT - No such file or directory",
            "H_ESRCH - No such process",
            "H_EINTR - Interrupted system call",
            "H_EIO - I/O error",
            "H_ENXIO - No such device or address",
            "H_E2BIG - Argument list too long",
            "H_ENOEXEC - Exec format error",
            "H_EBADF - Bad file number",
            "H_ECHILD - No child processes",
            "H_EAGAIN - Try again",
            "H_ENOMEM - Out of memory",
            "H_EACCES - Permission denied",
            "H_EFAULT - Bad address",
            "H_ENOTBLK - Block device required",
            "H_EBUSY - Device or resource busy",
            "H_EEXIST - File exists",
            "H_EXDEV - Cross-device link",
            "H_ENODEV - No such device",
            "H_ENOTDIR - Not a directory",
            "H_EISDIR - Is a directory",
            "H_EINVAL - Invalid argument",
            "H_ENFILE - File table overflow",
            "H_EMFILE - Too many open files",
            "H_ENOTTY - Not a typewriter",
            "H_ETXTBSY - Text file busy",
            "H_EFBIG - File too large",
            "H_ENOSPC - No space left on device",
            "H_ESPIPE - Illegal seek",
            "H_EROFS - Read-only file system",
            "H_EMLINK - Too many links",
            "H_EPIPE - Broken pipe",
            "H_EDOM - Math argument out of domain of func",
            "H_ERANGE - Math result not representable",
            "H_EDEADLK - Resource deadlock would occur",
            "H_ENAMETOOLONG - File name too long",
            "H_ENOLCK - No record locks available",
            "H_ENOSYS - Invalid system call number",
            "H_ENOTEMPTY - Directory not empty",
            "H_ELOOP - Too many symbolic links encountered",
            "H_EWOULDBLOCK - Operation would block",
            "H_ENOMSG - No message of desired type",
            "H_EIDRM - Identifier removed",
            "H_ECHRNG - Channel number out of range",
            "H_EL2NSYNC - Level 2 not synchronized",
            "H_EL3HLT - Level 3 halted",
            "H_EL3RST - Level 3 reset",
            "H_ELNRNG - Link number out of range",
            "H_EUNATCH - Protocol driver not attached",
            "H_ENOCSI - No CSI structure available",
            "H_EL2HLT - Level 2 halted",
            "H_EBADE - Invalid exchange",
            "H_EBADR - Invalid request descriptor",
            "H_EXFULL - Exchange full",
            "H_ENOANO - No anode",
            "H_EBADRQC - Invalid request code",
            "H_EBADSLT - Invalid slot",
            "H_EBFONT - Bad font file format",
            "H_ENOSTR - Device not a stream",
            "H_ENODATA - No data available",
            "H_ETIME - Timer expired",
            "H_ENOSR - Out of streams resources",
            "H_ENONET - Machine is not on the network",
            "H_ENOPKG - Package not installed",
            "H_EREMOTE - Object is remote",
            "H_ENOLINK - Link has been severed",
            "H_EADV - Advertise error",
            "H_ESRMNT - Srmount error",
            "H_ECOMM - Communication error on send",
            "H_EPROTO - Protocol error",
            "H_EMULTIHOP - Multihop attempted",
            "H_EDOTDOT - RFS specific error",
            "H_EBADMSG - Not a data message",
            "H_EOVERFLOW - Value too large for defined data type",
            "H_ENOTUNIQ - Name not unique on network",
            "H_EBADFD - File descriptor in bad state",
            "H_EREMCHG - Remote address changed",
            "H_ELIBACC - Can not access a needed shared library",
            "H_ELIBBAD - Accessing a corrupted shared library",
            "H_ELIBSCN - .lib section in a.out corrupted",
            "H_ELIBMAX - Attempting to link in too many shared libraries",
            "H_ELIBEXEC - Cannot exec a shared library directly",
            "H_EILSEQ - Illegal byte sequence",
            "H_ERESTART - Interrupted system call should be restarted",
            "H_ESTRPIPE - Streams pipe error",
            "H_EUSERS - Too many users",
            "H_ENOTSOCK - Socket operation on non-socket",
            "H_EDESTADDRREQ - Destination address required",
            "H_EMSGSIZE - Message too long",
            "H_EPROTOTYPE - Protocol wrong type for socket",
            "H_ENOPROTOOPT - Protocol not available",
            "H_EPROTONOSUPPORT - Protocol not supported",
            "H_ESOCKTNOSUPPORT - Socket type not supported",
            "H_EOPNOTSUPP - Operation not supported on transport endpoint",
            "H_ENOTSUP - Operation not supported",
            "H_EPFNOSUPPORT - Protocol family not supported",
            "H_EAFNOSUPPORT - Address family not supported by protocol",
            "H_EADDRINUSE - Address already in use",
            "H_EADDRNOTAVAIL - Cannot assign requested address",
            "H_ENETDOWN - Network is down",
            "H_ENETUNREACH - Network is unreachable",
            "H_ENETRESET - Network dropped connection because of reset",
            "H_ECONNABORTED - Software caused connection abort",
            "H_ECONNRESET - Connection reset by peer",
            "H_ENOBUFS - No buffer space available",
            "H_EISCONN - Transport endpoint is already connected",
            "H_ENOTCONN - Transport endpoint is not connected",
            "H_ESHUTDOWN - Cannot send after transport endpoint shutdown",
            "H_ETOOMANYREFS - Too many references: cannot splice",
            "H_ETIMEDOUT - Connection timed out",
            "H_ECONNREFUSED - Connection refused",
            "H_EHOSTDOWN - Host is down",
            "H_EHOSTUNREACH - No route to host",
            "H_EALREADY - Operation already in progress",
            "H_EINPROGRESS - Operation now in progress",
            "H_ESTALE - Stale file handle",
            "H_EUCLEAN - Structure needs cleaning",
            "H_ENOTNAM - Not a XENIX named type file",
            "H_ENAVAIL - No XENIX semaphores available",
            "H_EISNAM - Is a named type file",
            "H_EREMOTEIO - Remote I/O error",
            "H_EDQUOT - Quota exceeded",
            "H_ENOMEDIUM - No medium found",
            "H_EMEDIUMTYPE - Wrong medium type",
            "H_ECANCELED - Operation Canceled",
            "H_ENOKEY - Required key not available",
            "H_EKEYEXPIRED - Key has expired",
            "H_EKEYREVOKED - Key has been revoked",
            "H_EKEYREJECTED - Key was rejected by service",
            "H_EOWNERDEAD - Owner died",
            "H_ENOTRECOVERABLE - State not recoverable",
            "H_ERFKILL - Operation not possible due to RF-kill",
            "H_EHWPOISON - Memory page has hardware error"};

        constexpr struct ParamType_INT
        {
        } PT_INT;
        constexpr struct ParamType_UINT
        {
        } PT_UINT;
        constexpr struct ParamType_PTR
        {
        } PT_PTR;
        constexpr struct ParamType_STR
        {
        } PT_STR;

        struct ParamType_FLAGS_Flag
        {
            constexpr ParamType_FLAGS_Flag(const char *n, uint32_t m)
                : name(n), mask(m), value(m)
            {
            }

            constexpr ParamType_FLAGS_Flag(const char *n, uint32_t m, uint32_t v)
                : name(n), mask(m), value(v)
            {
            }

            // Flag active if
            // `(param & mask) == value`
            const char *name;
            uint32_t mask;
            uint32_t value;
        };

        template <ParamType_FLAGS_Flag...>
        struct ParamType_FLAGS
        {
        };

        template <ParamType_FLAGS_Flag... Flags>
        constexpr ParamType_FLAGS<Flags...> PT_FLAGS;

        template <size_t N, class T>
        struct SysTraceParam
        {
            const char name[N];
            T type;
        };

        void print_arg(ParamType_INT, const char *name, uint32_t i)
        {
            _trace("%s=%" PRIi32, name, (int32_t)i);
        }

        void print_arg(ParamType_UINT, const char *name, uint32_t i)
        {
            _trace("%s=%" PRIu32, name, i);
        }

        void print_arg(ParamType_PTR, const char *name, uint32_t p)
        {
            _trace("%s=%08" PRIx32, name, p);

            Task *current_task = scheduler.get_current_task();

            int perms = current_task->get_memory().get_permissions(p);
            if (perms < 0)
            {
                _trace(" (ptr invalid)");
            }
            else
            {
                _trace(" (addr perms %c%c%c)", perms & PERM_READ ? 'r' : '-',
                       perms & PERM_WRITE ? 'w' : '-',
                       perms & PERM_EXEC ? 'x' : '-');
            }
        }

        void print_arg(ParamType_STR, const char *name, uint32_t s)
        {
            Task *current_task = scheduler.get_current_task();

            char *str = current_task->get_memory().get_string(s);
            if (str)
            {
                _trace("%s=\"", name);
                for (const char *it = str; *it; ++it)
                {
                    char c = *it;
                    if (c >= 0x20 && c < 0x7F)
                        _trace("%c", c);
                    else
                        _trace("\\x%02x", c);
                }
                _trace("\" @ 0x%08" PRIx32, s);
                dealloc(str);
            }
            else
            {
                _trace("(inaccessible str @ %08" PRIx32 ")", s);
            }
        }

        template <ParamType_FLAGS_Flag... Flags>
        void print_arg(ParamType_FLAGS<Flags...>, const char *name, uint32_t i)
        {
            _trace("%s=(", name);
            bool first = true;
            for (const auto &flag : {Flags...})
            {
                if (i & flag.mask == flag.value)
                {
                    if (!first)
                        _trace(" | ");
                    first = false;
                    _trace("%s", flag.name);
                }
            }
            _trace(")");
        }

        template <SysTraceParam... Args>
        void trace_syscall(const char *name, int32_t *args, int32_t result)
        {
            Task *current_task = scheduler.get_current_task();
            _trace("TID %" PRIu32 " system call %s, args: {", current_task->tid, name);
            ((
                 print_arg(Args.type, Args.name, (uint32_t)*args),
                 _trace(", "),
                 ++args),
             ...);
            _trace("} -> %" PRIi32, result);

            if (result < 0 && (ssize_t)result > -(ssize_t)(sizeof(error_names) / sizeof(error_names[0])))
                _trace(" (error %s)", error_names[-result]);
            _trace("\n");
        }
#else // NTRACE
#define trace_syscall(...) ((void)0)
#endif // NTRACE
    } // namespace

    int32_t cvt_error()
    {
        int32_t err = error;
        error = 0;
        return -err;
    }

    int32_t syscall(int32_t sys_id)
    {
        // Save the arguments for tracing
        int32_t args[6] = {0};
        Task *current_task = scheduler.get_current_task();
        memcpy(args, current_task->emulator.x + 10, sizeof(args));

        int32_t result;
        switch (sys_id)
        {
        case SyscallID::EXIT:
            result = syscall(sys_exit);
            trace_syscall<>("exit", args, result);
            break;
        case SyscallID::GETPID:
            result = syscall(sys_getpid);
            trace_syscall<>("getpid", args, result);
            break;
        case SyscallID::GETTID:
            result = syscall(sys_gettid);
            trace_syscall<>("gettid", args, result);
            break;
        case SyscallID::SETPGID:
            result = syscall(sys_setpgid);
            trace_syscall<>("setpgid", args, result);
            break;
        case SyscallID::GETPGID:
            result = syscall(sys_getpgid);
            trace_syscall<>("getpgid", args, result);
            break;
        case SyscallID::GETSID:
            result = syscall(sys_getsid);
            trace_syscall<>("getsid", args, result);
            break;
        case SyscallID::SETSID:
            result = syscall(sys_setsid);
            trace_syscall<>("setsid", args, result);
            break;
        case SyscallID::SCHED_YIELD:
            result = syscall(sys_sched_yield);
            trace_syscall<>("sched_yield", args, result);
            break;
        case SyscallID::GETPPID:
            result = syscall(sys_getppid);
            trace_syscall<>("getppid", args, result);
            break;
        case SyscallID::CLONE:
            result = syscall(sys_clone);
            trace_syscall<>("clone", args, result);
            break;
        case SyscallID::EXECVE:
            result = syscall(sys_execve);
            trace_syscall<>("execve", args, result);
            break;
        case SyscallID::EXECVEAT:
            result = syscall(sys_execveat);
            trace_syscall<>("execveat", args, result);
            break;
        case SyscallID::WAITID:
            result = syscall(sys_waitid);
            trace_syscall<>("waitid", args, result);
            break;
        case SyscallID::WAIT4:
            result = syscall(sys_wait4);
            trace_syscall<>("wait4", args, result);
            break;
        case SyscallID::KILL:
            result = syscall(sys_kill);
            trace_syscall<>("kill", args, result);
            break;
        case SyscallID::TGKILL:
            result = syscall(sys_tgkill);
            trace_syscall<>("tgkill", args, result);
            break;
        case SyscallID::GETRANDOM:
            result = syscall(sys_getrandom);
            trace_syscall<>("getrandom", args, result);
            break;
        case SyscallID::SETUID:
            result = syscall(sys_setuid);
            trace_syscall<>("setuid", args, result);
            break;
        case SyscallID::SETREUID:
            result = syscall(sys_setreuid);
            trace_syscall<>("setreuid", args, result);
            break;
        case SyscallID::SETRESUID:
            result = syscall(sys_setresuid);
            trace_syscall<>("setresuid", args, result);
            break;
        case SyscallID::SETGID:
            result = syscall(sys_setgid);
            trace_syscall<>("setgid", args, result);
            break;
        case SyscallID::SETREGID:
            result = syscall(sys_setregid);
            trace_syscall<>("setregid", args, result);
            break;
        case SyscallID::SETRESGID:
            result = syscall(sys_setresgid);
            trace_syscall<>("setresgid", args, result);
            break;
        case SyscallID::OPENAT:
            result = syscall(sys_openat);
            trace_syscall<
                SysTraceParam{"dirfd", PT_INT},
                SysTraceParam{"path", PT_STR},
                SysTraceParam{"flags", PT_UINT},
                SysTraceParam{"mode", PT_INT}>("openat", args, result);
            break;
        case SyscallID::READ:
            result = syscall(sys_read);
            trace_syscall<>("read", args, result);
            break;
        case SyscallID::WRITE:
            result = syscall(sys_write);
            trace_syscall<>("write", args, result);
            break;
        case SyscallID::CLOSE:
            result = syscall(sys_close);
            trace_syscall<>("close", args, result);
            break;
        case SyscallID::SENDFILE64:
            result = syscall(sys_sendfile64);
            trace_syscall<>("sendfile64", args, result);
            break;
        case SyscallID::SPLICE:
            result = syscall(sys_splice);
            trace_syscall<>("splice", args, result);
            break;
        case SyscallID::STATFS:
            result = syscall(sys_statfs);
            trace_syscall<>("statfs", args, result);
            break;
        case SyscallID::FSTATFS:
            result = syscall(sys_fstatfs);
            trace_syscall<>("fstatfs", args, result);
            break;
        case SyscallID::MOUNT:
            result = syscall(sys_mount);
            trace_syscall<>("mount", args, result);
            break;
        case SyscallID::UMOUNT2:
            result = syscall(sys_umount2);
            trace_syscall<>("umount2", args, result);
            break;
        case SyscallID::FCHOWNAT:
            result = syscall(sys_fchownat);
            trace_syscall<>("fchownat", args, result);
            break;
        case SyscallID::FCHOWN:
            result = syscall(sys_fchown);
            trace_syscall<>("fchown", args, result);
            break;
        case SyscallID::FCHMODAT:
            result = syscall(sys_fchmodat);
            trace_syscall<>("fchmodat", args, result);
            break;
        case SyscallID::FCHMOD:
            result = syscall(sys_fchmod);
            trace_syscall<>("fchmod", args, result);
            break;
        case SyscallID::FTRUNCATE64:
            result = syscall(sys_ftruncate64);
            trace_syscall<>("ftruncate64", args, result);
            break;
        case SyscallID::TRUNCATE64:
            result = syscall(sys_truncate64);
            trace_syscall<>("truncate64", args, result);
            break;
        case SyscallID::LLSEEK:
            result = syscall(sys_llseek);
            trace_syscall<>("llseek", args, result);
            break;
        case SyscallID::NEWFSTATAT:
            result = syscall(sys_newfstatat);
            trace_syscall<>("newfstatat", args, result);
            break;
        case SyscallID::NEWFSTAT:
            result = syscall(sys_newfstat);
            trace_syscall<>("newfstat", args, result);
            break;
        case SyscallID::DUP:
            result = syscall(sys_dup);
            trace_syscall<>("dup", args, result);
            break;
        case SyscallID::DUP3:
            result = syscall(sys_dup3);
            trace_syscall<>("dup3", args, result);
            break;
        case SyscallID::MKDIRAT:
            result = syscall(sys_mkdirat);
            trace_syscall<>("mkdirat", args, result);
            break;
        case SyscallID::UNLINKAT:
            result = syscall(sys_unlinkat);
            trace_syscall<>("unlinkat", args, result);
            break;
        case SyscallID::LINKAT:
            result = syscall(sys_linkat);
            trace_syscall<>("linkat", args, result);
            break;
        case SyscallID::RENAMEAT:
            result = syscall(sys_renameat);
            trace_syscall<>("renameat", args, result);
            break;
        case SyscallID::RENAMEAT2:
            result = syscall(sys_renameat2);
            trace_syscall<>("renameat2", args, result);
            break;
        case SyscallID::GETDENTS64:
            result = syscall(sys_getdents64);
            trace_syscall<>("getdents64", args, result);
            break;
        case SyscallID::CHDIR:
            result = syscall(sys_chdir);
            trace_syscall<>("chdir", args, result);
            break;
        case SyscallID::GETCWD:
            result = syscall(sys_getcwd);
            trace_syscall<>("getcwd", args, result);
            break;
        case SyscallID::FACCESSAT:
            result = syscall(sys_faccessat);
            trace_syscall<>("faccessat", args, result);
            break;
        case SyscallID::FACCESSAT2:
            result = syscall(sys_faccessat2);
            trace_syscall<>("faccessat2", args, result);
            break;
        case SyscallID::PIPE2:
            result = syscall(sys_pipe2);
            trace_syscall<>("pipe2", args, result);
            break;
        case SyscallID::BRK:
            result = syscall(sys_brk);
            trace_syscall<>("brk", args, result);
            break;
        case SyscallID::MMAP2:
            result = syscall(sys_mmap2);
            trace_syscall<>("mmap2", args, result);
            break;
        case SyscallID::MREMAP:
            result = syscall(sys_mremap);
            trace_syscall<>("mremap", args, result);
            break;
        case SyscallID::MUNMAP:
            result = syscall(sys_munmap);
            trace_syscall<>("munmap", args, result);
            break;
        case SyscallID::MPROTECT:
            result = syscall(sys_mprotect);
            trace_syscall<>("mprotect", args, result);
            break;
        case SyscallID::STATX:
            result = syscall(sys_statx);
            trace_syscall<>("statx", args, result);
            break;
        case SyscallID::READLINKAT:
            result = syscall(sys_readlinkat);
            trace_syscall<>("readlinkat", args, result);
            break;
        case SyscallID::SYMLINKAT:
            result = syscall(sys_symlinkat);
            trace_syscall<>("symlinkat", args, result);
            break;
        case SyscallID::GETUID:
            result = syscall(sys_getuid);
            trace_syscall<>("getuid", args, result);
            break;
        case SyscallID::GETEUID:
            result = syscall(sys_geteuid);
            trace_syscall<>("geteuid", args, result);
            break;
        case SyscallID::GETRESUID:
            result = syscall(sys_getresuid);
            trace_syscall<>("getresuid", args, result);
            break;
        case SyscallID::GETGID:
            result = syscall(sys_getgid);
            trace_syscall<>("getgid", args, result);
            break;
        case SyscallID::GETEGID:
            result = syscall(sys_getegid);
            trace_syscall<>("getegid", args, result);
            break;
        case SyscallID::GETRESGID:
            result = syscall(sys_getresgid);
            trace_syscall<>("getresgid", args, result);
            break;
        case SyscallID::GETGROUPS:
            result = syscall(sys_getgroups);
            trace_syscall<>("getgroups", args, result);
            break;
        case SyscallID::SETGROUPS:
            result = syscall(sys_setgroups);
            trace_syscall<>("setgroups", args, result);
            break;
        case SyscallID::IOCTL:
            result = syscall(sys_ioctl);
            trace_syscall<>("ioctl", args, result);
            break;
        case SyscallID::FCNTL64:
            result = syscall(sys_fcntl64);
            trace_syscall<>("fcntl64", args, result);
            break;
        case SyscallID::PRCTL:
            result = syscall(sys_prctl);
            trace_syscall<>("prctl", args, result);
            break;
        case SyscallID::EXIT_GROUP:
            result = syscall(sys_exit_group);
            trace_syscall<>("exit_group", args, result);
            break;
        case SyscallID::RT_SIGACTION:
            result = syscall(sys_rt_sigaction);
            trace_syscall<>("rt_sigaction", args, result);
            break;
        case SyscallID::RT_SIGRETURN:
            result = syscall(sys_rt_sigreturn);
            trace_syscall<>("rt_sigreturn", args, result);
            break;
        case SyscallID::RT_SIGPROCMASK:
            result = syscall(sys_rt_sigprocmask);
            trace_syscall<>("rt_sigprocmask", args, result);
            break;
        case SyscallID::RT_SIGPENDING:
            result = syscall(sys_rt_sigpending);
            trace_syscall<>("rt_sigpending", args, result);
            break;
        case SyscallID::RT_SIGTIMEDWAIT_TIME64:
            result = syscall(sys_rt_sigtimedwait_time64);
            trace_syscall<>("rt_sigtimedwait_time64", args, result);
            break;
        case SyscallID::RT_SIGQUEUEINFO:
            result = syscall(sys_rt_sigqueueinfo);
            trace_syscall<>("rt_sigqueueinfo", args, result);
            break;
        case SyscallID::RT_SIGSUSPEND:
            result = syscall(sys_rt_sigsuspend);
            trace_syscall<>("rt_sigsuspend", args, result);
            break;
        case SyscallID::UNAME:
            result = syscall(sys_uname);
            trace_syscall<>("uname", args, result);
            break;
        case SyscallID::PSELECT6_TIME64:
            result = syscall(sys_pselect6_time64);
            trace_syscall<>("pselect6_time64", args, result);
            break;
        case SyscallID::FSYNC:
            result = syscall(sys_fsync);
            trace_syscall<>("fsync", args, result);
            break;
        case SyscallID::FDATASYNC:
            result = syscall(sys_fdatasync);
            trace_syscall<>("fdatasync", args, result);
            break;
        case SyscallID::CLOCK_GETRES_TIME64:
            result = syscall(sys_clock_getres_time64);
            trace_syscall<>("clock_getres_time64", args, result);
            break;
        case SyscallID::CLOCK_GETTIME64:
            result = syscall(sys_clock_gettime64);
            trace_syscall<>("clock_gettime64", args, result);
            break;
        case SyscallID::CLOCK_NANOSLEEP_TIME64:
            result = syscall(sys_clock_nanosleep_time64);
            trace_syscall<>("clock_nanosleep_time64", args, result);
            break;
        case SyscallID::CLOCK_SETTIME64:
            result = syscall(sys_clock_settime64);
            trace_syscall<>("clock_settime64", args, result);
            break;
        default:
            // Unsupported syscall ID
            result = -H_ENOSYS;
            break;
        }

        return result;
    }

    // Default implementations for sys_* functions
    // These just return -H_ENOSYS

    __attribute__((weak)) int32_t sys_exit(int32_t status) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_getpid() { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_gettid() { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_setpgid(int32_t pid, int32_t pgid) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_getpgid(int32_t pid) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_getsid(int32_t pid) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_setsid() { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_sched_yield() { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_getppid() { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_clone(uint32_t flags, uint32_t stack_loc, uint32_t ptid_loc,
                                            uint32_t ctid_loc, uint32_t newtls) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_execve(uint32_t filename_loc, uint32_t argv_loc,
                                             uint32_t envp_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_execveat(int32_t dirfd, uint32_t filename_loc,
                                               uint32_t argv_loc, uint32_t envp_loc, int32_t flags) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_waitid(int32_t which, int32_t pid, uint32_t infop_loc,
                                             int32_t options, uint32_t ru_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_wait4(int32_t pid, uint32_t status_loc, int32_t options,
                                            uint32_t ru_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_kill(int32_t pid, int32_t sig) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_tgkill(int32_t tgid, int32_t tid, int32_t sig) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_getrandom(uint32_t buf_loc, uint32_t buflen,
                                                uint32_t flags) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_setuid(uint32_t uid) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_setreuid(uint32_t ruid, uint32_t euid) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_setresuid(uint32_t ruid, uint32_t euid, uint32_t suid) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_setgid(uint32_t gid) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_setregid(uint32_t rgid, uint32_t egid) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_setresgid(uint32_t rgid, uint32_t egid, uint32_t sgid) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_openat(int32_t dfd, uint32_t pathname_loc, int32_t flags,
                                             uint32_t mode) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_read(int32_t fd, uint32_t buf_loc, uint32_t count) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_write(int32_t fd, uint32_t buf_loc, uint32_t count) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_close(int32_t fd) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_sendfile64(int32_t out_fd, int32_t in_fd,
                                                 uint32_t offset_loc,
                                                 uint32_t count) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_splice(int32_t fd_in, uint32_t off_in_loc,
                                             int32_t fd_out, uint32_t off_out_loc,
                                             uint32_t len, uint32_t flags) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_statfs(uint32_t path_loc, uint32_t size, uint32_t buf_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_fstatfs(int32_t fd, uint32_t size, uint32_t buf_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_mount(uint32_t source_loc, uint32_t target_loc,
                                            uint32_t filesystemtype_loc, uint32_t mountflags,
                                            uint32_t data_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_umount2(uint32_t target_loc, int32_t flags) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_fchownat(int32_t dirfd, uint32_t pathname_loc,
                                               uint32_t owner, uint32_t group, int32_t flags) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_fchown(int32_t fd, uint32_t owner, uint32_t group) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_fchmodat(int32_t dirfd, uint32_t pathname_loc,
                                               uint32_t mode, int32_t flags) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_fchmod(int32_t fd, uint32_t mode) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_ftruncate64(int32_t fd, uint32_t off_high, uint32_t off_low) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_truncate64(uint32_t path_loc, uint32_t off_high,
                                                 uint32_t off_low) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_llseek(int32_t fd, uint32_t off_high, uint32_t off_low,
                                             uint32_t result_loc, int32_t whence) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_newfstatat(int32_t dirfd, uint32_t pathname_loc,
                                                 uint32_t statbuf_loc, int32_t flags) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_newfstat(int32_t fd, uint32_t statbuf_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_dup(int32_t oldfd) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_dup3(int32_t oldfd, int32_t newfd, int32_t flags) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_mkdirat(int32_t dirfd, uint32_t pathname_loc,
                                              uint32_t mode) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_unlinkat(int32_t dirfd, uint32_t pathname_loc,
                                               int32_t flags) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_linkat(int32_t olddirfd, uint32_t oldpathname_loc,
                                             int32_t newdirfd, uint32_t newpathname_loc,
                                             int32_t flags) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_renameat(int32_t olddirfd, uint32_t oldpathname_loc,
                                               int32_t newdirfd, uint32_t newpathname_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_renameat2(int32_t olddirfd, uint32_t oldpathname_loc,
                                                int32_t newdirfd, uint32_t newpathname_loc,
                                                uint32_t flags) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_getdents64(int32_t fd, uint32_t dirp_loc,
                                                 uint32_t count) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_chdir(uint32_t path_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_getcwd(uint32_t buf_loc, uint32_t size) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_faccessat(int32_t dirfd, uint32_t pathname_loc,
                                                int32_t mode) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_faccessat2(int32_t dirfd, uint32_t pathname_loc,
                                                 int32_t mode, int32_t flags) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_pipe2(uint32_t pipefd_loc, int32_t flags) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_brk(uint32_t end_data_segment_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_mmap2(uint32_t addr, uint32_t length,
                                            uint32_t prot, uint32_t flags,
                                            int32_t fd, uint32_t offset) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_mremap(uint32_t old_address, uint32_t old_size,
                                             uint32_t new_size, uint32_t flags,
                                             uint32_t new_address) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_munmap(uint32_t addr, uint32_t length) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_mprotect(uint32_t addr, uint32_t len, int32_t prot) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_statx(int32_t dirfd, uint32_t pathname_loc,
                                            int32_t flags, uint32_t mask,
                                            uint32_t statxbuf_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_readlinkat(int32_t dirfd, uint32_t pathname_loc,
                                                 uint32_t buf_loc, uint32_t bufsiz) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_symlinkat(uint32_t target_loc, int32_t newdirfd,
                                                uint32_t linkpath_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_getuid() { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_geteuid() { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_getresuid(uint32_t ruid_loc,
                                                uint32_t euid_loc, uint32_t suid_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_getgid() { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_getegid() { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_getresgid(uint32_t rgid_loc,
                                                uint32_t egid_loc, uint32_t sgid_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_getgroups(uint32_t size, uint32_t list_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_setgroups(uint32_t size, uint32_t list_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_ioctl(int32_t fd, int32_t request, uint32_t arg) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_fcntl64(int32_t fd, int32_t cmd,
                                              uint32_t arg) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_prctl(int32_t option, uint32_t arg2,
                                            uint32_t arg3, uint32_t arg4,
                                            uint32_t arg5) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_exit_group(int32_t status) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_rt_sigaction(int32_t signum, uint32_t act_loc,
                                                   uint32_t oldact_loc, uint32_t sigsetsize) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_rt_sigpending(uint32_t sigset_loc, uint32_t sigsetsize) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_rt_sigprocmask(int32_t how, uint32_t set_loc,
                                                     uint32_t oldset_loc, uint32_t sigsetsize) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_rt_sigqueueinfo(int32_t pid, int32_t sig,
                                                      uint32_t uinfo_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_rt_sigreturn() { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_rt_sigsuspend(uint32_t unewset_loc, uint32_t sigsetsize) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_rt_sigtimedwait_time64(int32_t sigset_loc,
                                                             uint32_t info_loc, uint32_t timeout_loc,
                                                             uint32_t sigsetsize) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_rt_tgsigqueueinfo(int32_t tgid, int32_t tid, int32_t sig,
                                                        uint32_t uinfo_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_uname(uint32_t buf_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_pselect6_time64(int32_t nfds, uint32_t readfds_loc,
                                                      uint32_t writefds_loc, uint32_t exceptfds_loc,
                                                      uint32_t timeout_loc, uint32_t sigmask_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_fsync(int32_t fd) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_fdatasync(int32_t fd) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_clock_getres_time64(int32_t clock_id, uint32_t res_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_clock_gettime64(int32_t clock_id, uint32_t tp_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_clock_nanosleep_time64(int32_t clock_id, int32_t flags, uint32_t req_loc, uint32_t rem_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_clock_settime64(int32_t clock_id, uint32_t tp_loc) { return -H_ENOSYS; }
} // namespace Hamster
