
#include <syscall/syscall.hpp>
#include <errno/errno.h>

namespace Hamster
{
    int32_t cvt_error(int32_t val)
    {
        int32_t res = val < 0 ? -error : val;
        error = 0;
        return res;
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
                                               uint32_t mode) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_fchmodat2(Task &task, int32_t dirfd, uint32_t pathname_loc,
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
    __attribute__((weak)) int32_t sys_tkill(Task &task, int32_t tid, int32_t sig) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_ppoll_time64(Task &task, uint32_t fds_loc, uint32_t nfds, uint32_t timeout_loc,
                                                   uint32_t sigmask_loc, uint32_t sigset_size) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_socket(Task &task, int32_t domain, int32_t type, int32_t protocol) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_bind(Task &task, int32_t sockfd, uint32_t addr_loc, uint32_t addrlen) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_listen(Task &task, int32_t sockfd, int32_t backlog) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_accept(Task &task, int32_t sockfd, uint32_t addr_loc, uint32_t addrlen_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_accept4(Task &task, int32_t sockfd, uint32_t addr_loc, uint32_t addrlen_loc, int32_t flags) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_connect(Task &task, int32_t sockfd, uint32_t addr_loc, uint32_t addrlen) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_sendto(Task &task, int32_t sockfd, uint32_t buf_loc, uint32_t len, int32_t flags,
                                             uint32_t dest_addr_loc, uint32_t addrlen) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_recvfrom(Task &task, int32_t sockfd, uint32_t buf_loc, uint32_t len, int32_t flags,
                                               uint32_t src_addr_loc, uint32_t addrlen_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_setsockopt(Task &task, int32_t sockfd, int32_t level, int32_t optname, uint32_t optval_loc,
                                                 uint32_t optlen) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_getsockopt(Task &task, int32_t sockfd, int32_t level, int32_t optname, uint32_t optval_loc,
                                                 uint32_t optlen_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_getsockname(Task &task, int32_t sockfd, uint32_t addr_loc, uint32_t addrlen_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_getpeername(Task &task, int32_t sockfd, uint32_t addr_loc, uint32_t addrlen_loc) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_shutdown(Task &task, int32_t sockfd, int32_t how) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_sendmsg(Task &task, int32_t sockfd, uint32_t msg_loc, int32_t flags) { return -H_ENOSYS; }
    __attribute__((weak)) int32_t sys_recvmsg(Task &task, int32_t sockfd, uint32_t msg_loc, int32_t flags) { return -H_ENOSYS; }
} // namespace Hamster
