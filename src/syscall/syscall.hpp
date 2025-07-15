// Hamster system calls

#pragma once

#include <process/thread.hpp>
#include <process/process.hpp>
#include <abi/syscall_id.hpp>
#include <cstdint>

namespace Hamster
{
    int do_syscall(Thread &thread);
    uint32_t get_arg(Thread &thread, int index);

    // Transfers the error code from `Hamster::error` to the thread
    // Also returns from the system call
    int transfer_error(Thread &thread);
    int set_return(Thread &thread, uint32_t value);

    int deref_fd(Thread &thread, int thread_fd);

    int sys_exit(Thread &thread);
    int sys_getpid(Thread &thread);
    int sys_getppid(Thread &thread);
    int sys_clone(Thread &thread);
    int sys_execve(Thread &thread);
    int sys_execveat(Thread &thread);
    int sys_waitid(Thread &thread);
    int sys_wait4(Thread &thread);
    int sys_kill(Thread &thread);
    int sys_openat(Thread &thread);
    int sys_read(Thread &thread);
    int sys_write(Thread &thread);
    int sys_close(Thread &thread);
    int sys_llseek(Thread &thread);
    int sys_newfstatat(Thread &thread);
    int sys_newfstat(Thread &thread);
    int sys_dup(Thread &thread);
    int sys_dup3(Thread &thread);
    int sys_mkdirat(Thread &thread);
    int sys_unlinkat(Thread &thread);
    int sys_linkat(Thread &thread);
    int sys_renameat(Thread &thread);
    int sys_renameat2(Thread &thread);
    int sys_getdents64(Thread &thread);
    int sys_chdir(Thread &thread);
    int sys_getcwd(Thread &thread);
    int sys_faccessat(Thread &thread);
    int sys_pipe2(Thread &thread);
    int sys_brk(Thread &thread);
    int sys_mmap2(Thread &thread);
    int sys_munmap(Thread &thread);
    int sys_mprotect(Thread &thread);
    int sys_statx(Thread &thread);
    int sys_readlinkat(Thread &thread);
    int sys_symlinkat(Thread &thread);
    int sys_getuid(Thread &thread);
    int sys_geteuid(Thread &thread);
    int sys_getgid(Thread &thread);
    int sys_getegid(Thread &thread);
} // namespace Hamster

