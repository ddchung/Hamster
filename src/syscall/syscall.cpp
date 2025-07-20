
// Hamster system call switch

#include <syscall/syscall.hpp>
#include <abi/syscall_id.hpp>
#include <errno/errno.h>

static const char *error_names[] = {
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

namespace Hamster
{
    int do_syscall(Thread &thread)
    {
        int syscall_id = thread.get_regs()[17]; // a7 is the syscall ID in RISC-V calling convention
        
        _trace("[INFO] Syscall ID: %d", syscall_id);

        if (syscall_id == 93 || syscall_id == 95 || syscall_id == 260 || syscall_id == 221 || syscall_id == 281)
        {
            // These don't return, so print a newline
            _trace("\n");
        }
        else
        {
            _trace(" returning: ");
        }

        switch (syscall_id)
        {
            case SyscallID::EXIT:
                return sys_exit(thread);
            case SyscallID::GETPID:
                return sys_getpid(thread);
            case SyscallID::GETPPID:
                return sys_getppid(thread);
            case SyscallID::CLONE:
                return sys_clone(thread);
            case SyscallID::EXECVE:
                return sys_execve(thread);
            case SyscallID::EXECVEAT:
                return sys_execveat(thread);
            case SyscallID::WAITID:
                return sys_waitid(thread);
            case SyscallID::WAIT4:
                return sys_wait4(thread);
            case SyscallID::KILL:
                return sys_kill(thread);
            case SyscallID::OPENAT:
                return sys_openat(thread);
            case SyscallID::READ:
                return sys_read(thread);
            case SyscallID::WRITE:
                return sys_write(thread);
            case SyscallID::CLOSE:
                return sys_close(thread);
            case SyscallID::LLSEEK:
                return sys_llseek(thread);
            case SyscallID::NEWFSTATAT:
                return sys_newfstatat(thread);
            case SyscallID::NEWFSTAT:
                return sys_newfstat(thread);
            case SyscallID::DUP:
                return sys_dup(thread);
            case SyscallID::DUP3:
                return sys_dup3(thread);
            case SyscallID::MKDIRAT:
                return sys_mkdirat(thread);
            case SyscallID::UNLINKAT:
                return sys_unlinkat(thread);
            case SyscallID::LINKAT:
                return sys_linkat(thread);
            case SyscallID::RENAMEAT:
                return sys_renameat(thread);
            case SyscallID::RENAMEAT2:
                return sys_renameat2(thread);
            case SyscallID::GETDENTS64:
                return sys_getdents64(thread);
            case SyscallID::CHDIR:
                return sys_chdir(thread);
            case SyscallID::GETCWD:
                return sys_getcwd(thread);
            case SyscallID::FACCESSAT:
                return sys_faccessat(thread);
            case SyscallID::PIPE2:
                return sys_pipe2(thread);
            case SyscallID::BRK:
                return sys_brk(thread);
            case SyscallID::MMAP2:
                return sys_mmap2(thread);
            case SyscallID::MUNMAP:
                return sys_munmap(thread);
            case SyscallID::MPROTECT:
                return sys_mprotect(thread);
            case SyscallID::STATX:
                return sys_statx(thread);
            case SyscallID::READLINKAT:
                return sys_readlinkat(thread);
            case SyscallID::SYMLINKAT:
                return sys_symlinkat(thread);
            case SyscallID::GETUID:
                return sys_getuid(thread);
            case SyscallID::GETEUID:
                return sys_geteuid(thread);
            case SyscallID::GETGID:
                return sys_getgid(thread);
            case SyscallID::GETEGID:
                return sys_getegid(thread);
            case SyscallID::IOCTL:
                return sys_ioctl(thread);
            case SyscallID::FCNTL:
                return sys_fcntl(thread);
            default:
                _trace("(Unknown system call) ");
                error = ENOSYS;
                return transfer_error(thread);
        }

        // ???
        return -1;
    }

    uint32_t get_arg(Thread &thread, int index)
    {
        // a7 is used for syscall ID, a0-a6 are used for arguments
        if (index < 0 || index >= 7)
        {
            return 0;
        }

        // trace("[%d = %d] ", index, thread.get_regs()[10 + index]);

        return thread.get_regs()[10 + index]; // a0 is at index 10 in the register array
    }

    int transfer_error(Thread &thread)
    {
        // Transfer the error code to the thread's error code
        int error_code = Hamster::error;
        Hamster::error = 0; // Reset the global error code

        _trace("%d", -error_code);

        if (error_code >= 0 && error_code < sizeof(error_names) / sizeof(error_names[0]) && error_names[error_code])
        {
            _trace(" (%s)", error_names[error_code]);
        }
        else
        {
            _trace(" (Unknown error code %d)", error_code);
        }

        _flush_trace();

        // Set the return value to -error_code
        thread.get_regs()[10] = -error_code;
        _trace("\n");

        return -1;
    }

    int set_return(Thread &thread, uint32_t value)
    {
        // Set the return value in a0 register
        thread.get_regs()[10] = value; // a0 is at index 10 in the register array
        _trace("%d\n", value);
        _flush_trace();
        return 0; // Success
    }

    int deref_fd(Thread &thread, int thread_fd)
    {
        if (thread_fd < 0)
        {
            error = EINVAL;
            return -1;
        }

        Process *proc = thread.get_process();
        if (thread_fd >= (int)proc->fds.size())
        {
            error = EBADF;
            return -1;
        }

        int fd = proc->fds[thread_fd].fd;

        if (fd < 0)
        {
            error = EBADF;
            return -1;
        }

        return fd;
    }

    __attribute__((weak)) int sys_exit(Thread &thread) { return set_return(thread, -ENOSYS); };
    __attribute__((weak)) int sys_getpid(Thread &thread) { return set_return(thread, -ENOSYS); };
    __attribute__((weak)) int sys_clone(Thread &thread) { return set_return(thread, -ENOSYS); };
    __attribute__((weak)) int sys_execve(Thread &thread) { return set_return(thread, -ENOSYS); };
    __attribute__((weak)) int sys_execveat(Thread &thread) { return set_return(thread, -ENOSYS); };
    __attribute__((weak)) int sys_waitid(Thread &thread) { return set_return(thread, -ENOSYS); };
    __attribute__((weak)) int sys_wait4(Thread &thread) { return set_return(thread, -ENOSYS); };
    __attribute__((weak)) int sys_kill(Thread &thread) { return set_return(thread, -ENOSYS); };
    __attribute__((weak)) int sys_openat(Thread &thread) { return set_return(thread, -ENOSYS); };
    __attribute__((weak)) int sys_read(Thread &thread) { return set_return(thread, -ENOSYS); };
    __attribute__((weak)) int sys_write(Thread &thread) { return set_return(thread, -ENOSYS); };
    __attribute__((weak)) int sys_close(Thread &thread) { return set_return(thread, -ENOSYS); };
    __attribute__((weak)) int sys_llseek(Thread &thread) { return set_return(thread, -ENOSYS); };
    __attribute__((weak)) int sys_newfstatat(Thread &thread) { return set_return(thread, -ENOSYS); };
    __attribute__((weak)) int sys_newfstat(Thread &thread) { return set_return(thread, -ENOSYS); };
    __attribute__((weak)) int sys_dup(Thread &thread) { return set_return(thread, -ENOSYS); };
    __attribute__((weak)) int sys_dup3(Thread &thread) { return set_return(thread, -ENOSYS); };
    __attribute__((weak)) int sys_mkdirat(Thread &thread) { return set_return(thread, -ENOSYS); };
    __attribute__((weak)) int sys_unlinkat(Thread &thread) { return set_return(thread, -ENOSYS); };
    __attribute__((weak)) int sys_linkat(Thread &thread) { return set_return(thread, -ENOSYS); };
    __attribute__((weak)) int sys_renameat(Thread &thread) { return set_return(thread, -ENOSYS); };
    __attribute__((weak)) int sys_renameat2(Thread &thread) { return set_return(thread, -ENOSYS); };
    __attribute__((weak)) int sys_getdents64(Thread &thread) { return set_return(thread, -ENOSYS); };
    __attribute__((weak)) int sys_chdir(Thread &thread) { return set_return(thread, -ENOSYS); };
    __attribute__((weak)) int sys_getcwd(Thread &thread) { return set_return(thread, -ENOSYS); };
    __attribute__((weak)) int sys_faccessat(Thread &thread) { return set_return(thread, -ENOSYS); };
    __attribute__((weak)) int sys_pipe2(Thread &thread) { return set_return(thread, -ENOSYS); };
    __attribute__((weak)) int sys_brk(Thread &thread) { return set_return(thread, -ENOSYS); };
    __attribute__((weak)) int sys_mmap2(Thread &thread) { return set_return(thread, -ENOSYS); };
    __attribute__((weak)) int sys_munmap(Thread &thread) { return set_return(thread, -ENOSYS); };
    __attribute__((weak)) int sys_mprotect(Thread &thread) { return set_return(thread, -ENOSYS); };
    __attribute__((weak)) int sys_statx(Thread &thread) { return set_return(thread, -ENOSYS); };
    __attribute__((weak)) int sys_readlinkat(Thread &thread) { return set_return(thread, -ENOSYS); };
    __attribute__((weak)) int sys_symlinkat(Thread &thread) { return set_return(thread, -ENOSYS); };
    __attribute__((weak)) int sys_getuid(Thread &thread) { return set_return(thread, -ENOSYS); };
    __attribute__((weak)) int sys_geteuid(Thread &thread) { return set_return(thread, -ENOSYS); };
    __attribute__((weak)) int sys_getgid(Thread &thread) { return set_return(thread, -ENOSYS); };
    __attribute__((weak)) int sys_getegid(Thread &thread) { return set_return(thread, -ENOSYS); };
    __attribute__((weak)) int sys_ioctl(Thread &thread) { return set_return(thread, -ENOSYS); };
    __attribute__((weak)) int sys_fcntl(Thread &thread) { return set_return(thread, -ENOSYS); };
} // namespace Hamster

