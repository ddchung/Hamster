
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
    int do_syscall(Task &task)
    {
        int syscall_id = task.emulator.x[17]; // a7 is the syscall ID in RISC-V calling convention
        
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
                return sys_exit(task);
            case SyscallID::GETPID:
                return sys_getpid(task);
            case SyscallID::GETPPID:
                return sys_getppid(task);
            case SyscallID::CLONE:
                return sys_clone(task);
            case SyscallID::EXECVE:
                return sys_execve(task);
            case SyscallID::EXECVEAT:
                return sys_execveat(task);
            case SyscallID::WAITID:
                return sys_waitid(task);
            case SyscallID::WAIT4:
                return sys_wait4(task);
            case SyscallID::KILL:
                return sys_kill(task);
            case SyscallID::OPENAT:
                return sys_openat(task);
            case SyscallID::READ:
                return sys_read(task);
            case SyscallID::WRITE:
                return sys_write(task);
            case SyscallID::CLOSE:
                return sys_close(task);
            case SyscallID::LLSEEK:
                return sys_llseek(task);
            case SyscallID::NEWFSTATAT:
                return sys_newfstatat(task);
            case SyscallID::NEWFSTAT:
                return sys_newfstat(task);
            case SyscallID::DUP:
                return sys_dup(task);
            case SyscallID::DUP3:
                return sys_dup3(task);
            case SyscallID::MKDIRAT:
                return sys_mkdirat(task);
            case SyscallID::UNLINKAT:
                return sys_unlinkat(task);
            case SyscallID::LINKAT:
                return sys_linkat(task);
            case SyscallID::RENAMEAT:
                return sys_renameat(task);
            case SyscallID::RENAMEAT2:
                return sys_renameat2(task);
            case SyscallID::GETDENTS64:
                return sys_getdents64(task);
            case SyscallID::CHDIR:
                return sys_chdir(task);
            case SyscallID::GETCWD:
                return sys_getcwd(task);
            case SyscallID::FACCESSAT:
                return sys_faccessat(task);
            case SyscallID::PIPE2:
                return sys_pipe2(task);
            case SyscallID::BRK:
                return sys_brk(task);
            case SyscallID::MMAP2:
                return sys_mmap2(task);
            case SyscallID::MUNMAP:
                return sys_munmap(task);
            case SyscallID::MPROTECT:
                return sys_mprotect(task);
            case SyscallID::STATX:
                return sys_statx(task);
            case SyscallID::READLINKAT:
                return sys_readlinkat(task);
            case SyscallID::SYMLINKAT:
                return sys_symlinkat(task);
            case SyscallID::GETUID:
                return sys_getuid(task);
            case SyscallID::GETEUID:
                return sys_geteuid(task);
            case SyscallID::GETGID:
                return sys_getgid(task);
            case SyscallID::GETEGID:
                return sys_getegid(task);
            case SyscallID::IOCTL:
                return sys_ioctl(task);
            case SyscallID::FCNTL:
                return sys_fcntl(task);
            default:
                _trace("(Unknown system call) ");
                error = ENOSYS;
                return transfer_error(task);
        }

        // ???
        return -1;
    }

    uint32_t get_arg(Task &task, int index)
    {
        // a7 is used for syscall ID, a0-a6 are used for arguments
        if (index < 0 || index >= 7)
        {
            return 0;
        }

        // trace("[%d = %d] ", index, task.get_regs()[10 + index]);

        return task.emulator.x[10 + index]; // a0 is at index 10 in the register array
    }

    int transfer_error(Task &task)
    {
        // Transfer the error code to the task's error code
        int error_code = Hamster::error;
        Hamster::error = 0; // Reset the global error code

        _trace("%d", -error_code);

        if (error_code >= 0 && (unsigned long)error_code < sizeof(error_names) / sizeof(error_names[0]) && error_names[error_code])
        {
            _trace(" (%s)", error_names[error_code]);
        }
        else
        {
            _trace(" (Unknown error code %d)", error_code);
        }

        _flush_trace();

        // Set the return value to -error_code
        task.emulator.x[10] = -error_code;
        _trace("\n");

        return -1;
    }

    int set_return(Task &task, uint32_t value)
    {
        // Set the return value in a0 register
        task.emulator.x[10] = value; // a0 is at index 10 in the register array
        _trace("%d\n", value);
        _flush_trace();
        return 0; // Success
    }

    __attribute__((weak)) int sys_exit(Task &task) { return set_return(task, -ENOSYS); };
    __attribute__((weak)) int sys_getpid(Task &task) { return set_return(task, -ENOSYS); };
    __attribute__((weak)) int sys_clone(Task &task) { return set_return(task, -ENOSYS); };
    __attribute__((weak)) int sys_execve(Task &task) { return set_return(task, -ENOSYS); };
    __attribute__((weak)) int sys_execveat(Task &task) { return set_return(task, -ENOSYS); };
    __attribute__((weak)) int sys_waitid(Task &task) { return set_return(task, -ENOSYS); };
    __attribute__((weak)) int sys_wait4(Task &task) { return set_return(task, -ENOSYS); };
    __attribute__((weak)) int sys_kill(Task &task) { return set_return(task, -ENOSYS); };
    __attribute__((weak)) int sys_openat(Task &task) { return set_return(task, -ENOSYS); };
    __attribute__((weak)) int sys_read(Task &task) { return set_return(task, -ENOSYS); };
    __attribute__((weak)) int sys_write(Task &task) { return set_return(task, -ENOSYS); };
    __attribute__((weak)) int sys_close(Task &task) { return set_return(task, -ENOSYS); };
    __attribute__((weak)) int sys_llseek(Task &task) { return set_return(task, -ENOSYS); };
    __attribute__((weak)) int sys_newfstatat(Task &task) { return set_return(task, -ENOSYS); };
    __attribute__((weak)) int sys_newfstat(Task &task) { return set_return(task, -ENOSYS); };
    __attribute__((weak)) int sys_dup(Task &task) { return set_return(task, -ENOSYS); };
    __attribute__((weak)) int sys_dup3(Task &task) { return set_return(task, -ENOSYS); };
    __attribute__((weak)) int sys_mkdirat(Task &task) { return set_return(task, -ENOSYS); };
    __attribute__((weak)) int sys_unlinkat(Task &task) { return set_return(task, -ENOSYS); };
    __attribute__((weak)) int sys_linkat(Task &task) { return set_return(task, -ENOSYS); };
    __attribute__((weak)) int sys_renameat(Task &task) { return set_return(task, -ENOSYS); };
    __attribute__((weak)) int sys_renameat2(Task &task) { return set_return(task, -ENOSYS); };
    __attribute__((weak)) int sys_getdents64(Task &task) { return set_return(task, -ENOSYS); };
    __attribute__((weak)) int sys_chdir(Task &task) { return set_return(task, -ENOSYS); };
    __attribute__((weak)) int sys_getcwd(Task &task) { return set_return(task, -ENOSYS); };
    __attribute__((weak)) int sys_faccessat(Task &task) { return set_return(task, -ENOSYS); };
    __attribute__((weak)) int sys_pipe2(Task &task) { return set_return(task, -ENOSYS); };
    __attribute__((weak)) int sys_brk(Task &task) { return set_return(task, -ENOSYS); };
    __attribute__((weak)) int sys_mmap2(Task &task) { return set_return(task, -ENOSYS); };
    __attribute__((weak)) int sys_munmap(Task &task) { return set_return(task, -ENOSYS); };
    __attribute__((weak)) int sys_mprotect(Task &task) { return set_return(task, -ENOSYS); };
    __attribute__((weak)) int sys_statx(Task &task) { return set_return(task, -ENOSYS); };
    __attribute__((weak)) int sys_readlinkat(Task &task) { return set_return(task, -ENOSYS); };
    __attribute__((weak)) int sys_symlinkat(Task &task) { return set_return(task, -ENOSYS); };
    __attribute__((weak)) int sys_getuid(Task &task) { return set_return(task, -ENOSYS); };
    __attribute__((weak)) int sys_geteuid(Task &task) { return set_return(task, -ENOSYS); };
    __attribute__((weak)) int sys_getgid(Task &task) { return set_return(task, -ENOSYS); };
    __attribute__((weak)) int sys_getegid(Task &task) { return set_return(task, -ENOSYS); };
    __attribute__((weak)) int sys_ioctl(Task &task) { return set_return(task, -ENOSYS); };
    __attribute__((weak)) int sys_fcntl(Task &task) { return set_return(task, -ENOSYS); };
} // namespace Hamster

