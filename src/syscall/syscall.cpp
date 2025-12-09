
#include <syscall/syscall.hpp>
#include <abi/syscall_id.hpp>
#include <process/task.hpp>
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
            "EPERM - Operation not permitted",
            "ENOENT - No such file or directory",
            "ESRCH - No such process",
            "EINTR - Interrupted system call",
            "EIO - I/O error",
            "ENXIO - No such device or address",
            "E2BIG - Argument list too long",
            "ENOEXEC - Exec format error",
            "EBADF - Bad file number",
            "ECHILD - No child processes",
            "EAGAIN - Try again",
            "ENOMEM - Out of memory",
            "EACCES - Permission denied",
            "EFAULT - Bad address",
            "ENOTBLK - Block device required",
            "EBUSY - Device or resource busy",
            "EEXIST - File exists",
            "EXDEV - Cross-device link",
            "ENODEV - No such device",
            "ENOTDIR - Not a directory",
            "EISDIR - Is a directory",
            "EINVAL - Invalid argument",
            "ENFILE - File table overflow",
            "EMFILE - Too many open files",
            "ENOTTY - Not a typewriter",
            "ETXTBSY - Text file busy",
            "EFBIG - File too large",
            "ENOSPC - No space left on device",
            "ESPIPE - Illegal seek",
            "EROFS - Read-only file system",
            "EMLINK - Too many links",
            "EPIPE - Broken pipe",
            "EDOM - Math argument out of domain of func",
            "ERANGE - Math result not representable",
            "EDEADLK - Resource deadlock would occur",
            "ENAMETOOLONG - File name too long",
            "ENOLCK - No record locks available",
            "ENOSYS - Invalid system call number",
            "ENOTEMPTY - Directory not empty",
            "ELOOP - Too many symbolic links encountered",
            "EWOULDBLOCK - Operation would block",
            "ENOMSG - No message of desired type",
            "EIDRM - Identifier removed",
            "ECHRNG - Channel number out of range",
            "EL2NSYNC - Level 2 not synchronized",
            "EL3HLT - Level 3 halted",
            "EL3RST - Level 3 reset",
            "ELNRNG - Link number out of range",
            "EUNATCH - Protocol driver not attached",
            "ENOCSI - No CSI structure available",
            "EL2HLT - Level 2 halted",
            "EBADE - Invalid exchange",
            "EBADR - Invalid request descriptor",
            "EXFULL - Exchange full",
            "ENOANO - No anode",
            "EBADRQC - Invalid request code",
            "EBADSLT - Invalid slot",
            "EBFONT - Bad font file format",
            "ENOSTR - Device not a stream",
            "ENODATA - No data available",
            "ETIME - Timer expired",
            "ENOSR - Out of streams resources",
            "ENONET - Machine is not on the network",
            "ENOPKG - Package not installed",
            "EREMOTE - Object is remote",
            "ENOLINK - Link has been severed",
            "EADV - Advertise error",
            "ESRMNT - Srmount error",
            "ECOMM - Communication error on send",
            "EPROTO - Protocol error",
            "EMULTIHOP - Multihop attempted",
            "EDOTDOT - RFS specific error",
            "EBADMSG - Not a data message",
            "EOVERFLOW - Value too large for defined data type",
            "ENOTUNIQ - Name not unique on network",
            "EBADFD - File descriptor in bad state",
            "EREMCHG - Remote address changed",
            "ELIBACC - Can not access a needed shared library",
            "ELIBBAD - Accessing a corrupted shared library",
            "ELIBSCN - .lib section in a.out corrupted",
            "ELIBMAX - Attempting to link in too many shared libraries",
            "ELIBEXEC - Cannot exec a shared library directly",
            "EILSEQ - Illegal byte sequence",
            "ERESTART - Interrupted system call should be restarted",
            "ESTRPIPE - Streams pipe error",
            "EUSERS - Too many users",
            "ENOTSOCK - Socket operation on non-socket",
            "EDESTADDRREQ - Destination address required",
            "EMSGSIZE - Message too long",
            "EPROTOTYPE - Protocol wrong type for socket",
            "ENOPROTOOPT - Protocol not available",
            "EPROTONOSUPPORT - Protocol not supported",
            "ESOCKTNOSUPPORT - Socket type not supported",
            "EOPNOTSUPP - Operation not supported on transport endpoint",
            "ENOTSUP - Operation not supported",
            "EPFNOSUPPORT - Protocol family not supported",
            "EAFNOSUPPORT - Address family not supported by protocol",
            "EADDRINUSE - Address already in use",
            "EADDRNOTAVAIL - Cannot assign requested address",
            "ENETDOWN - Network is down",
            "ENETUNREACH - Network is unreachable",
            "ENETRESET - Network dropped connection because of reset",
            "ECONNABORTED - Software caused connection abort",
            "ECONNRESET - Connection reset by peer",
            "ENOBUFS - No buffer space available",
            "EISCONN - Transport endpoint is already connected",
            "ENOTCONN - Transport endpoint is not connected",
            "ESHUTDOWN - Cannot send after transport endpoint shutdown",
            "ETOOMANYREFS - Too many references: cannot splice",
            "ETIMEDOUT - Connection timed out",
            "ECONNREFUSED - Connection refused",
            "EHOSTDOWN - Host is down",
            "EHOSTUNREACH - No route to host",
            "EALREADY - Operation already in progress",
            "EINPROGRESS - Operation now in progress",
            "ESTALE - Stale file handle",
            "EUCLEAN - Structure needs cleaning",
            "ENOTNAM - Not a XENIX named type file",
            "ENAVAIL - No XENIX semaphores available",
            "EISNAM - Is a named type file",
            "EREMOTEIO - Remote I/O error",
            "EDQUOT - Quota exceeded",
            "ENOMEDIUM - No medium found",
            "EMEDIUMTYPE - Wrong medium type",
            "ECANCELED - Operation Canceled",
            "ENOKEY - Required key not available",
            "EKEYEXPIRED - Key has expired",
            "EKEYREVOKED - Key has been revoked",
            "EKEYREJECTED - Key was rejected by service",
            "EOWNERDEAD - Owner died",
            "ENOTRECOVERABLE - State not recoverable",
            "ERFKILL - Operation not possible due to RF-kill",
            "EHWPOISON - Memory page has hardware error"};

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
            const char name[32];
            const uint32_t mask;
            const uint32_t value = mask;
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

        void print_arg(ParamType_INT, Task &, const char *name, uint32_t i)
        {
            _trace("\033[35m%s\033[0m=%" PRIi32, name, (int32_t)i);
        }

        void print_arg(ParamType_UINT, Task &, const char *name, uint32_t i)
        {
            _trace("\033[35m%s\033[0m=%" PRIu32, name, i);
        }

        void print_arg(ParamType_PTR, Task &task, const char *name, uint32_t p)
        {
            _trace("\033[35m%s\033[0m=%08" PRIx32, name, p);

            int perms = task.mem_get_permissions(p);
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

        void print_arg(ParamType_STR, Task &task, const char *name, uint32_t s)
        {
            char *str = task.mem_get_string(s);
            if (str)
            {
                _trace("\033[35m%s\033[0m=\"", name);
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
        void print_arg(ParamType_FLAGS<Flags...>, Task &, const char *name, uint32_t i)
        {
            _trace("\033[35m%s\033[0m=(", name);
            bool first = true;
            for (const auto &flag : {Flags...})
            {
                if ((i & flag.mask) == flag.value)
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
        void trace_syscall(const char *name, Task &task, int32_t *args)
        {
            _trace("TID \033[34m%" PRIu32 "\033[0m\tsystem call \033[32m%s\033[0m,\tpc: 0x%08" PRIx32 "\targs: {", task.get_tid(), name, task.get_emulator().pc);
            ((
                 print_arg(Args.type, task, Args.name, (uint32_t)*args),
                 _trace(",\t"),
                 ++args),
             ...);
        }
#define SysTraceParam(...) \
    SysTraceParam { __VA_ARGS__ }
#else // NTRACE
        template <int...>
        inline void trace_syscall(...)
        {
        }

#define SysTraceParam(...) 0
#endif // NTRACE
    } // namespace

    void trace_syscall_result(Task &task, int32_t result)
    {
        if (task.is_blocking())
        {
            _trace("} -> \033[36m?\033[0m (started blocking)\n");
            return;
        }
        _trace("} -> \033[36m%" PRIi32, result);

        if (result < 0 && (ssize_t)result > -(ssize_t)(sizeof(error_names) / sizeof(error_names[0])))
            _trace("\t\033[31m(error %s)", error_names[-result]);
        _trace("\033[0m\n");
    }

    int32_t cvt_error()
    {
        int32_t err = error;
        error = 0;
        return -err;
    }

    int32_t syscall(Task &task, int32_t sys_id)
    {
        // Save the arguments for tracing
        int32_t args[6] = {0};
        memcpy(args, task.get_emulator().x + 10, sizeof(args));

        int32_t result;
        switch (sys_id)
        {
        case SyscallID::EXIT:
            trace_syscall<SysTraceParam("code", PT_INT)>("exit", task, args);
            result = syscall(task, sys_exit);
            trace_syscall_result(task, result);
            break;
        case SyscallID::GETPID:
            trace_syscall<>("getpid", task, args);
            result = syscall(task, sys_getpid);
            trace_syscall_result(task, result);
            break;
        case SyscallID::GETTID:
            trace_syscall<>("gettid", task, args);
            result = syscall(task, sys_gettid);
            trace_syscall_result(task, result);
            break;
        case SyscallID::SETPGID:
            trace_syscall<
                SysTraceParam("pid", PT_INT),
                SysTraceParam("pgid", PT_INT)>("setpgid", task, args);
            result = syscall(task, sys_setpgid);
            trace_syscall_result(task, result);
            break;
        case SyscallID::GETPGID:
            trace_syscall<SysTraceParam("pid", PT_INT)>("getpgid", task, args);
            result = syscall(task, sys_getpgid);
            trace_syscall_result(task, result);
            break;
        case SyscallID::GETSID:
            trace_syscall<SysTraceParam("pid", PT_INT)>("getsid", task, args);
            result = syscall(task, sys_getsid);
            trace_syscall_result(task, result);
            break;
        case SyscallID::SETSID:
            trace_syscall<>("setsid", task, args);
            result = syscall(task, sys_setsid);
            trace_syscall_result(task, result);
            break;
        case SyscallID::SCHED_YIELD:
            trace_syscall<>("sched_yield", task, args);
            result = syscall(task, sys_sched_yield);
            trace_syscall_result(task, result);
            break;
        case SyscallID::GETPPID:
            trace_syscall<>("getppid", task, args);
            result = syscall(task, sys_getppid);
            trace_syscall_result(task, result);
            break;
        case SyscallID::CLONE:
            trace_syscall<
                SysTraceParam("flags", PT_FLAGS<
                    {"CLONE_SIGNAL", H_CLONE_SIGNAL},
                    {"CLONE_VM", H_CLONE_VM},
                    {"CLONE_FS", H_CLONE_FS},
                    {"CLONE_FILES", H_CLONE_FILES},
                    {"CLONE_SIGHAND", H_CLONE_SIGHAND},
                    {"CLONE_PIDFD", H_CLONE_PIDFD},
                    {"CLONE_PTRACE", H_CLONE_PTRACE},
                    {"CLONE_VFORK", H_CLONE_VFORK},
                    {"CLONE_PARENT", H_CLONE_PARENT},
                    {"CLONE_THREAD", H_CLONE_THREAD},
                    {"CLONE_NEWNS", H_CLONE_NEWNS},
                    {"CLONE_SYSVSEM", H_CLONE_SYSVSEM},
                    {"CLONE_SETTLS", H_CLONE_SETTLS},
                    {"CLONE_PARENT_SETTID", H_CLONE_PARENT_SETTID},
                    {"CLONE_CHILD_CLEARTID", H_CLONE_CHILD_CLEARTID},
                    {"CLONE_DETACHED", H_CLONE_DETACHED},
                    {"CLONE_UNTRACED", H_CLONE_UNTRACED},
                    {"CLONE_CHILD_SETTID", H_CLONE_CHILD_SETTID},
                    {"CLONE_NEWCGROUP", H_CLONE_NEWCGROUP},
                    {"CLONE_NEWUTS", H_CLONE_NEWUTS}
                >),
                SysTraceParam("stack", PT_PTR),
                SysTraceParam("ptid", PT_PTR),
                SysTraceParam("tls", PT_INT),
                SysTraceParam("ctid", PT_PTR)>("clone", task, args);
            result = syscall(task, sys_clone);
            trace_syscall_result(task, result);
            break;
        case SyscallID::EXECVE:
            trace_syscall<
                SysTraceParam("filename", PT_STR),
                SysTraceParam("argv", PT_PTR),
                SysTraceParam("envp", PT_PTR)>("execve", task, args);
            result = syscall(task, sys_execve);
            trace_syscall_result(task, result);
            break;
        case SyscallID::EXECVEAT:
            trace_syscall<
                SysTraceParam("flags", PT_UINT),
                SysTraceParam("filename", PT_STR),
                SysTraceParam("argv", PT_PTR),
                SysTraceParam("envp", PT_PTR),
                SysTraceParam("flags", PT_UINT)>("execveat", task, args);
            result = syscall(task, sys_execveat);
            trace_syscall_result(task, result);
            break;
        case SyscallID::WAITID:
            trace_syscall<
                SysTraceParam("which", PT_INT),
                SysTraceParam("pid", PT_INT),
                SysTraceParam("infop", PT_PTR),
                SysTraceParam("options", PT_INT),
                SysTraceParam("ru", PT_PTR)>("waitid", task, args);
            result = syscall(task, sys_waitid);
            trace_syscall_result(task, result);
            break;
        case SyscallID::WAIT4:
            trace_syscall<
                SysTraceParam("pid", PT_INT),
                SysTraceParam("status", PT_PTR),
                SysTraceParam("options", PT_INT),
                SysTraceParam("ru", PT_PTR)>("wait4", task, args);
            result = syscall(task, sys_wait4);
            trace_syscall_result(task, result);
            break;
        case SyscallID::KILL:
            trace_syscall<
                SysTraceParam("pid", PT_INT),
                SysTraceParam("sig", PT_INT)>("kill", task, args);
            result = syscall(task, sys_kill);
            trace_syscall_result(task, result);
            break;
        case SyscallID::TGKILL:
            trace_syscall<
                SysTraceParam("tgid", PT_INT),
                SysTraceParam("tid", PT_INT),
                SysTraceParam("sig", PT_INT)>("tgkill", task, args);
            result = syscall(task, sys_tgkill);
            trace_syscall_result(task, result);
            break;
        case SyscallID::GETRANDOM:
            trace_syscall<
                SysTraceParam("buf", PT_PTR),
                SysTraceParam("buflen", PT_UINT),
                SysTraceParam("flags", PT_UINT)>("getrandom", task, args);
            result = syscall(task, sys_getrandom);
            trace_syscall_result(task, result);
            break;
        case SyscallID::SETUID:
            trace_syscall<
                SysTraceParam("uid", PT_UINT)>("setuid", task, args);
            result = syscall(task, sys_setuid);
            trace_syscall_result(task, result);
            break;
        case SyscallID::SETREUID:
            trace_syscall<
                SysTraceParam("ruid", PT_UINT),
                SysTraceParam("euid", PT_UINT)>("setreuid", task, args);
            result = syscall(task, sys_setreuid);
            trace_syscall_result(task, result);
            break;
        case SyscallID::SETRESUID:
            trace_syscall<
                SysTraceParam("ruid", PT_UINT),
                SysTraceParam("euid", PT_UINT),
                SysTraceParam("suid", PT_UINT)>("setresuid", task, args);
            result = syscall(task, sys_setresuid);
            trace_syscall_result(task, result);
            break;
        case SyscallID::SETGID:
            trace_syscall<
                SysTraceParam("gid", PT_UINT)>("setgid", task, args);
            result = syscall(task, sys_setgid);
            trace_syscall_result(task, result);
            break;
        case SyscallID::SETREGID:
            trace_syscall<
                SysTraceParam("rgid", PT_UINT),
                SysTraceParam("egid", PT_UINT)>("setregid", task, args);
            result = syscall(task, sys_setregid);
            trace_syscall_result(task, result);
            break;
        case SyscallID::SETRESGID:
            trace_syscall<
                SysTraceParam("rgid", PT_UINT),
                SysTraceParam("egid", PT_UINT),
                SysTraceParam("sgid", PT_UINT)>("setresgid", task, args);
            result = syscall(task, sys_setresgid);
            trace_syscall_result(task, result);
            break;
        case SyscallID::OPENAT:
            trace_syscall<
                SysTraceParam("dirfd", PT_INT),
                SysTraceParam("path", PT_STR),
                SysTraceParam("flags", PT_FLAGS<
                    {"O_RDONLY", OPEN_ACCMODE, OPEN_RDONLY},
                    {"O_WRONLY", OPEN_ACCMODE, OPEN_WRONLY},
                    {"O_RDWR", OPEN_ACCMODE, OPEN_RDWR},
                    {"O_CREAT", OPEN_CREAT},
                    {"O_EXCL", OPEN_EXCL},
                    {"O_NOCTTY", OPEN_NOCTTY},
                    {"O_TRUNC", OPEN_TRUNC},
                    {"O_APPEND", OPEN_APPEND},
                    {"O_NONBLOCK", OPEN_NONBLOCK},
                    {"O_SYNC", OPEN_SYNC},
                    {"O_ASYNC", OPEN_ASYNC},
                    {"O_DIRECTORY", OPEN_DIRECTORY},
                    {"O_NOFOLLOW", OPEN_NOFOLLOW},
                    {"O_CLOEXEC", OPEN_CLOEXEC}
                >),
                SysTraceParam("mode", PT_INT)>("openat", task, args);
            result = syscall(task, sys_openat);
            trace_syscall_result(task, result);
            break;
        case SyscallID::READ:
            trace_syscall<
                SysTraceParam("fd", PT_INT),
                SysTraceParam("buf", PT_PTR),
                SysTraceParam("count", PT_UINT)>("read", task, args);
            result = syscall(task, sys_read);
            trace_syscall_result(task, result);
            break;
        case SyscallID::WRITE:
            trace_syscall<
                SysTraceParam("fd", PT_INT),
                SysTraceParam("buf", PT_PTR),
                SysTraceParam("count", PT_UINT)>("write", task, args);
            result = syscall(task, sys_write);
            trace_syscall_result(task, result);
            break;
        case SyscallID::CLOSE:
            trace_syscall<
                SysTraceParam("fd", PT_INT)>("close", task, args);
            result = syscall(task, sys_close);
            trace_syscall_result(task, result);
            break;
        case SyscallID::SENDFILE64:
            trace_syscall<
                SysTraceParam("out_fd", PT_INT),
                SysTraceParam("in_fd", PT_INT),
                SysTraceParam("offset", PT_PTR),
                SysTraceParam("count", PT_UINT)>("sendfile64", task, args);
            result = syscall(task, sys_sendfile64);
            trace_syscall_result(task, result);
            break;
        case SyscallID::SPLICE:
            trace_syscall<
                SysTraceParam("fd_in", PT_INT),
                SysTraceParam("off_in", PT_PTR),
                SysTraceParam("fd_out", PT_INT),
                SysTraceParam("off_out", PT_PTR),
                SysTraceParam("len", PT_UINT),
                SysTraceParam("flags", PT_UINT)>("splice", task, args);
            result = syscall(task, sys_splice);
            trace_syscall_result(task, result);
            break;
        case SyscallID::STATFS:
            trace_syscall<
                SysTraceParam("path", PT_STR),
                SysTraceParam("size", PT_UINT),
                SysTraceParam("buf", PT_PTR)>("statfs", task, args);
            result = syscall(task, sys_statfs);
            trace_syscall_result(task, result);
            break;
        case SyscallID::FSTATFS:
            trace_syscall<
                SysTraceParam("fd", PT_INT),
                SysTraceParam("size", PT_UINT),
                SysTraceParam("buf", PT_PTR)>("fstatfs", task, args);
            result = syscall(task, sys_fstatfs);
            trace_syscall_result(task, result);
            break;
        case SyscallID::MOUNT:
            trace_syscall<
                SysTraceParam("source", PT_STR),
                SysTraceParam("target", PT_STR),
                SysTraceParam("filesystemtype", PT_STR),
                SysTraceParam("mountflags", PT_UINT),
                SysTraceParam("data", PT_PTR)>("mount", task, args);
            result = syscall(task, sys_mount);
            trace_syscall_result(task, result);
            break;
        case SyscallID::UMOUNT2:
            trace_syscall<
                SysTraceParam("target", PT_STR),
                SysTraceParam("flags", PT_INT)>("umount2", task, args);
            result = syscall(task, sys_umount2);
            trace_syscall_result(task, result);
            break;
        case SyscallID::FCHOWNAT:
            trace_syscall<
                SysTraceParam("dirfd", PT_INT),
                SysTraceParam("pathname", PT_STR),
                SysTraceParam("owner", PT_UINT),
                SysTraceParam("group", PT_UINT),
                SysTraceParam("flags", PT_INT)>("fchownat", task, args);
            result = syscall(task, sys_fchownat);
            trace_syscall_result(task, result);
            break;
        case SyscallID::FCHOWN:
            trace_syscall<
                SysTraceParam("fd", PT_INT),
                SysTraceParam("owner", PT_UINT),
                SysTraceParam("group", PT_UINT)>("fchown", task, args);
            result = syscall(task, sys_fchown);
            trace_syscall_result(task, result);
            break;
        case SyscallID::FCHMODAT:
            trace_syscall<
                SysTraceParam("dirfd", PT_INT),
                SysTraceParam("pathname", PT_STR),
                SysTraceParam("mode", PT_UINT),
                SysTraceParam("flags", PT_INT)>("fchmodat", task, args);
            result = syscall(task, sys_fchmodat);
            trace_syscall_result(task, result);
            break;
        case SyscallID::FCHMOD:
            trace_syscall<
                SysTraceParam("fd", PT_INT),
                SysTraceParam("mode", PT_UINT)>("fchmod", task, args);
            result = syscall(task, sys_fchmod);
            trace_syscall_result(task, result);
            break;
        case SyscallID::FTRUNCATE64:
            trace_syscall<
                SysTraceParam("fd", PT_INT),
                SysTraceParam("off_high", PT_UINT),
                SysTraceParam("off_low", PT_UINT)>("ftruncate64", task, args);
            result = syscall(task, sys_ftruncate64);
            trace_syscall_result(task, result);
            break;
        case SyscallID::TRUNCATE64:
            trace_syscall<
                SysTraceParam("path", PT_STR),
                SysTraceParam("off_high", PT_UINT),
                SysTraceParam("off_low", PT_UINT)>("truncate64", task, args);
            result = syscall(task, sys_truncate64);
            trace_syscall_result(task, result);
            break;
        case SyscallID::LLSEEK:
            trace_syscall<
                SysTraceParam("fd", PT_INT),
                SysTraceParam("off_high", PT_UINT),
                SysTraceParam("off_low", PT_UINT),
                SysTraceParam("result", PT_PTR),
                SysTraceParam("whence", PT_INT)>("llseek", task, args);
            result = syscall(task, sys_llseek);
            trace_syscall_result(task, result);
            break;
        case SyscallID::NEWFSTATAT:
            trace_syscall<
                SysTraceParam("dirfd", PT_INT),
                SysTraceParam("pathname", PT_STR),
                SysTraceParam("statbuf", PT_PTR),
                SysTraceParam("flags", PT_INT)>("newfstatat", task, args);
            result = syscall(task, sys_newfstatat);
            trace_syscall_result(task, result);
            break;
        case SyscallID::NEWFSTAT:
            trace_syscall<
                SysTraceParam("fd", PT_INT),
                SysTraceParam("statbuf", PT_PTR)>("newfstat", task, args);
            result = syscall(task, sys_newfstat);
            trace_syscall_result(task, result);
            break;
        case SyscallID::DUP:
            trace_syscall<
                SysTraceParam("oldfd", PT_INT)>("dup", task, args);
            result = syscall(task, sys_dup);
            trace_syscall_result(task, result);
            break;
        case SyscallID::DUP3:
            trace_syscall<
                SysTraceParam("oldfd", PT_INT),
                SysTraceParam("newfd", PT_INT),
                SysTraceParam("flags", PT_INT)>("dup3", task, args);
            result = syscall(task, sys_dup3);
            trace_syscall_result(task, result);
            break;
        case SyscallID::MKDIRAT:
            trace_syscall<
                SysTraceParam("dirfd", PT_INT),
                SysTraceParam("pathname", PT_STR),
                SysTraceParam("mode", PT_UINT)>("mkdirat", task, args);
            result = syscall(task, sys_mkdirat);
            trace_syscall_result(task, result);
            break;
        case SyscallID::UNLINKAT:
            trace_syscall<
                SysTraceParam("dirfd", PT_INT),
                SysTraceParam("pathname", PT_STR),
                SysTraceParam("flags", PT_INT)>("unlinkat", task, args);
            result = syscall(task, sys_unlinkat);
            trace_syscall_result(task, result);
            break;
        case SyscallID::LINKAT:
            trace_syscall<
                SysTraceParam("olddirfd", PT_INT),
                SysTraceParam("oldpathname", PT_STR),
                SysTraceParam("newdirfd", PT_INT),
                SysTraceParam("newpathname", PT_STR),
                SysTraceParam("flags", PT_INT)>("linkat", task, args);
            result = syscall(task, sys_linkat);
            trace_syscall_result(task, result);
            break;
        case SyscallID::RENAMEAT:
            trace_syscall<
                SysTraceParam("olddirfd", PT_INT),
                SysTraceParam("oldpathname", PT_STR),
                SysTraceParam("newdirfd", PT_INT),
                SysTraceParam("newpathname", PT_STR)>("renameat", task, args);
            result = syscall(task, sys_renameat);
            trace_syscall_result(task, result);
            break;
        case SyscallID::RENAMEAT2:
            trace_syscall<
                SysTraceParam("olddirfd", PT_INT),
                SysTraceParam("oldpathname", PT_STR),
                SysTraceParam("newdirfd", PT_INT),
                SysTraceParam("newpathname", PT_STR),
                SysTraceParam("flags", PT_UINT)>("renameat2", task, args);
            result = syscall(task, sys_renameat2);
            trace_syscall_result(task, result);
            break;
        case SyscallID::GETDENTS64:
            trace_syscall<
                SysTraceParam("fd", PT_INT),
                SysTraceParam("dirp", PT_PTR),
                SysTraceParam("count", PT_UINT)>("getdents64", task, args);
            result = syscall(task, sys_getdents64);
            trace_syscall_result(task, result);
            break;
        case SyscallID::CHDIR:
            trace_syscall<
                SysTraceParam("path", PT_STR)>("chdir", task, args);
            result = syscall(task, sys_chdir);
            trace_syscall_result(task, result);
            break;
        case SyscallID::GETCWD:
            trace_syscall<
                SysTraceParam("buf", PT_PTR),
                SysTraceParam("size", PT_UINT)>("getcwd", task, args);
            result = syscall(task, sys_getcwd);
            trace_syscall_result(task, result);
            break;
        case SyscallID::FACCESSAT:
            trace_syscall<
                SysTraceParam("dirfd", PT_INT),
                SysTraceParam("pathname", PT_STR),
                SysTraceParam("mode", PT_INT)>("faccessat", task, args);
            result = syscall(task, sys_faccessat);
            trace_syscall_result(task, result);
            break;
        case SyscallID::FACCESSAT2:
            trace_syscall<
                SysTraceParam("dirfd", PT_INT),
                SysTraceParam("pathname", PT_STR),
                SysTraceParam("mode", PT_INT),
                SysTraceParam("flags", PT_INT)>("faccessat2", task, args);
            result = syscall(task, sys_faccessat2);
            trace_syscall_result(task, result);
            break;
        case SyscallID::PIPE2:
            trace_syscall<
                SysTraceParam("pipefd", PT_PTR),
                SysTraceParam("flags", PT_INT)>("pipe2", task, args);
            result = syscall(task, sys_pipe2);
            trace_syscall_result(task, result);
            break;
        case SyscallID::BRK:
            trace_syscall<
                SysTraceParam("end_data_segment", PT_PTR)>("brk", task, args);
            result = syscall(task, sys_brk);
            trace_syscall_result(task, result);
            break;
        case SyscallID::MMAP2:
            trace_syscall<
                SysTraceParam("addr", PT_PTR),
                SysTraceParam("length", PT_UINT),
                SysTraceParam("prot", PT_UINT),
                SysTraceParam("flags", PT_UINT),
                SysTraceParam("fd", PT_INT),
                SysTraceParam("offset", PT_UINT)>("mmap2", task, args);
            result = syscall(task, sys_mmap2);
            trace_syscall_result(task, result);
            break;
        case SyscallID::MREMAP:
            trace_syscall<
                SysTraceParam("old_address", PT_PTR),
                SysTraceParam("old_size", PT_UINT),
                SysTraceParam("new_size", PT_UINT),
                SysTraceParam("flags", PT_UINT),
                SysTraceParam("new_address", PT_PTR)>("mremap", task, args);
            result = syscall(task, sys_mremap);
            trace_syscall_result(task, result);
            break;
        case SyscallID::MUNMAP:
            trace_syscall<
                SysTraceParam("addr", PT_PTR),
                SysTraceParam("length", PT_UINT)>("munmap", task, args);
            result = syscall(task, sys_munmap);
            trace_syscall_result(task, result);
            break;
        case SyscallID::MPROTECT:
            trace_syscall<
                SysTraceParam("addr", PT_PTR),
                SysTraceParam("len", PT_UINT),
                SysTraceParam("prot", PT_INT)>("mprotect", task, args);
            result = syscall(task, sys_mprotect);
            trace_syscall_result(task, result);
            break;
        case SyscallID::STATX:
            trace_syscall<
                SysTraceParam("dirfd", PT_INT),
                SysTraceParam("pathname", PT_STR),
                SysTraceParam("flags", PT_INT),
                SysTraceParam("mask", PT_UINT),
                SysTraceParam("statxbuf", PT_PTR)>("statx", task, args);
            result = syscall(task, sys_statx);
            trace_syscall_result(task, result);
            break;
        case SyscallID::READLINKAT:
            trace_syscall<
                SysTraceParam("dirfd", PT_INT),
                SysTraceParam("pathname", PT_STR),
                SysTraceParam("buf", PT_PTR),
                SysTraceParam("bufsiz", PT_UINT)>("readlinkat", task, args);
            result = syscall(task, sys_readlinkat);
            trace_syscall_result(task, result);
            break;
        case SyscallID::SYMLINKAT:
            trace_syscall<
                SysTraceParam("target", PT_STR),
                SysTraceParam("newdirfd", PT_INT),
                SysTraceParam("linkpath", PT_STR)>("symlinkat", task, args);
            result = syscall(task, sys_symlinkat);
            trace_syscall_result(task, result);
            break;
        case SyscallID::GETUID:
            trace_syscall<>("getuid", task, args);
            result = syscall(task, sys_getuid);
            trace_syscall_result(task, result);
            break;
        case SyscallID::GETEUID:
            trace_syscall<>("geteuid", task, args);
            result = syscall(task, sys_geteuid);
            trace_syscall_result(task, result);
            break;
        case SyscallID::GETRESUID:
            trace_syscall<
                SysTraceParam("ruid", PT_PTR),
                SysTraceParam("euid", PT_PTR),
                SysTraceParam("suid", PT_PTR)>("getresuid", task, args);
            result = syscall(task, sys_getresuid);
            trace_syscall_result(task, result);
            break;
        case SyscallID::GETGID:
            trace_syscall<>("getgid", task, args);
            result = syscall(task, sys_getgid);
            trace_syscall_result(task, result);
            break;
        case SyscallID::GETEGID:
            trace_syscall<>("getegid", task, args);
            result = syscall(task, sys_getegid);
            trace_syscall_result(task, result);
            break;
        case SyscallID::GETRESGID:
            trace_syscall<
                SysTraceParam("rgid", PT_PTR),
                SysTraceParam("egid", PT_PTR),
                SysTraceParam("sgid", PT_PTR)>("getresgid", task, args);
            result = syscall(task, sys_getresgid);
            trace_syscall_result(task, result);
            break;
        case SyscallID::GETGROUPS:
            trace_syscall<
                SysTraceParam("size", PT_UINT),
                SysTraceParam("list", PT_PTR)>("getgroups", task, args);
            result = syscall(task, sys_getgroups);
            trace_syscall_result(task, result);
            break;
        case SyscallID::SETGROUPS:
            trace_syscall<
                SysTraceParam("size", PT_UINT),
                SysTraceParam("list", PT_PTR)>("setgroups", task, args);
            result = syscall(task, sys_setgroups);
            trace_syscall_result(task, result);
            break;
        case SyscallID::IOCTL:
            trace_syscall<
                SysTraceParam("fd", PT_INT),
                SysTraceParam("request", PT_INT),
                SysTraceParam("arg", PT_PTR)>("ioctl", task, args);
            result = syscall(task, sys_ioctl);
            trace_syscall_result(task, result);
            break;
        case SyscallID::FCNTL64:
            trace_syscall<
                SysTraceParam("fd", PT_INT),
                SysTraceParam("cmd", PT_INT),
                SysTraceParam("arg", PT_PTR)>("fcntl64", task, args);
            result = syscall(task, sys_fcntl64);
            trace_syscall_result(task, result);
            break;
        case SyscallID::PRCTL:
            trace_syscall<
                SysTraceParam("option", PT_INT),
                SysTraceParam("arg2", PT_PTR),
                SysTraceParam("arg3", PT_PTR),
                SysTraceParam("arg4", PT_PTR),
                SysTraceParam("arg5", PT_PTR)>("prctl", task, args);
            result = syscall(task, sys_prctl);
            trace_syscall_result(task, result);
            break;
        case SyscallID::EXIT_GROUP:
            trace_syscall<
                SysTraceParam("status", PT_INT)>("exit_group", task, args);
            result = syscall(task, sys_exit_group);
            trace_syscall_result(task, result);
            break;
        case SyscallID::RT_SIGACTION:
            trace_syscall<
                SysTraceParam("signum", PT_INT),
                SysTraceParam("act", PT_PTR),
                SysTraceParam("oldact", PT_PTR),
                SysTraceParam("sigsetsize", PT_UINT)>("rt_sigaction", task, args);
            result = syscall(task, sys_rt_sigaction);
            trace_syscall_result(task, result);
            break;
        case SyscallID::RT_SIGRETURN:
            trace_syscall<>("rt_sigreturn", task, args);
            result = syscall(task, sys_rt_sigreturn);
            trace_syscall_result(task, result);
            break;
        case SyscallID::RT_SIGPROCMASK:
            trace_syscall<
                SysTraceParam("how", PT_INT),
                SysTraceParam("set", PT_PTR),
                SysTraceParam("oldset", PT_PTR),
                SysTraceParam("sigsetsize", PT_UINT)>("rt_sigprocmask", task, args);
            result = syscall(task, sys_rt_sigprocmask);
            trace_syscall_result(task, result);
            break;
        case SyscallID::RT_SIGPENDING:
            trace_syscall<
                SysTraceParam("set", PT_PTR),
                SysTraceParam("sigsetsize", PT_UINT)>("rt_sigpending", task, args);
            result = syscall(task, sys_rt_sigpending);
            trace_syscall_result(task, result);
            break;
        case SyscallID::RT_SIGTIMEDWAIT_TIME64:
            trace_syscall<
                SysTraceParam("sigset", PT_PTR),
                SysTraceParam("info", PT_PTR),
                SysTraceParam("timeout", PT_PTR),
                SysTraceParam("sigsetsize", PT_UINT)>("rt_sigtimedwait_time64", task, args);
            result = syscall(task, sys_rt_sigtimedwait_time64);
            trace_syscall_result(task, result);
            break;
        case SyscallID::RT_SIGQUEUEINFO:
            trace_syscall<
                SysTraceParam("tgid", PT_INT),
                SysTraceParam("sig", PT_INT),
                SysTraceParam("info", PT_PTR)>("rt_sigqueueinfo", task, args);
            result = syscall(task, sys_rt_sigqueueinfo);
            trace_syscall_result(task, result);
            break;
        case SyscallID::RT_SIGSUSPEND:
            trace_syscall<
                SysTraceParam("unewset", PT_PTR),
                SysTraceParam("sigsetsize", PT_UINT)>("rt_sigsuspend", task, args);
            result = syscall(task, sys_rt_sigsuspend);
            trace_syscall_result(task, result);
            break;
        case SyscallID::UNAME:
            trace_syscall<
                SysTraceParam("buf", PT_PTR)>("uname", task, args);
            result = syscall(task, sys_uname);
            trace_syscall_result(task, result);
            break;
        case SyscallID::PSELECT6_TIME64:
            trace_syscall<
                SysTraceParam("nfds", PT_INT),
                SysTraceParam("readfds", PT_PTR),
                SysTraceParam("writefds", PT_PTR),
                SysTraceParam("exceptfds", PT_PTR),
                SysTraceParam("timeout", PT_PTR),
                SysTraceParam("sigmask", PT_PTR)>("pselect6_time64", task, args);
            result = syscall(task, sys_pselect6_time64);
            trace_syscall_result(task, result);
            break;
        case SyscallID::FSYNC:
            trace_syscall<
                SysTraceParam("fd", PT_INT)>("fsync", task, args);
            result = syscall(task, sys_fsync);
            trace_syscall_result(task, result);
            break;
        case SyscallID::FDATASYNC:
            trace_syscall<
                SysTraceParam("fd", PT_INT)>("fdatasync", task, args);
            result = syscall(task, sys_fdatasync);
            trace_syscall_result(task, result);
            break;
        case SyscallID::CLOCK_GETRES_TIME64:
            trace_syscall<
                SysTraceParam("clock_id", PT_INT),
                SysTraceParam("res", PT_PTR)>("clock_getres_time64", task, args);
            result = syscall(task, sys_clock_getres_time64);
            trace_syscall_result(task, result);
            break;
        case SyscallID::CLOCK_GETTIME64:
            trace_syscall<
                SysTraceParam("clock_id", PT_INT),
                SysTraceParam("tp", PT_PTR)>("clock_gettime64", task, args);
            result = syscall(task, sys_clock_gettime64);
            trace_syscall_result(task, result);
            break;
        case SyscallID::CLOCK_NANOSLEEP_TIME64:
            trace_syscall<
                SysTraceParam("clock_id", PT_INT),
                SysTraceParam("flags", PT_INT),
                SysTraceParam("req", PT_PTR),
                SysTraceParam("rem", PT_PTR)>("clock_nanosleep_time64", task, args);
            result = syscall(task, sys_clock_nanosleep_time64);
            trace_syscall_result(task, result);
            break;
        case SyscallID::CLOCK_SETTIME64:
            trace_syscall<
                SysTraceParam("clock_id", PT_INT),
                SysTraceParam("tp", PT_PTR)>("clock_settime64", task, args);
            result = syscall(task, sys_clock_settime64);
            trace_syscall_result(task, result);
            break;
        case SyscallID::CHROOT:
            trace_syscall<
                SysTraceParam("path", PT_STR)>("chroot", task, args);
            result = syscall(task, sys_chroot);
            trace_syscall_result(task, result);
            break;
        case SyscallID::CLOSE_RANGE:
            // TODO: close range flags
            trace_syscall<
                SysTraceParam("first", PT_UINT),
                SysTraceParam("last", PT_UINT),
                SysTraceParam("flags", PT_UINT)>("close_range", task, args);
            result = syscall(task, sys_close_range);
            trace_syscall_result(task, result);
            break;
        case SyscallID::COPY_FILE_RANGE:
            // TODO: copy file range flags
            trace_syscall<
                SysTraceParam("fd_in", PT_INT),
                SysTraceParam("off_in", PT_PTR),
                SysTraceParam("fd_out", PT_INT),
                SysTraceParam("off_out", PT_PTR),
                SysTraceParam("len", PT_UINT),
                SysTraceParam("flags", PT_UINT)>("copy_file_range", task, args);
            result = syscall(task, sys_copy_file_range);
            trace_syscall_result(task, result);
            break;
        case SyscallID::GET_ROBUST_LIST:
            trace_syscall<
                SysTraceParam("pid", PT_INT),
                SysTraceParam("head_ptr", PT_PTR),
                SysTraceParam("len", PT_PTR)>("get_robust_list", task, args);
            result = syscall(task, sys_get_robust_list);
            trace_syscall_result(task, result);
            break;
        case SyscallID::PREAD64:
            trace_syscall<
                SysTraceParam("fd", PT_INT),
                SysTraceParam("buf", PT_PTR),
                SysTraceParam("count", PT_UINT),
                SysTraceParam("_pad", PT_UINT),
                SysTraceParam("offset_low", PT_UINT),
                SysTraceParam("offset_high", PT_UINT)>("pread64", task, args);
            result = syscall(task, sys_pread64);
            trace_syscall_result(task, result);
            break;
        case SyscallID::PREADV:
            trace_syscall<
                SysTraceParam("fd", PT_INT),
                SysTraceParam("iovec", PT_PTR),
                SysTraceParam("iovcnt", PT_UINT),
                SysTraceParam("offset_low", PT_UINT),
                SysTraceParam("offset_high", PT_UINT)>("preadv", task, args);
            result = syscall(task, sys_preadv);
            trace_syscall_result(task, result);
            break;
        case SyscallID::PREADV2:
            // TODO: preadv2 flags
            trace_syscall<
                SysTraceParam("fd", PT_INT),
                SysTraceParam("iovec", PT_PTR),
                SysTraceParam("iovcnt", PT_UINT),
                SysTraceParam("offset_low", PT_UINT),
                SysTraceParam("offset_high", PT_UINT),
                SysTraceParam("flags", PT_UINT)>("preadv2", task, args);
            result = syscall(task, sys_preadv2);
            trace_syscall_result(task, result);
            break;
        case SyscallID::PWRITE64:
            trace_syscall<
                SysTraceParam("fd", PT_INT),
                SysTraceParam("buf", PT_PTR),
                SysTraceParam("count", PT_UINT),
                SysTraceParam("_pad", PT_UINT),
                SysTraceParam("offset_low", PT_UINT),
                SysTraceParam("offset_high", PT_UINT)>("pwrite64", task, args);
            result = syscall(task, sys_pwrite64);
            trace_syscall_result(task, result);
            break;
        case SyscallID::PWRITEV:
            trace_syscall<
                SysTraceParam("fd", PT_INT),
                SysTraceParam("iovec", PT_PTR),
                SysTraceParam("iovcnt", PT_UINT),
                SysTraceParam("offset_low", PT_UINT),
                SysTraceParam("offset_high", PT_UINT)>("pwritev", task, args);
            result = syscall(task, sys_pwritev);
            trace_syscall_result(task, result);
            break;
        case SyscallID::PWRITEV2:
            // TODO: pwritev2 flags
            trace_syscall<
                SysTraceParam("fd", PT_INT),
                SysTraceParam("iovec", PT_PTR),
                SysTraceParam("iovcnt", PT_UINT),
                SysTraceParam("offset_low", PT_UINT),
                SysTraceParam("offset_high", PT_UINT),
                SysTraceParam("flags", PT_UINT)>("pwritev2", task, args);
            result = syscall(task, sys_pwritev2);
            trace_syscall_result(task, result);
            break;
        case SyscallID::RISCV_FLUSH_ICACHE:
            // TODO: riscv_flush_icache flags
            trace_syscall<
                SysTraceParam("start", PT_PTR),
                SysTraceParam("end", PT_PTR),
                SysTraceParam("flags", PT_UINT)>("riscv_flush_icache", task, args);
            result = syscall(task, sys_riscv_flush_icache);
            trace_syscall_result(task, result);
            break;
        case SyscallID::SET_ROBUST_LIST:
            trace_syscall<
                SysTraceParam("head_ptr", PT_PTR),
                SysTraceParam("len", PT_UINT)>("set_robust_list", task, args);
            result = syscall(task, sys_set_robust_list);
            trace_syscall_result(task, result);
            break;
        case SyscallID::SET_TID_ADDRESS:
            trace_syscall<
                SysTraceParam("tidptr", PT_PTR)>("set_tid_address", task, args);
            result = syscall(task, sys_set_tid_address);
            trace_syscall_result(task, result);
            break;
        case SyscallID::SIGALTSTACK:
            trace_syscall<
                SysTraceParam("ss", PT_PTR),
                SysTraceParam("old_ss", PT_PTR)>("sigaltstack", task, args);
            result = syscall(task, sys_sigaltstack);
            trace_syscall_result(task, result);
            break;
        case SyscallID::FUTEX_TIME64:
            trace_syscall<
                SysTraceParam("uaddr", PT_PTR),
                SysTraceParam("op", PT_INT),
                SysTraceParam("val", PT_UINT),
                SysTraceParam("timeout", PT_PTR),
                SysTraceParam("uaddr2", PT_PTR),
                SysTraceParam("val3", PT_UINT)>("futex_time64", task, args);
            result = syscall(task, sys_futex_time64);
            trace_syscall_result(task, result);
            break;
        case SyscallID::READV:
            trace_syscall<
                SysTraceParam("fd", PT_INT),
                SysTraceParam("iovec", PT_PTR),
                SysTraceParam("iovcnt", PT_UINT)>("readv", task, args);
            result = syscall(task, sys_readv);
            trace_syscall_result(task, result);
            break;
        case SyscallID::WRITEV:
            trace_syscall<
                SysTraceParam("fd", PT_INT),
                SysTraceParam("iovec", PT_PTR),
                SysTraceParam("iovcnt", PT_UINT)>("writev", task, args);
            result = syscall(task, sys_writev);
            trace_syscall_result(task, result);
            break;
        default:
            _trace("TID %" PRIu32 "\tUnknown system call %" PRIu32 " at pc 0x%08" PRIx32 " -> ENOSYS - Invalid system call number\n", task.get_tid(), sys_id, task.get_emulator().pc);
            // Unsupported syscall ID
            result = -H_ENOSYS;
            break;
        }

        return result;
    }

    // Default implementations for sys_* functions
    // These just return -H_ENOSYS

    __attribute__((weak)) int32_t sys_exit(Task &task, int32_t status) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_getpid(Task &task) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_gettid(Task &task) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_setpgid(Task &task, int32_t pid, int32_t pgid) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_getpgid(Task &task, int32_t pid) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_getsid(Task &task, int32_t pid) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_setsid(Task &task) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_sched_yield(Task &task) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_getppid(Task &task) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_clone(Task &task, uint32_t flags, uint32_t stack_loc, uint32_t ptid_loc,
                                            uint32_t ctid_loc, uint32_t newtls) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_execve(Task &task, uint32_t filename_loc, uint32_t argv_loc,
                                             uint32_t envp_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_execveat(Task &task, int32_t dirfd, uint32_t filename_loc,
                                               uint32_t argv_loc, uint32_t envp_loc, int32_t flags) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_waitid(Task &task, int32_t which, int32_t pid, uint32_t infop_loc,
                                             int32_t options, uint32_t ru_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_wait4(Task &task, int32_t pid, uint32_t status_loc, int32_t options,
                                            uint32_t ru_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_kill(Task &task, int32_t pid, int32_t sig) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_tgkill(Task &task, int32_t tgid, int32_t tid, int32_t sig) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_getrandom(Task &task, uint32_t buf_loc, uint32_t buflen,
                                                uint32_t flags) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_setuid(Task &task, uint32_t uid) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_setreuid(Task &task, uint32_t ruid, uint32_t euid) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_setresuid(Task &task, uint32_t ruid, uint32_t euid, uint32_t suid) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_setgid(Task &task, uint32_t gid) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_setregid(Task &task, uint32_t rgid, uint32_t egid) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_setresgid(Task &task, uint32_t rgid, uint32_t egid, uint32_t sgid) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_openat(Task &task, int32_t dfd, uint32_t pathname_loc, int32_t flags,
                                             uint32_t mode) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_read(Task &task, int32_t fd, uint32_t buf_loc, uint32_t count) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_write(Task &task, int32_t fd, uint32_t buf_loc, uint32_t count) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_close(Task &task, int32_t fd) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_sendfile64(Task &task, int32_t out_fd, int32_t in_fd,
                                                 uint32_t offset_loc,
                                                 uint32_t count) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_splice(Task &task, int32_t fd_in, uint32_t off_in_loc,
                                             int32_t fd_out, uint32_t off_out_loc,
                                             uint32_t len, uint32_t flags) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_statfs(Task &task, uint32_t path_loc, uint32_t size, uint32_t buf_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_fstatfs(Task &task, int32_t fd, uint32_t size, uint32_t buf_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_mount(Task &task, uint32_t source_loc, uint32_t target_loc,
                                            uint32_t filesystemtype_loc, uint32_t mountflags,
                                            uint32_t data_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_umount2(Task &task, uint32_t target_loc, int32_t flags) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_fchownat(Task &task, int32_t dirfd, uint32_t pathname_loc,
                                               uint32_t owner, uint32_t group, int32_t flags) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_fchown(Task &task, int32_t fd, uint32_t owner, uint32_t group) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_fchmodat(Task &task, int32_t dirfd, uint32_t pathname_loc,
                                               uint32_t mode, int32_t flags) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_fchmod(Task &task, int32_t fd, uint32_t mode) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_ftruncate64(Task &task, int32_t fd, uint32_t off_high, uint32_t off_low) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_truncate64(Task &task, uint32_t path_loc, uint32_t off_high,
                                                 uint32_t off_low) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_llseek(Task &task, int32_t fd, uint32_t off_high, uint32_t off_low,
                                             uint32_t result_loc, int32_t whence) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_newfstatat(Task &task, int32_t dirfd, uint32_t pathname_loc,
                                                 uint32_t statbuf_loc, int32_t flags) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_newfstat(Task &task, int32_t fd, uint32_t statbuf_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_dup(Task &task, int32_t oldfd) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_dup3(Task &task, int32_t oldfd, int32_t newfd, int32_t flags) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_mkdirat(Task &task, int32_t dirfd, uint32_t pathname_loc,
                                              uint32_t mode) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_unlinkat(Task &task, int32_t dirfd, uint32_t pathname_loc,
                                               int32_t flags) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_linkat(Task &task, int32_t olddirfd, uint32_t oldpathname_loc,
                                             int32_t newdirfd, uint32_t newpathname_loc,
                                             int32_t flags) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_renameat(Task &task, int32_t olddirfd, uint32_t oldpathname_loc,
                                               int32_t newdirfd, uint32_t newpathname_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_renameat2(Task &task, int32_t olddirfd, uint32_t oldpathname_loc,
                                                int32_t newdirfd, uint32_t newpathname_loc,
                                                uint32_t flags) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_getdents64(Task &task, int32_t fd, uint32_t dirp_loc,
                                                 uint32_t count) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_chdir(Task &task, uint32_t path_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_getcwd(Task &task, uint32_t buf_loc, uint32_t size) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_faccessat(Task &task, int32_t dirfd, uint32_t pathname_loc,
                                                int32_t mode) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_faccessat2(Task &task, int32_t dirfd, uint32_t pathname_loc,
                                                 int32_t mode, int32_t flags) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_pipe2(Task &task, uint32_t pipefd_loc, int32_t flags) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_brk(Task &task, uint32_t end_data_segment_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_mmap2(Task &task, uint32_t addr, uint32_t length,
                                            uint32_t prot, uint32_t flags,
                                            int32_t fd, uint32_t offset) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_mremap(Task &task, uint32_t old_address, uint32_t old_size,
                                             uint32_t new_size, uint32_t flags,
                                             uint32_t new_address) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_munmap(Task &task, uint32_t addr, uint32_t length) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_mprotect(Task &task, uint32_t addr, uint32_t len, int32_t prot) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_statx(Task &task, int32_t dirfd, uint32_t pathname_loc,
                                            int32_t flags, uint32_t mask,
                                            uint32_t statxbuf_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_readlinkat(Task &task, int32_t dirfd, uint32_t pathname_loc,
                                                 uint32_t buf_loc, uint32_t bufsiz) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_symlinkat(Task &task, uint32_t target_loc, int32_t newdirfd,
                                                uint32_t linkpath_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_getuid(Task &task) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_geteuid(Task &task) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_getresuid(Task &task, uint32_t ruid_loc,
                                                uint32_t euid_loc, uint32_t suid_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_getgid(Task &task) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_getegid(Task &task) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_getresgid(Task &task, uint32_t rgid_loc,
                                                uint32_t egid_loc, uint32_t sgid_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_getgroups(Task &task, uint32_t size, uint32_t list_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_setgroups(Task &task, uint32_t size, uint32_t list_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_ioctl(Task &task, int32_t fd, int32_t request, uint32_t arg) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_fcntl64(Task &task, int32_t fd, int32_t cmd,
                                              uint32_t arg) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_prctl(Task &task, int32_t option, uint32_t arg2,
                                            uint32_t arg3, uint32_t arg4,
                                            uint32_t arg5) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_exit_group(Task &task, int32_t status) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_rt_sigaction(Task &task, int32_t signum, uint32_t act_loc,
                                                   uint32_t oldact_loc, uint32_t sigsetsize) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_rt_sigpending(Task &task, uint32_t sigset_loc, uint32_t sigsetsize) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_rt_sigprocmask(Task &task, int32_t how, uint32_t set_loc,
                                                     uint32_t oldset_loc, uint32_t sigsetsize) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_rt_sigqueueinfo(Task &task, int32_t pid, int32_t sig,
                                                      uint32_t uinfo_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_rt_sigreturn(Task &task) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_rt_sigsuspend(Task &task, uint32_t unewset_loc, uint32_t sigsetsize) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_rt_sigtimedwait_time64(Task &task, int32_t sigset_loc,
                                                             uint32_t info_loc, uint32_t timeout_loc,
                                                             uint32_t sigsetsize) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_rt_tgsigqueueinfo(Task &task, int32_t tgid, int32_t tid, int32_t sig,
                                                        uint32_t uinfo_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_uname(Task &task, uint32_t buf_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_pselect6_time64(Task &task, int32_t nfds, uint32_t readfds_loc,
                                                      uint32_t writefds_loc, uint32_t exceptfds_loc,
                                                      uint32_t timeout_loc, uint32_t sigmask_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_fsync(Task &task, int32_t fd) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_fdatasync(Task &task, int32_t fd) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_clock_getres_time64(Task &task, int32_t clock_id, uint32_t res_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_clock_gettime64(Task &task, int32_t clock_id, uint32_t tp_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_clock_nanosleep_time64(Task &task, int32_t clock_id, int32_t flags, uint32_t req_loc, uint32_t rem_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_clock_settime64(Task &task, int32_t clock_id, uint32_t tp_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_chroot(Task &task, uint32_t path_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_close_range(Task &task, uint32_t first, uint32_t last, uint32_t flags) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_copy_file_range(Task &task, int32_t fd_in, uint32_t off_in_loc,
                                                      int32_t fd_out, uint32_t off_out_loc,
                                                      uint32_t len, uint32_t flags) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_get_robust_list(Task &task, int32_t pid, uint32_t head_ptr_loc, uint32_t len_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_pread64(Task &task, int32_t fd, uint32_t buf_loc, uint32_t count, uint32_t _pad, uint32_t off_low,
                                              uint32_t off_high) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_preadv(Task &task, int32_t fd, uint32_t vec_loc, uint32_t vlen, uint32_t pos_low, uint32_t pos_high) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_preadv2(Task &task, int32_t fd, uint32_t vec_loc, uint32_t vlen, uint32_t pos_low, uint32_t pos_high,
                                              uint32_t flags) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_pwrite64(Task &task, int32_t fd, uint32_t buf_loc, uint32_t count, uint32_t _pad, uint32_t off_low,
                                               uint32_t off_high) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_pwritev(Task &task, int32_t fd, uint32_t vec_loc, uint32_t vlen, uint32_t pos_low, uint32_t pos_high) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_pwritev2(Task &task, int32_t fd, uint32_t vec_loc, uint32_t vlen, uint32_t pos_low, uint32_t pos_high,
                                               uint32_t flags) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_riscv_flush_icache(Task &task, uint32_t start_loc, uint32_t end_loc, uint32_t flags) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_set_robust_list(Task &task, uint32_t head_loc, uint32_t len) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_set_tid_address(Task &task, uint32_t tidptr_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_sigaltstack(Task &task, uint32_t ss_loc, uint32_t old_ss_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_futex_time64(Task &task, uint32_t uaddr_loc, int32_t futex_op, uint32_t val,
                                                   uint32_t timeout_loc, uint32_t uaddr2_loc, uint32_t val3) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_readv(Task &task, int32_t fd, uint32_t vec_loc, uint32_t vlen) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_writev(Task &task, int32_t fd, uint32_t vec_loc, uint32_t vlen) { return -H_ENOSYS; }
} // namespace Hamster
