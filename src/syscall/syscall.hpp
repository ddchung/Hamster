// Hamster system calls

#pragma once

#include <process/task.hpp>
#include <abi/syscall_id.hpp>
#include <cstdint>

namespace Hamster
{
    int do_syscall(Task &task);

    // Transfers the error code from `Hamster::error` to the task
    // Also returns from the system call
    int transfer_error(Task &task);
    int set_return(Task &task, uint32_t value);

    int deref_fd(Task &task, int task_fd);

    int sys_exit(Task &task);
    int sys_getpid(Task &task);
    int sys_getppid(Task &task);
    int sys_clone(Task &task);
    int sys_execve(Task &task);
    int sys_execveat(Task &task);
    int sys_waitid(Task &task);
    int sys_wait4(Task &task);
    int sys_kill(Task &task);
    int sys_openat(Task &task);
    int sys_read(Task &task);
    int sys_write(Task &task);
    int sys_close(Task &task);
    int sys_llseek(Task &task);
    int sys_newfstatat(Task &task);
    int sys_newfstat(Task &task);
    int sys_dup(Task &task);
    int sys_dup3(Task &task);
    int sys_mkdirat(Task &task);
    int sys_unlinkat(Task &task);
    int sys_linkat(Task &task);
    int sys_renameat(Task &task);
    int sys_renameat2(Task &task);
    int sys_getdents64(Task &task);
    int sys_chdir(Task &task);
    int sys_getcwd(Task &task);
    int sys_faccessat(Task &task);
    int sys_pipe2(Task &task);
    int sys_brk(Task &task);
    int sys_mmap2(Task &task);
    int sys_munmap(Task &task);
    int sys_mprotect(Task &task);
    int sys_statx(Task &task);
    int sys_readlinkat(Task &task);
    int sys_symlinkat(Task &task);
    int sys_getuid(Task &task);
    int sys_geteuid(Task &task);
    int sys_getgid(Task &task);
    int sys_getegid(Task &task);
    int sys_ioctl(Task &task);
    int sys_fcntl(Task &task);
} // namespace Hamster

