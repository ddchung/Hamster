
// Hamster system call switch

#include <syscall/syscall.hpp>
#include <abi/syscall_id.hpp>
#include <errno/errno.h>

namespace Hamster
{
    int do_syscall(Thread &thread)
    {
        int syscall_id = thread.get_regs()[17]; // a7 is the syscall ID in RISC-V calling convention

        switch (syscall_id)
        {
            case SyscallID::EXIT:
                return sys_exit(thread);
            case SyscallID::GETPID:
                return sys_getpid(thread);
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
            case SyscallID::LSEEK:
                return sys_lseek(thread);
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
            default:
                set_return(thread, -EINVAL);
                return -1;
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
        return thread.get_regs()[10 + index]; // a0 is at index 10 in the register array
    }

    int transfer_error(Thread &thread)
    {
        // Transfer the error code to the thread's error code
        int error_code = Hamster::error;
        Hamster::error = 0; // Reset the global error code

        return set_return(thread, -error_code);
    }

    int set_return(Thread &thread, uint32_t value)
    {
        // Set the return value in a0 register
        thread.get_regs()[10] = value; // a0 is at index 10 in the register array
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
        if (thread_fd > (int)proc->fds.size())
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
    __attribute__((weak)) int sys_lseek(Thread &thread) { return set_return(thread, -ENOSYS); };
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
} // namespace Hamster

