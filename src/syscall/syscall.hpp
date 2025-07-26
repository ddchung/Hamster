// Hamster system calls

#pragma once

#include <abi/syscall_id.hpp>
#include <cstdint>

namespace Hamster
{
    // Note: All of these return `int32_t`, but
    // it may actually be signed or unsigned depending on the syscall.
    // This does not affect anything, as it is userspace
    // that interprets it.
    //
    // Also, all of these, on error, return a negative errno value

    int32_t sys_exit(int32_t status);
    int32_t sys_getpid();
    int32_t sys_gettid();
    int32_t sys_setpgid(int32_t pid, int32_t pgid);
    int32_t sys_getpgid(int32_t pid);
    int32_t sys_getsid(int32_t pid);
    int32_t sys_setsid();
    int32_t sys_sched_yield();
    int32_t sys_getppid();
    int32_t sys_clone(uint32_t flags, uint32_t stack_loc, uint32_t ptid_loc,
                      uint32_t ctid_loc, uint32_t newtls);
    int32_t sys_execve(uint32_t filename_loc, uint32_t argv_loc,
                       uint32_t envp_loc);
    int32_t sys_execveat(int32_t dirfd, uint32_t filename_loc,
                         uint32_t argv_loc, uint32_t envp_loc, int32_t flags);
    int32_t sys_waitid(int32_t which, int32_t pid, uint32_t infop_loc,
                       int32_t options, uint32_t ru_loc);
    int32_t sys_wait4(int32_t pid, uint32_t status_loc, int32_t options,
                      uint32_t ru_loc);
    int32_t sys_kill(int32_t pid, int32_t sig);
    int32_t sys_tgkill(int32_t tgid, int32_t tid, int32_t sig);
    int32_t sys_getrandom(uint32_t buf_loc, uint32_t buflen,
                          uint32_t flags);
    int32_t sys_setuid(uint32_t uid);
    int32_t sys_setreuid(uint32_t ruid, uint32_t euid);
    int32_t sys_setresuid(uint32_t ruid, uint32_t euid, uint32_t suid);
    int32_t sys_setgid(uint32_t gid);
    int32_t sys_setregid(uint32_t rgid, uint32_t egid);
    int32_t sys_setresgid(uint32_t rgid, uint32_t egid, uint32_t sgid);
    int32_t sys_openat(int32_t dfd, uint32_t pathname_loc, int32_t flags,
                       uint32_t mode);
    int32_t sys_read(int32_t fd, uint32_t buf_loc, uint32_t count);
    int32_t sys_write(int32_t fd, uint32_t buf_loc, uint32_t count);
    int32_t sys_close(int32_t fd);
    int32_t sys_sendfile64(int32_t out_fd, int32_t in_fd,
                          uint32_t offset_loc,
                          uint32_t count);
    int32_t sys_splice(int32_t fd_in, uint32_t off_in_loc,
                       int32_t fd_out, uint32_t off_out_loc,
                       uint32_t len, uint32_t flags);
    int32_t sys_statfs(uint32_t path_loc, uint32_t size, uint32_t buf_loc);
    int32_t sys_fstatfs(int32_t fd, uint32_t size, uint32_t buf_loc);
    int32_t sys_mount(uint32_t source_loc, uint32_t target_loc,
                      uint32_t filesystemtype_loc, uint32_t mountflags,
                      uint32_t data_loc);
    int32_t sys_umount2(uint32_t target_loc, int32_t flags);
    int32_t sys_fchownat(int32_t dirfd, uint32_t pathname_loc,
                         uint32_t owner, uint32_t group, int32_t flags);
    int32_t sys_fchown(int32_t fd, uint32_t owner, uint32_t group);
    int32_t sys_fchmodat(int32_t dirfd, uint32_t pathname_loc,
                         uint32_t mode);
    int32_t sys_fchmod(int32_t fd, uint32_t mode);
    int32_t sys_ftruncate64(int32_t fd, uint32_t off_high, uint32_t off_low);
    int32_t sys_truncate64(uint32_t path_loc, uint32_t off_high,
                           uint32_t off_low);
    int32_t sys_llseek(int32_t fd, uint32_t off_high, uint32_t off_low,
                       uint32_t result_loc, int32_t whence);
    int32_t sys_newfstatat(int32_t dirfd, uint32_t pathname_loc,
                           uint32_t statbuf_loc, int32_t flags);
    int32_t sys_newfstat(int32_t fd, uint32_t statbuf_loc);
    int32_t sys_dup(int32_t oldfd);
    int32_t sys_dup3(int32_t oldfd, int32_t newfd, int32_t flags);
    int32_t sys_mkdirat(int32_t dirfd, uint32_t pathname_loc,
                        uint32_t mode);
    int32_t sys_unlinkat(int32_t dirfd, uint32_t pathname_loc,
                         int32_t flags);
    int32_t sys_linkat(int32_t olddirfd, uint32_t oldpathname_loc,
                       int32_t newdirfd, uint32_t newpathname_loc,
                       int32_t flags);
    int32_t sys_renameat(int32_t olddirfd, uint32_t oldpathname_loc,
                         int32_t newdirfd, uint32_t newpathname_loc);
    int32_t sys_renameat2(int32_t olddirfd, uint32_t oldpathname_loc,
                          int32_t newdirfd, uint32_t newpathname_loc,
                          uint32_t flags);
    int32_t sys_getdents64(int32_t fd, uint32_t dirp_loc,
                           uint32_t count);
    int32_t sys_chdir(uint32_t path_loc);
    int32_t sys_getcwd(uint32_t buf_loc, uint32_t size);
    int32_t sys_faccessat(int32_t dirfd, uint32_t pathname_loc,
                          int32_t mode);
    int32_t sys_faccessat2(int32_t dirfd, uint32_t pathname_loc,
                           int32_t mode, int32_t flags);
    int32_t sys_pipe2(uint32_t pipefd_loc, int32_t flags);
    int32_t sys_brk(uint32_t end_data_segment_loc);
    int32_t sys_mmap2(uint32_t addr, uint32_t length,
                          uint32_t prot, uint32_t flags,
                          int32_t fd, uint32_t offset);
    int32_t sys_mremap(uint32_t old_address, uint32_t old_size,
                          uint32_t new_size, uint32_t flags,
                          uint32_t new_address);
    int32_t sys_munmap(uint32_t addr, uint32_t length);
    int32_t sys_mprotect(uint32_t addr, uint32_t len, int32_t prot);
    int32_t sys_statx(int32_t dirfd, uint32_t pathname_loc,
                      int32_t flags, uint32_t mask,
                      uint32_t statxbuf_loc);
    int32_t sys_readlinkat(int32_t dirfd, uint32_t pathname_loc,
                           uint32_t buf_loc, uint32_t bufsiz);
    int32_t sys_symlinkat(uint32_t target_loc, int32_t newdirfd,
                          uint32_t linkpath_loc);
    int32_t sys_getuid();
    int32_t sys_geteuid();
    int32_t sys_getresuid(uint32_t ruid_loc,
                          uint32_t euid_loc, uint32_t suid_loc);
    int32_t sys_getgid();
    int32_t sys_getegid();
    int32_t sys_getresgid(uint32_t rgid_loc,
                          uint32_t egid_loc, uint32_t sgid_loc);
    int32_t sys_ioctl(int32_t fd, int32_t request, uint32_t arg);
    int32_t sys_fcntl64(int32_t fd, int32_t cmd,
                      uint32_t arg);
    
    /**
     * @brief Call a system call
     * @param sys_id The system call ID to call, one of `Hamster::SyscallID::*`
     * @return Whatever the sys_* function returns, or -ENOSYS if the ID is not recognized 
     */
    int32_t syscall(int32_t sys_id);
}
