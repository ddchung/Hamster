
#include <syscall/syscall.hpp>
#include <abi/syscall_id.hpp>
#include <process/scheduler.hpp>
#include <platform/platform.hpp>
#include <errno/errno.h>

namespace Hamster
{
    namespace
    {
        const char *error_names[] = {
            "No Error",                                // 0 - No error
            "EPERM - Operation not permitted",         // 1
            "ENOENT - No such file or directory",      // 2
            "ESRCH - No such process",                 // 3
            "EINTR - Interrupted system call",         // 4
            "EIO - I/O error",                         // 5
            "ENXIO - No such device or address",       // 6
            "E2BIG - Argument list too long",          // 7
            "ENOEXEC - Exec format error",             // 8
            "EBADF - Bad file number",                 // 9
            "ECHILD - No child processes",             // 10
            "EAGAIN - Try again",                      // 11
            "ENOMEM - Out of memory",                  // 12
            "EACCES - Permission denied",              // 13
            "EFAULT - Bad address",                    // 14
            "ENOTBLK - Block device required",         // 15
            "EBUSY - Device or resource busy",         // 16
            "EEXIST - File exists",                    // 17
            "EXDEV - Cross-device link",               // 18
            "ENODEV - No such device",                 // 19
            "ENOTDIR - Not a directory",               // 20
            "EISDIR - Is a directory",                 // 21
            "EINVAL - Invalid argument",               // 22
            "ENFILE - File table overflow",            // 23
            "EMFILE - Too many open files",            // 24
            "ENOTTY - Not a typewriter",               // 25
            "ETXTBSY - Text file busy",                // 26
            "EFBIG - File too large",                  // 27
            "ENOSPC - No space left on device",        // 28
            "ESPIPE - Illegal seek",                   // 29
            "EROFS - Read-only file system",           // 30
            "EMLINK - Too many links",                 // 31
            "EPIPE - Broken pipe",                     // 32
            "EDOM - Math argument out of domain of func", // 33
            "ERANGE - Math result not representable"   // 34
        };
    } // namespace

    int32_t cvt_error()
    {
        int32_t err = error;
        error = 0;
        return -err;
    }

    int32_t syscall(int32_t sys_id)
    {
        int32_t result;
        switch (sys_id)
        {
        case SyscallID::EXIT:
            result = syscall(sys_exit);
            break;
        case SyscallID::GETPID:
            result = syscall(sys_getpid);
            break;
        case SyscallID::GETTID:
            result = syscall(sys_gettid);
            break;
        case SyscallID::SETPGID:
            result = syscall(sys_setpgid);
            break;
        case SyscallID::GETPGID:
            result = syscall(sys_getpgid);
            break;
        case SyscallID::GETSID:
            result = syscall(sys_getsid);
            break;
        case SyscallID::SETSID:
            result = syscall(sys_setsid);
            break;
        case SyscallID::SCHED_YIELD:
            result = syscall(sys_sched_yield);
            break;
        case SyscallID::GETPPID:
            result = syscall(sys_getppid);
            break;
        case SyscallID::CLONE:
            result = syscall(sys_clone);
            break;
        case SyscallID::EXECVE:
            result = syscall(sys_execve);
            break;
        case SyscallID::EXECVEAT:
            result = syscall(sys_execveat);
            break;
        case SyscallID::WAITID:
            result = syscall(sys_waitid);
            break;
        case SyscallID::WAIT4:
            result = syscall(sys_wait4);
            break;
        case SyscallID::KILL:
            result = syscall(sys_kill);
            break;
        case SyscallID::TGKILL:
            result = syscall(sys_tgkill);
            break;
        case SyscallID::GETRANDOM:
            result = syscall(sys_getrandom);
            break;
        case SyscallID::SETUID:
            result = syscall(sys_setuid);
            break;
        case SyscallID::SETREUID:
            result = syscall(sys_setreuid);
            break;
        case SyscallID::SETRESUID:
            result = syscall(sys_setresuid);
            break;
        case SyscallID::SETGID:
            result = syscall(sys_setgid);
            break;
        case SyscallID::SETREGID:
            result = syscall(sys_setregid);
            break;
        case SyscallID::SETRESGID:
            result = syscall(sys_setresgid);
            break;
        case SyscallID::OPENAT:
            result = syscall(sys_openat);
            break;
        case SyscallID::READ:
            result = syscall(sys_read);
            break;
        case SyscallID::WRITE:
            result = syscall(sys_write);
            break;
        case SyscallID::CLOSE:
            result = syscall(sys_close);
            break;
        case SyscallID::SENDFILE64:
            result = syscall(sys_sendfile64);
            break;
        case SyscallID::SPLICE:
            result = syscall(sys_splice);
            break;
        case SyscallID::STATFS:
            result = syscall(sys_statfs);
            break;
        case SyscallID::FSTATFS:
            result = syscall(sys_fstatfs);
            break;
        case SyscallID::MOUNT:
            result = syscall(sys_mount);
            break;
        case SyscallID::UMOUNT2:
            result = syscall(sys_umount2);
            break;
        case SyscallID::FCHOWNAT:
            result = syscall(sys_fchownat);
            break;
        case SyscallID::FCHOWN:
            result = syscall(sys_fchown);
            break;
        case SyscallID::FCHMODAT:
            result = syscall(sys_fchmodat);
            break;
        case SyscallID::FCHMOD:
            result = syscall(sys_fchmod);
            break;
        case SyscallID::FTRUNCATE64:
            result = syscall(sys_ftruncate64);
            break;
        case SyscallID::TRUNCATE64:
            result = syscall(sys_truncate64);
            break;
        case SyscallID::LLSEEK:
            result = syscall(sys_llseek);
            break;
        case SyscallID::NEWFSTATAT:
            result = syscall(sys_newfstatat);
            break;
        case SyscallID::NEWFSTAT:
            result = syscall(sys_newfstat);
            break;
        case SyscallID::DUP:
            result = syscall(sys_dup);
            break;
        case SyscallID::DUP3:
            result = syscall(sys_dup3);
            break;
        case SyscallID::MKDIRAT:
            result = syscall(sys_mkdirat);
            break;
        case SyscallID::UNLINKAT:
            result = syscall(sys_unlinkat);
            break;
        case SyscallID::LINKAT:
            result = syscall(sys_linkat);
            break;
        case SyscallID::RENAMEAT:
            result = syscall(sys_renameat);
            break;
        case SyscallID::RENAMEAT2:
            result = syscall(sys_renameat2);
            break;
        case SyscallID::GETDENTS64:
            result = syscall(sys_getdents64);
            break;
        case SyscallID::CHDIR:
            result = syscall(sys_chdir);
            break;
        case SyscallID::GETCWD:
            result = syscall(sys_getcwd);
            break;
        case SyscallID::FACCESSAT:
            result = syscall(sys_faccessat);
            break;
        case SyscallID::FACCESSAT2:
            result = syscall(sys_faccessat2);
            break;
        case SyscallID::PIPE2:
            result = syscall(sys_pipe2);
            break;
        case SyscallID::BRK:
            result = syscall(sys_brk);
            break;
        case SyscallID::MMAP2:
            result = syscall(sys_mmap2);
            break;
        case SyscallID::MREMAP:
            result = syscall(sys_mremap);
            break;
        case SyscallID::MUNMAP:
            result = syscall(sys_munmap);
            break;
        case SyscallID::MPROTECT:
            result = syscall(sys_mprotect);
            break;
        case SyscallID::STATX:
            result = syscall(sys_statx);
            break;
        case SyscallID::READLINKAT:
            result = syscall(sys_readlinkat);
            break;
        case SyscallID::SYMLINKAT:
            result = syscall(sys_symlinkat);
            break;
        case SyscallID::GETUID:
            result = syscall(sys_getuid);
            break;
        case SyscallID::GETEUID:
            result = syscall(sys_geteuid);
            break;
        case SyscallID::GETRESUID:
            result = syscall(sys_getresuid);
            break;
        case SyscallID::GETGID:
            result = syscall(sys_getgid);
            break;
        case SyscallID::GETEGID:
            result = syscall(sys_getegid);
            break;
        case SyscallID::GETRESGID:
            result = syscall(sys_getresgid);
            break;
        case SyscallID::IOCTL:
            result = syscall(sys_ioctl);
            break;
        case SyscallID::FCNTL64:
            result = syscall(sys_fcntl64);
            break;
        default:
            // Unsupported syscall ID
            result = -ENOSYS;
            break;
        }

        _trace("syscall(%d) = %d", sys_id, result);

        if (result < 0 && result > -128)
        {
            _trace(" (error)");

            if ((size_t)-result < sizeof(error_names) / sizeof(error_names[0]))
            {
                _trace(": %s", error_names[-result]);
            }
        }

        _trace("\n");

        return result;
    }

    // Default implementations for sys_* functions
    // These just return -ENOSYS

    __attribute__((weak)) int32_t sys_exit(int32_t status) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_getpid() { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_gettid() { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_setpgid(int32_t pid, int32_t pgid) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_getpgid(int32_t pid) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_getsid(int32_t pid) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_setsid() { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_sched_yield() { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_getppid() { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_clone(uint32_t flags, uint32_t stack_loc, uint32_t ptid_loc,
                      uint32_t ctid_loc, uint32_t newtls) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_execve(uint32_t filename_loc, uint32_t argv_loc,
                       uint32_t envp_loc) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_execveat(int32_t dirfd, uint32_t filename_loc,
                         uint32_t argv_loc, uint32_t envp_loc, int32_t flags) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_waitid(int32_t which, int32_t pid, uint32_t infop_loc,
                       int32_t options, uint32_t ru_loc) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_wait4(int32_t pid, uint32_t status_loc, int32_t options,
                      uint32_t ru_loc) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_kill(int32_t pid, int32_t sig) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_tgkill(int32_t tgid, int32_t tid, int32_t sig) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_getrandom(uint32_t buf_loc, uint32_t buflen,
                          uint32_t flags) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_setuid(uint32_t uid) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_setreuid(uint32_t ruid, uint32_t euid) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_setresuid(uint32_t ruid, uint32_t euid, uint32_t suid) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_setgid(uint32_t gid) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_setregid(uint32_t rgid, uint32_t egid) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_setresgid(uint32_t rgid, uint32_t egid, uint32_t sgid) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_openat(int32_t dfd, uint32_t pathname_loc, int32_t flags,
                       uint32_t mode) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_read(int32_t fd, uint32_t buf_loc, uint32_t count) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_write(int32_t fd, uint32_t buf_loc, uint32_t count) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_close(int32_t fd) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_sendfile64(int32_t out_fd, int32_t in_fd,
                          uint32_t offset_loc,
                          uint32_t count) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_splice(int32_t fd_in, uint32_t off_in_loc,
                       int32_t fd_out, uint32_t off_out_loc,
                       uint32_t len, uint32_t flags) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_statfs(uint32_t path_loc, uint32_t size, uint32_t buf_loc) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_fstatfs(int32_t fd, uint32_t size, uint32_t buf_loc) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_mount(uint32_t source_loc, uint32_t target_loc,
                      uint32_t filesystemtype_loc, uint32_t mountflags,
                      uint32_t data_loc) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_umount2(uint32_t target_loc, int32_t flags) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_fchownat(int32_t dirfd, uint32_t pathname_loc,
                         uint32_t owner, uint32_t group, int32_t flags) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_fchown(int32_t fd, uint32_t owner, uint32_t group) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_fchmodat(int32_t dirfd, uint32_t pathname_loc,
                         uint32_t mode) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_fchmod(int32_t fd, uint32_t mode) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_ftruncate64(int32_t fd, uint32_t off_high, uint32_t off_low) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_truncate64(uint32_t path_loc, uint32_t off_high,
                           uint32_t off_low) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_llseek(int32_t fd, uint32_t off_high, uint32_t off_low,
                       uint32_t result_loc, int32_t whence) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_newfstatat(int32_t dirfd, uint32_t pathname_loc,
                           uint32_t statbuf_loc, int32_t flags) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_newfstat(int32_t fd, uint32_t statbuf_loc) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_dup(int32_t oldfd) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_dup3(int32_t oldfd, int32_t newfd, int32_t flags) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_mkdirat(int32_t dirfd, uint32_t pathname_loc,
                        uint32_t mode) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_unlinkat(int32_t dirfd, uint32_t pathname_loc,
                         int32_t flags) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_linkat(int32_t olddirfd, uint32_t oldpathname_loc,
                       int32_t newdirfd, uint32_t newpathname_loc,
                       int32_t flags) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_renameat(int32_t olddirfd, uint32_t oldpathname_loc,
                         int32_t newdirfd, uint32_t newpathname_loc) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_renameat2(int32_t olddirfd, uint32_t oldpathname_loc,
                          int32_t newdirfd, uint32_t newpathname_loc,
                          uint32_t flags) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_getdents64(int32_t fd, uint32_t dirp_loc,
                           uint32_t count) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_chdir(uint32_t path_loc) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_getcwd(uint32_t buf_loc, uint32_t size) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_faccessat(int32_t dirfd, uint32_t pathname_loc,
                          int32_t mode) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_faccessat2(int32_t dirfd, uint32_t pathname_loc,
                           int32_t mode, int32_t flags) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_pipe2(uint32_t pipefd_loc, int32_t flags) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_brk(uint32_t end_data_segment_loc) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_mmap2(uint32_t addr, uint32_t length,
                          uint32_t prot, uint32_t flags,
                          int32_t fd, uint32_t offset) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_mremap(uint32_t old_address, uint32_t old_size,
                          uint32_t new_size, uint32_t flags,
                          uint32_t new_address) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_munmap(uint32_t addr, uint32_t length) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_mprotect(uint32_t addr, uint32_t len, int32_t prot) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_statx(int32_t dirfd, uint32_t pathname_loc,
                      int32_t flags, uint32_t mask,
                      uint32_t statxbuf_loc) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_readlinkat(int32_t dirfd, uint32_t pathname_loc,
                           uint32_t buf_loc, uint32_t bufsiz) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_symlinkat(uint32_t target_loc, int32_t newdirfd,
                          uint32_t linkpath_loc) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_getuid() { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_geteuid() { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_getresuid(uint32_t ruid_loc,
                          uint32_t euid_loc, uint32_t suid_loc) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_getgid() { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_getegid() { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_getresgid(uint32_t rgid_loc,
                          uint32_t egid_loc, uint32_t sgid_loc) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_ioctl(int32_t fd, int32_t request, uint32_t arg) { return -ENOSYS; }
    __attribute__((weak)) int32_t sys_fcntl64(int32_t fd, int32_t cmd,
                      uint32_t arg) { return -ENOSYS; }
} // namespace Hamster

