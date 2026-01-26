// Hamster system calls

#pragma once

#include <abi/syscall_id.hpp>
#include <process/task.hpp>
#include <errno/errno.h>
#include <cstdint>

namespace Hamster
{
    // Note: All of these return `int32_t`, but
    // it may actually be signed or unsigned depending on the syscall.
    // This does not affect anything, as it is userspace
    // that interprets it.
    //
    // Also, all of these, on error, return a negative errno value
    //
    // Additional Note: You may notice that the passing of 64-bit values
    // is inconsistent. See syscall(2) for more information (hint: historical baggage)

    int32_t sys_exit(Task &task, int32_t status);
    int32_t sys_getpid(Task &task);
    int32_t sys_gettid(Task &task);
    int32_t sys_setpgid(Task &task, int32_t pid, int32_t pgid);
    int32_t sys_getpgid(Task &task, int32_t pid);
    int32_t sys_getsid(Task &task, int32_t pid);
    int32_t sys_setsid(Task &task);
    int32_t sys_sched_yield(Task &task);
    int32_t sys_getppid(Task &task);
    int32_t sys_clone(Task &task, uint32_t flags, uint32_t stack_loc, uint32_t ptid_loc,
                      uint32_t tls, uint32_t ctid_loc);
    int32_t sys_execve(Task &task, uint32_t filename_loc, uint32_t argv_loc,
                       uint32_t envp_loc);
    int32_t sys_execveat(Task &task, int32_t dirfd, uint32_t filename_loc,
                         uint32_t argv_loc, uint32_t envp_loc, int32_t flags);
    int32_t sys_waitid(Task &task, int32_t which, int32_t pid, uint32_t infop_loc,
                       int32_t options, uint32_t ru_loc);
    int32_t sys_wait4(Task &task, int32_t pid, uint32_t status_loc, int32_t options,
                      uint32_t ru_loc);
    int32_t sys_kill(Task &task, int32_t pid, int32_t sig);
    int32_t sys_tgkill(Task &task, int32_t tgid, int32_t tid, int32_t sig);
    int32_t sys_getrandom(Task &task, uint32_t buf_loc, uint32_t buflen,
                          uint32_t flags);
    int32_t sys_setuid(Task &task, uint32_t uid);
    int32_t sys_setreuid(Task &task, uint32_t ruid, uint32_t euid);
    int32_t sys_setresuid(Task &task, uint32_t ruid, uint32_t euid, uint32_t suid);
    int32_t sys_setgid(Task &task, uint32_t gid);
    int32_t sys_setregid(Task &task, uint32_t rgid, uint32_t egid);
    int32_t sys_setresgid(Task &task, uint32_t rgid, uint32_t egid, uint32_t sgid);
    int32_t sys_openat(Task &task, int32_t dfd, uint32_t pathname_loc, int32_t flags,
                       uint32_t mode);
    int32_t sys_read(Task &task, int32_t fd, uint32_t buf_loc, uint32_t count);
    int32_t sys_write(Task &task, int32_t fd, uint32_t buf_loc, uint32_t count);
    int32_t sys_close(Task &task, int32_t fd);
    int32_t sys_sendfile64(Task &task, int32_t out_fd, int32_t in_fd,
                           uint32_t offset_loc,
                           uint32_t count);
    int32_t sys_splice(Task &task, int32_t fd_in, uint32_t off_in_loc,
                       int32_t fd_out, uint32_t off_out_loc,
                       uint32_t len, uint32_t flags);
    int32_t sys_statfs(Task &task, uint32_t path_loc, uint32_t size, uint32_t buf_loc);
    int32_t sys_fstatfs(Task &task, int32_t fd, uint32_t size, uint32_t buf_loc);
    int32_t sys_mount(Task &task, uint32_t source_loc, uint32_t target_loc,
                      uint32_t filesystemtype_loc, uint32_t mountflags,
                      uint32_t data_loc);
    int32_t sys_umount2(Task &task, uint32_t target_loc, int32_t flags);
    int32_t sys_fchownat(Task &task, int32_t dirfd, uint32_t pathname_loc,
                         uint32_t owner, uint32_t group, int32_t flags);
    int32_t sys_fchown(Task &task, int32_t fd, uint32_t owner, uint32_t group);
    int32_t sys_fchmodat(Task &task, int32_t dirfd, uint32_t pathname_loc,
                         uint32_t mode);
    int32_t sys_fchmodat2(Task &task, int32_t dirfd, uint32_t pathname_loc,
                         uint32_t mode, int32_t flags);
    int32_t sys_fchmod(Task &task, int32_t fd, uint32_t mode);
    int32_t sys_ftruncate64(Task &task, int32_t fd, uint32_t off_high, uint32_t off_low);
    int32_t sys_truncate64(Task &task, uint32_t path_loc, uint32_t off_high,
                           uint32_t off_low);
    int32_t sys_llseek(Task &task, int32_t fd, uint32_t off_high, uint32_t off_low,
                       uint32_t result_loc, int32_t whence);
    int32_t sys_newfstatat(Task &task, int32_t dirfd, uint32_t pathname_loc,
                           uint32_t statbuf_loc, int32_t flags);
    int32_t sys_newfstat(Task &task, int32_t fd, uint32_t statbuf_loc);
    int32_t sys_dup(Task &task, int32_t oldfd);
    int32_t sys_dup3(Task &task, int32_t oldfd, int32_t newfd, int32_t flags);
    int32_t sys_mkdirat(Task &task, int32_t dirfd, uint32_t pathname_loc,
                        uint32_t mode);
    int32_t sys_unlinkat(Task &task, int32_t dirfd, uint32_t pathname_loc,
                         int32_t flags);
    int32_t sys_linkat(Task &task, int32_t olddirfd, uint32_t oldpathname_loc,
                       int32_t newdirfd, uint32_t newpathname_loc,
                       int32_t flags);
    int32_t sys_renameat(Task &task, int32_t olddirfd, uint32_t oldpathname_loc,
                         int32_t newdirfd, uint32_t newpathname_loc);
    int32_t sys_renameat2(Task &task, int32_t olddirfd, uint32_t oldpathname_loc,
                          int32_t newdirfd, uint32_t newpathname_loc,
                          uint32_t flags);
    int32_t sys_getdents64(Task &task, int32_t fd, uint32_t dirp_loc,
                           uint32_t count);
    int32_t sys_chdir(Task &task, uint32_t path_loc);
    int32_t sys_getcwd(Task &task, uint32_t buf_loc, uint32_t size);
    int32_t sys_faccessat(Task &task, int32_t dirfd, uint32_t pathname_loc,
                          int32_t mode);
    int32_t sys_faccessat2(Task &task, int32_t dirfd, uint32_t pathname_loc,
                           int32_t mode, int32_t flags);
    int32_t sys_pipe2(Task &task, uint32_t pipefd_loc, int32_t flags);
    int32_t sys_brk(Task &task, uint32_t end_data_segment_loc);
    int32_t sys_mmap2(Task &task, uint32_t addr, uint32_t length,
                      uint32_t prot, uint32_t flags,
                      int32_t fd, uint32_t offset);
    int32_t sys_mremap(Task &task, uint32_t old_address, uint32_t old_size,
                       uint32_t new_size, uint32_t flags,
                       uint32_t new_address);
    int32_t sys_munmap(Task &task, uint32_t addr, uint32_t length);
    int32_t sys_mprotect(Task &task, uint32_t addr, uint32_t len, int32_t prot);
    int32_t sys_statx(Task &task, int32_t dirfd, uint32_t pathname_loc,
                      int32_t flags, uint32_t mask,
                      uint32_t statxbuf_loc);
    int32_t sys_readlinkat(Task &task, int32_t dirfd, uint32_t pathname_loc,
                           uint32_t buf_loc, uint32_t bufsiz);
    int32_t sys_symlinkat(Task &task, uint32_t target_loc, int32_t newdirfd,
                          uint32_t linkpath_loc);
    int32_t sys_getuid(Task &task);
    int32_t sys_geteuid(Task &task);
    int32_t sys_getresuid(Task &task, uint32_t ruid_loc,
                          uint32_t euid_loc, uint32_t suid_loc);
    int32_t sys_getgid(Task &task);
    int32_t sys_getegid(Task &task);
    int32_t sys_getresgid(Task &task, uint32_t rgid_loc,
                          uint32_t egid_loc, uint32_t sgid_loc);
    int32_t sys_getgroups(Task &task, uint32_t size, uint32_t list_loc);
    int32_t sys_setgroups(Task &task, uint32_t size, uint32_t list_loc);
    int32_t sys_ioctl(Task &task, int32_t fd, int32_t request, uint32_t arg);
    int32_t sys_fcntl64(Task &task, int32_t fd, int32_t cmd,
                        uint32_t arg);
    int32_t sys_prctl(Task &task, int32_t option, uint32_t arg2,
                      uint32_t arg3, uint32_t arg4,
                      uint32_t arg5);
    int32_t sys_exit_group(Task &task, int32_t status);
    int32_t sys_rt_sigaction(Task &task, int32_t signum, uint32_t act_loc,
                             uint32_t oldact_loc, uint32_t sigsetsize);
    int32_t sys_rt_sigpending(Task &task, uint32_t set_loc, uint32_t sigsetsize);
    int32_t sys_rt_sigprocmask(Task &task, int32_t how, uint32_t set_loc,
                               uint32_t oldset_loc, uint32_t sigsetsize);
    int32_t sys_rt_sigqueueinfo(Task &task, int32_t tgid, int32_t sig,
                                uint32_t info_loc);
    int32_t sys_rt_sigreturn(Task &task);
    int32_t sys_rt_sigsuspend(Task &task, uint32_t unewset_loc, uint32_t sigsetsize);
    int32_t sys_rt_sigtimedwait_time64(Task &task, int32_t sigset_loc,
                                       uint32_t info_loc, uint32_t timeout_loc,
                                       uint32_t sigsetsize);
    int32_t sys_rt_tgsigqueueinfo(Task &task, int32_t tgid, int32_t tid, int32_t sig,
                                  uint32_t info_loc);
    int32_t sys_uname(Task &task, uint32_t buf_loc);
    int32_t sys_pselect6_time64(Task &task, int32_t nfds, uint32_t readfds_loc,
                                uint32_t writefds_loc, uint32_t exceptfds_loc,
                                uint32_t timeout_loc, uint32_t sigmask_loc);
    int32_t sys_fsync(Task &task, int32_t fd);
    int32_t sys_fdatasync(Task &task, int32_t fd);
    int32_t sys_clock_getres_time64(Task &task, int32_t clock_id, uint32_t res_loc);
    int32_t sys_clock_gettime64(Task &task, int32_t clock_id, uint32_t tp_loc);
    int32_t sys_clock_nanosleep_time64(Task &task, int32_t clock_id, int32_t flags, uint32_t req_loc, uint32_t rem_loc);
    int32_t sys_clock_settime64(Task &task, int32_t clock_id, uint32_t tp_loc);
    int32_t sys_chroot(Task &task, uint32_t path_loc);
    int32_t sys_close_range(Task &task, uint32_t first, uint32_t last, uint32_t flags);
    int32_t sys_copy_file_range(Task &task, int32_t fd_in, uint32_t off_in_loc,
                                int32_t fd_out, uint32_t off_out_loc,
                                uint32_t len, uint32_t flags);
    int32_t sys_get_robust_list(Task &task, int32_t pid, uint32_t head_ptr_loc, uint32_t len_loc);
    int32_t sys_pread64(Task &task, int32_t fd, uint32_t buf_loc, uint32_t count, uint32_t _pad, uint32_t off_low,
                        uint32_t off_high);
    int32_t sys_preadv(Task &task, int32_t fd, uint32_t vec_loc, uint32_t vlen, uint32_t pos_low, uint32_t pos_high);
    int32_t sys_preadv2(Task &task, int32_t fd, uint32_t vec_loc, uint32_t vlen, uint32_t pos_low, uint32_t pos_high,
                        uint32_t flags);
    int32_t sys_pwrite64(Task &task, int32_t fd, uint32_t buf_loc, uint32_t count, uint32_t _pad, uint32_t off_low,
                         uint32_t off_high);
    int32_t sys_pwritev(Task &task, int32_t fd, uint32_t vec_loc, uint32_t vlen, uint32_t pos_low, uint32_t pos_high);
    int32_t sys_pwritev2(Task &task, int32_t fd, uint32_t vec_loc, uint32_t vlen, uint32_t pos_low, uint32_t pos_high,
                         uint32_t flags);
    int32_t sys_riscv_flush_icache(Task &task, uint32_t start_loc, uint32_t end_loc, uint32_t flags);
    int32_t sys_set_robust_list(Task &task, uint32_t head_loc, uint32_t len);
    int32_t sys_set_tid_address(Task &task, uint32_t tidptr_loc);
    int32_t sys_sigaltstack(Task &task, uint32_t ss_loc, uint32_t old_ss_loc);
    int32_t sys_futex_time64(Task &task, uint32_t uaddr_loc, int32_t futex_op, uint32_t val,
                             uint32_t timeout_loc, uint32_t uaddr2_loc, uint32_t val3);
    int32_t sys_readv(Task &task, int32_t fd, uint32_t vec_loc, uint32_t vlen);
    int32_t sys_writev(Task &task, int32_t fd, uint32_t vec_loc, uint32_t vlen);
    int32_t sys_tkill(Task &task, int32_t tid, int32_t sig);
    int32_t sys_ppoll_time64(Task &task, uint32_t fds_loc, uint32_t nfds, uint32_t timeout_loc,
                             uint32_t sigmask_loc, uint32_t sigset_size);

    /**
     * @brief Call a system call
     * @param task The task
     * @param sys_id The system call ID to call, one of `Hamster::SyscallID::*`
     * @return Whatever the sys_* function returns, or -H_ENOSYS if the ID is not recognized
     */
    int32_t syscall(Task &task, int32_t sys_id);

    /**
     * @brief Convert and clear the global `error` variable in the kernel
     * @param val
     * @return `val < 0 ? -error : val`. Also clears `error`
     */
    int32_t cvt_error(int32_t val = -1);

    /**
     * @brief Call a system call directly, but still automatically getting arguments
     * @param task The task
     * @param sys_fn The system call function to call, one of `Hamster::sys_*`
     * @return Whatever the system call returns
     * @note This will automatically get the arguments from the current task's registers
     */
    template <typename... SysArgs>
    int32_t syscall(Task &task, int32_t (*sys_fn)(Task &, SysArgs...))
    {
        static constexpr size_t num_args = sizeof...(SysArgs);
        static_assert(num_args <= 6, "syscall must have 6 or fewer arguments");

        // Enumerator

        if constexpr (num_args == 0)
            return sys_fn(task);
        else if constexpr (num_args == 1)
            return sys_fn(task, task.get_emulator().x[10]);
        else if constexpr (num_args == 2)
            return sys_fn(task, task.get_emulator().x[10], task.get_emulator().x[11]);
        else if constexpr (num_args == 3)
            return sys_fn(task, task.get_emulator().x[10], task.get_emulator().x[11], task.get_emulator().x[12]);
        else if constexpr (num_args == 4)
            return sys_fn(task, task.get_emulator().x[10], task.get_emulator().x[11], task.get_emulator().x[12], task.get_emulator().x[13]);
        else if constexpr (num_args == 5)
            return sys_fn(task, task.get_emulator().x[10], task.get_emulator().x[11], task.get_emulator().x[12], task.get_emulator().x[13], task.get_emulator().x[14]);
        else if constexpr (num_args == 6)
            return sys_fn(task, task.get_emulator().x[10], task.get_emulator().x[11], task.get_emulator().x[12], task.get_emulator().x[13], task.get_emulator().x[14], task.get_emulator().x[15]);
    }
}
