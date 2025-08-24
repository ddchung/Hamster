// Hamster system call IDs

#pragma once

#include <cstdint>

namespace Hamster
{
    namespace SyscallID
    {

        // minor note: those postfixed with // after the number (e.g. `EXIT = 93,`) are
        // actually implemented, while those without are not implemented yet and are just stubs
        enum ID : uint16_t
        {
            /**
             * @brief Exit system call
             * This system call terminates the calling process.
             * @note Userspace sig: `void exit(int status);`
             */
            EXIT = 93,//

            /**
             * @brief Get the PID of the calling process
             * This system call retrieves the process ID of the calling process.
             * @note Userspace sig: `pid_t getpid(void);`, `pid_t` = `int`
             */
            GETPID = 172,//

            /**
             * @brief Get the Thread ID of the calling thread
             * This system call retrieves the thread ID of the calling thread.
             * @note Userspace sig: `pid_t gettid(void);`, `pid_t` = `int`
             */
            GETTID = 178,//

            /**
             * @brief Set the PGID of a process
             * This system call sets the process group ID of the specified process.
             * @note Userspace sig: `int setpgid(pid_t pid, pid_t pgid);`
             */
            SETPGID = 154,//

            /**
             * @brief Get the PGID of a process
             * This system call retrieves the process group ID of the specified process.
             * @note Userspace sig: `pid_t getpgid(pid_t pid);`, `pid_t` = `int`
             */
            GETPGID = 155,//

            /**
             * @brief Get the SID of a process
             * This system call retrieves the session ID of the specified process.
             * @note Userspace sig: `pid_t getsid(pid_t pid);`, `pid_t` = `int`
             */
            GETSID = 156,//

            /**
             * @brief Create a new session
             * This system call creates a new session and sets the calling process as the session leader.
             * @note Userspace sig: `pid_t setsid(void);`, `pid_t` = `int`
             */
            SETSID = 157,//

            /**
             * @brief Voluntarily yield the CPU
             * This system call allows the calling process to yield the CPU, allowing other processes to run
             * @note Userspace sig: `void sched_yield(void);`
             */
            SCHED_YIELD = 124,//

            /**
             * @brief Get the PID of the parent process
             * This system call retrieves the process ID of the parent process of the calling process.
             * @note Userspace sig: `pid_t getppid(void);`, `pid_t` = `int`
             */
            GETPPID = 173,//

            /**
             * @brief Clone a task
             * This system call creates either a new process or a new thread, depending on the flags provided.
             * @note Userspace sig: `int clone(unsigned long flags, void *stack, int *ptid, int *ctid, unsigned long newtls);`
             */
            CLONE = 220,//

            /**
             * @brief Replace the current process image with a new one
             * This system call replaces the current process image with a new program.
             * @note Userspace sig: `int execve(const char *filename, char *const argv[], char *const envp[]);`
             */
            EXECVE = 221,//

            /**
             * @brief Replace the current process image with a new one using file descriptor
             * This system call is similar to `EXECVE`, but it allows the path to be relative
             * to a directory file descriptor, instead of the CWD
             * @note Userspace sig: `int execveat(int dirfd, const char *filename, char *const argv[], char *const envp[], int flags);`
             */
            EXECVEAT = 281,//

            /**
             * @brief Wait for a child process to change state
             * This system call suspends the calling process until the specified child processes
             * changes state, such as exiting or stopping.
             * @note Userspace sig: `pid_t waitid(int which, pid_t pid, struct siginfo *infop, int options, struct rusage *ru);`, `pid_t` = `int`
             */
            WAITID = 95,//

            /**
             * @brief Wait for a child process to change state
             * This system call suspends the calling process until the specified child process
             * changes state, such as exiting or stopping.
             * @note Userspace sig: `pid_t wait4(pid_t pid, int *status, int options, struct rusage *ru);`, `pid_t` = `int`
             */
            WAIT4 = 260,

            /**
             * @brief Send a signal to a process
             * This system call sends a signal to a specified process or process group.
             * @note Userspace sig: `int kill(pid_t pid, int sig);`,
             */
            KILL = 129,//

            /**
             * @brief Send a signal to a thread
             * This system call sends a signal to a specified thread.
             * @note Userspace sig: `int tgkill(int tgid, int tid, int sig);`
             */
            TGKILL = 234,//

            /**
             * @brief Get random numbers
             * This system call retrieves random numbers from the kernel's random number generator.
             * Note that for now, in Hamster, it uses the kernel C library's `rand()` function, and is not
             * guaranteed to be cryptographically secure.
             * @note Userspace sig: `int getrandom(void *buf, size_t buflen, unsigned int flags);`, `size_t` = `unsigned long`
             */
            GETRANDOM = 278,//

            /**
             * @brief Set the user ID of the calling process
             * This sets the effective UID, and if the EUID was 0, then all the other UIDs are also set
             * @note Userspace sig: `int setuid(uid_t uid);`, `uid_t` = `unsigned int`
             */
            SETUID = 146,//

            /**
             * @brief Set the real and effective user IDs of the calling process
             * This sets the real UID and effective UID of the calling process.
             * @note Userspace sig: `int setreuid(uid_t ruid, uid_t euid);`, `uid_t` = `unsigned int`
             */
            SETREUID = 145,//

            /**
             * @brief Set the real, effective, and saved set user IDs of the calling process
             * This sets the real, effective, and saved set user IDs of the calling process.
             * Note that on Hamster, the saved set user ID is not implemented yet, so it is ignored
             * @note Userspace sig: `int setresuid(uid_t ruid, uid_t euid, uid_t suid);`, `uid_t` = `unsigned int`
             */
            SETRESUID = 147,//

            /**
             * @brief Set the group ID of the calling process
             * This sets the effective GID, and if the EGID was 0, then all the other GIDs are also set
             * @note Userspace sig: `int setgid(gid_t gid);`, `gid_t` = `unsigned int`
             */
            SETGID = 144,//

            /**
             * @brief Set the real and effective group IDs of the calling process
             * This sets the real GID and effective GID of the calling process.
             * @note Userspace sig: `int setregid(gid_t rgid, gid_t egid);`, `gid_t` = `unsigned int`
             */
            SETREGID = 143,//

            /**
             * @brief Set the real, effective, and saved set group IDs of the calling process
             * This sets the real, effective, and saved set group IDs of the calling process.
             * Note that on Hamster, the saved set group ID is not implemented yet, so it is ignored
             * @note Userspace sig: `int setresgid(gid_t rgid, gid_t egid, gid_t sgid);`, `gid_t` = `unsigned int`
             */
            SETRESGID = 149,//

            /**
             * @brief Open a file
             * This system call opens a file and returns a file descriptor.
             * @note Userspace sig: `int openat(int dfd, const char *pathname, int flags, mode_t mode);`, `mode_t` = `unsigned int`
             * @note The `mode_t` is used only if `flags` includes `OPEN_CREAT`
             */
            OPENAT = 56,//

            /**
             * @brief Read from a file descriptor
             * This system call reads data from a file descriptor into a buffer.
             * @note Userspace sig: `ssize_t read(int fd, void *buf, size_t count);`, `ssize_t` = `long`
             */
            READ = 63,//

            /**
             * @brief Write to a file descriptor
             * This system call writes data from a buffer to a file descriptor.
             * @note Userspace sig: `ssize_t write(int fd, const void *buf, size_t count);`, `ssize_t` = `long`
             */
            WRITE = 64,//

            /**
             * @brief Close a file descriptor
             * This system call closes a file descriptor, releasing any resources associated with it.
             * @note Userspace sig: `int close(int fd);`
             */
            CLOSE = 57,//

            /**
             * @brief Copy data from one file descriptor to another
             * This system call copies data from one file descriptor to another, allowing for efficient data transfer
             * between file descriptors.
             * @note Userspace sig: `ssize_t sendfile64(int out_fd, int in_fd, off64_t *offset, size_t count);`
             */
            SENDFILE64 = 71,

            /**
             * @brief Copy data from one file descriptor to another, where at least one is a pipe
             * This system call copies data from one file descriptor to another, where at least one of the file descriptors is a pipe.
             * @note Userspace sig: `ssize_t splice(int fd_in, loff_t *off_in, int fd_out, loff_t *off_out, size_t len, unsigned int flags);`, `ssize_t` = `long`
             * @note `loff_t` = `long long`
             */
            SPLICE = 275,

            /**
             * @brief Get information about a filesystem
             * This system call retrieves information about a filesystem, such as its type, size, and available space.
             * @note Userspace sig: `int statfs(const char *path, size_t size, struct statfs *buf);`
             */
            STATFS = 43,

            /**
             * @brief Get information about a filesystem from a file descriptor
             * This system call retrieves information about a filesystem from a file descriptor, such as its type, size, and available space.
             * @note Userspace sig: `int fstatfs(int fd, size_t size, struct statfs *buf);`
             */
            FSTATFS = 44,

            /**
             * @brief Mount a filesystem
             * This system call mounts a filesystem at a specified mount point, allowing it to be accessed by the system.
             * @note Userspace sig: `int mount(const char *source, const char *target, const char *filesystemtype, unsigned long mountflags, const void * data);`
             */
            MOUNT = 40,

            /**
             * @brief Unmount a filesystem
             * This system call unmounts a filesystem from a specified mount point, making it inaccessible
             * @note Userspace sig: `int umount2(const char *target, int flags);`
             */
            UMOUNT2 = 39,

            /**
             * @brief Change the ownership of a file specified by path relative to a directory file descriptor
             * This system call changes the ownership of a file at the specified path relative to a directory file descriptor.
             * @note Userspace sig: `int fchownat(int dirfd, const char *pathname, uid_t owner, gid_t group, int flags);`, `uid_t` = `unsigned int`, `gid_t` = `unsigned int`
             */
            FCHOWNAT = 54,//

            /**
             * @brief Change the ownership of a file specified by a file descriptor
             * This system call changes the ownership of a file descriptor.
             * @note Userspace sig: `int fchown(int fd, uid_t owner, gid_t group);`, `uid_t` = `unsigned int`, `gid_t` =
             */
            FCHOWN = 55,//

            /**
             * @brief Change the permissions of a file specified by path relative to a directory file descriptor
             * This system call changes the permissions of a file at the specified path relative to a directory file descriptor.
             * @note Userspace sig: `int fchmodat(int dirfd, const char *pathname, mode_t mode, int flags);`, `mode_t` = `unsigned int`
             */
            FCHMODAT = 53,//

            /**
             * @brief Change the permissions of a file specified by a file descriptor
             * This system call changes the permissions of a file descriptor.
             * @note Userspace sig: `int fchmod(int fd, mode_t mode);`, `mode_t` = `unsigned int`
             */
            FCHMOD = 52,//

            /**
             * @brief Resize a file, specified by file descriptor
             * This system call resizes a file to a specified size, which can be larger or smaller than the current size.
             * @note Userspace sig: `int ftruncate64(int fd, off_t length);`, `off_t` = `long long`
             */
            FTRUNCATE64 = 46,//

            /**
             * @brief Resize a file, specified by path
             * This system call resizes a file at the specified path to a specified size, which can be larger or smaller than the current size.
             * @note Userspace sig: `int truncate64(const char *path, off_t length);`, `off_t` = `long long`
             */
            TRUNCATE64 = 45,//

            /**
             * @brief Change the file offset of a file descriptor
             * This system call changes the file offset of a file descriptor to a specified position.
             * @note Userspace sig: `int llseek(int fd, unsigned long off_high, unsigned long off_low, loff_t *result, int whence);`, `loff_t` = `long long`
             */
            LLSEEK = 62,//

            /**
             * @brief Get file status of a path relative to a directory file descriptor
             * This system call retrieves the status of a file descriptor, such as its mode, size,
             * and timestamps.
             * @note Userspace sig: `int newfstatat(int dirfd, const char *pathname, struct stat *statbuf, int flags);`
             */
            NEWFSTATAT = 79,

            /**
             * @brief Get file status of a path relative to the current working directory
             * This system call retrieves the status of a file, such as its mode, size,
             * and timestamps.
             * @note Userspace sig: `int newfstat(int fd, struct stat *statbuf);`
             */
            NEWFSTAT = 80,//

            /**
             * @brief Duplicate a file descriptor to the lowest available file descriptor
             * This system call creates a copy of a file descriptor, allowing multiple
             * file descriptors to refer to the same open file.
             * @note Userspace sig: `int dup(int oldfd);`
             */
            DUP = 23,//

            /**
             * @brief Duplicate a file descriptor to a specific file descriptor
             * This system call creates a copy of a file descriptor, allowing multiple
             * file descriptors to refer to the same open file, but it allows specifying
             * the target file descriptor. If the target file descriptor is already open,
             * it is closed, with any errors ignored, before the operation
             * @note Userspace sig: `int dup3(int oldfd, int newfd, int flag);`
             */
            DUP3 = 24,//

            /**
             * @brief Create a new directory from a path relative to a directory file descriptor
             * This system call creates a new directory at the specified path relative to a directory file descriptor
             * @note Userspace sig: `int mkdirat(int dirfd, const char *pathname, mode_t mode);`, `mode_t` = `unsigned int`
             */
            MKDIRAT = 34,//

            /**
             * @brief Remove a file or directory from a path relative to a directory file descriptor
             * This system call removes a file or directory at the specified path relative to a directory file descriptor.
             * @note Userspace sig: `int unlinkat(int dirfd, const char *pathname, int flags);`
             */
            UNLINKAT = 35,//

            /**
             * @brief Create a hard link to a file from a path relative to a directory file descriptor
             * This system call creates a hard link to a file at the specified path relative to a directory file descriptor.
             * @note Userspace sig: `int linkat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flags);`
             */
            LINKAT = 37,//

            /**
             * @brief Rename a file or directory from a path relative to a directory file descriptor
             * This system call renames a file or directory at the specified path relative to a directory file descriptor.
             * @note Userspace sig: `int renameat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath);`
             */
            RENAMEAT = 38,//

            /**
             * @brief Rename a file or directory from a path relative to a directory file descriptor with an additional flag
             * This system call renames a file or directory at the specified path relative to a directory file descriptor,
             * allowing for additional flags to control the operation.
             * @note Userspace sig: `int renameat2(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, unsigned int flags);`
             */
            RENAMEAT2 = 276,//

            /**
             * @brief Read the directory entries of a directory file descriptor
             * This system call reads the directory entries of a directory file descriptor and returns them in a
             * buffer.
             * @note Userspace sig: `ssize_t getdents(int fd, struct dirent *dirp, size_t count);`, `ssize_t` = `long`
             */
            GETDENTS64 = 61,//

            /**
             * @brief Change the current working directory
             * This system call changes the current working directory of the calling process to the specified path.
             * @note Userspace sig: `int chdir(const char *path);`
             */
            CHDIR = 49,//

            /**
             * @brief Get the current working directory
             * This system call retrieves the current working directory of the calling process.
             * @note Userspace sig: `char *getcwd(char *buf, size_t size);`, `size_t` = `unsigned long`
             */
            GETCWD = 17,//

            /**
             * @brief Check if a file exists, and potentially if it is read|write|execute accessible
             * @note Userspace sig: `long faccessat(int dirfd, const char *pathname, int mode);`
             */
            FACCESSAT = 48,//

            /**
             * @brief Check if a file exists, and potentially if it is read|write|execute accessible
             * This system call checks if a file exists at the specified path relative to a directory file descriptor,
             * and potentially checks if it is readable, writable, or executable.
             * @note Userspace sig: `long faccessat2(int dirfd, const char *pathname, int mode, int flags);`
             */
            FACCESSAT2 = 439,//

            /**
             * @brief Create an unnamed pipe
             * This system call creates a pipe, which is a unidirectional data channel that can
             * be used for inter-process communication.
             * @note Userspace sig: `int pipe(int pipefd[2], int flags);`
             */
            PIPE2 = 59,//

            /**
             * @brief Set the program data's end (the break)
             * @note Userspace sig: `void *brk(void *end_data_segment);`
             */
            BRK = 214,//

            /**
             * @brief Memory map a file or device into memory
             * This system call maps a file or device into memory, allowing it to be accessed as if it were part of the process's address space.
             * @note Userspace sig: `void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);`
             */
            MMAP2 = 222,//

            /**
             * @brief Remap a memory region
             * This system call remaps a memory region, allowing it to be moved or resized.
             * @note Userspace sig: `void *mremap(unsigned long addr, unsigned long old_len, unsigned long new_len, unsigned long flags, unsigned long new_addr);`
             */
            MREMAP = 216,

            /**
             * @brief Unmap a memory region
             * This system call unmaps a previously mapped memory region, releasing the resources associated with it.
             * @note Userspace sig: `int munmap(void *addr, size_t length);`
             */
            MUNMAP = 215,//

            /**
             * @brief Change the protection of a memory region
             * This system call changes the memory protection of a specified memory region, allowing or disallowing access to it.
             * @note Userspace sig: `int mprotect(void *addr, size_t len, int prot);`
             */
            MPROTECT = 226,//

            /**
             * @brief Extended stat
             * This system call retrieves extended file status information, such as attributes and timestamps,
             * for a file or directory.
             * @note Userspace sig: `int statx(int dirfd, const char *pathname, int flags, unsigned int mask, struct statx *statxbuf);`
             */
            STATX = 291,//

            /**
             * @brief Read a symbolic link
             * This system call reads the target of a symbolic link and returns it in a buffer.
             * @note Userspace sig: `ssize_t readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz);`, `ssize_t` = `long`
             */
            READLINKAT = 78,//

            /**
             * @brief Create a symbolic link
             * This system call creates a symbolic link at the specified path relative to a directory file descriptor
             * @note Userspace sig: `int symlinkat(const char *target, int newdirfd, const char *linkpath);`
             */
            SYMLINKAT = 36,//

            /**
             * @brief Get the user ID of the calling process
             * @note Userspace sig: `uid_t getuid(void);`, `uid_t` = `unsigned int`
             */
            GETUID = 174,//

            /**
             * @brief Get effective user ID of the calling process
             * @note Userspace sig: `uid_t geteuid(void);`, `uid_t` = `unsigned int`
             */
            GETEUID = 175,//

            /**
             * @brief Get the real, effective, and saved set user IDs of the calling process
             * This system call retrieves the real, effective, and saved set user IDs of the calling process.
             * @note Userspace sig: `int getresuid(uid_t *ruid, uid_t *euid, uid_t *suid);`, `uid_t`
             */
            GETRESUID = 148,//

            /**
             * @brief Get the group ID of the calling process
             * @note Userspace sig: `gid_t getgid(void);`, `gid_t` = `unsigned int`
             */
            GETGID = 176,//

            /**
             * @brief Get effective group ID of the calling process
             * @note Userspace sig: `gid_t getegid(void);`, `gid_t` = `unsigned int`
             */
            GETEGID = 177,//

            /**
             * @brief Get the real, effective, and saved set group IDs of the calling process
             * This system call retrieves the real, effective, and saved set group IDs of the calling process.
             * @note Userspace sig: `int getresgid(gid_t *rgid, gid_t *egid, gid_t *sgid);`, `gid_t` = `unsigned int`
             */
            GETRESGID = 150,//

            /**
             * @brief Get the supplementary group IDs of the calling process
             * This system call retrieves the supplementary group IDs of the calling process.
             * @note Userspace sig: `int getgroups(int size, gid_t *list);`, `gid_t` = `unsigned int`
             */
            GETGROUPS = 158,//

            /**
             * @brief Set the supplementary group IDs of the calling process
             * This system call sets the supplementary group IDs of the calling process.
             * @note Userspace sig: `int setgroups(int size, const gid_t *list);`, `gid_t` = `unsigned int`
             */
            SETGROUPS = 159,//

            /**
             * ioctl.
             * @note Userspace sig: `int ioctl(int fd, int request, ...);`
             */
            IOCTL = 29,//

            /**
             * @brief Perform an operation on a file descriptor
             * This system call performs an operation on a file descriptor, such as changing its flags or
             * retrieving its status.
             * @note Userspace sig: `int fcntl64(int fd, int cmd, ...);`
             */
            FCNTL64 = 25,//

            /**
             * @brief Manipulate behavior of the calling process
             * This system call manipulates the behavior of the calling process, such as setting its scheduling
             * policy or priority.
             * @note Userspace sig: `int prctl(int option, unsigned long arg2, unsigned long arg3, unsigned long arg4, unsigned long arg5);`
             */
            PRCTL = 167,//

            /**
             * @brief End all threads in the calling process
             * @note Userspace sig: `void exit_group(int status);`
             */
            EXIT_GROUP = 94,//

            /**
             * @brief Set a signal handler for a specific signal
             * @note Userspace sig: `int rt_sigaction(int signum, const struct sigaction *act, struct sigaction *oldact, size_t sigsetsize);`
             */
            RT_SIGACTION = 134,//

            /**
             * @brief Get the signals that are pending (waiting to be delivered, but blocked) for the calling process
             * @note Userspace sig: `int rt_sigpending(sigset_t *set, size_t sigsetsize);`
             */
            RT_SIGPENDING = 136,

            /**
             * @brief Get and/or set the signal mask of the calling process
             * @note Userspace sig: `int rt_sigprocmask(int how, const sigset_t *set, sigset_t *oldset, size_t sigsetsize);`
             */
            RT_SIGPROCMASK = 135,//

            /**
             * @brief Send a signal to a process
             * @note Userspace sig: `int rt_sigqueueinfo(pid_t tgid, int sig, const siginfo_t *info);`
             */
            RT_SIGQUEUEINFO = 138,//

            /**
             * @brief Return from a signal handler
             * @note Userspace sig: `int rt_sigreturn(void);`
             */
            RT_SIGRETURN = 139,//

            /**
             * @brief Temporarily change the signal mask, and wait for a signal to be delivered that either calls a signal handler or terminates the process
             * @note Userspace sig: `int rt_sigsuspend(const sigset_t *newset, size_t sigsetsize);`
             */
            RT_SIGSUSPEND = 133,

            /**
             * @brief Wait for one of the specified signals to be pending
             * @note Userspace sig: `int rt_sigtimedwait_time64(const sigset_t *set, siginfo_t *info, const struct timespec *timeout, size_t sigsetsize);`
             */
            RT_SIGTIMEDWAIT_TIME64 = 421,

            /**
             * @brief Send a signal to a thread
             * @note Userspace sig: `int rt_tgsigqueueinfo(pid_t tgid, pid_t tid, int sig, siginfo_t *info);`
             */
            RT_TGSIGQUEUEINFO = 240,//

            /**
             * @brief Get info about the system
             * @note Userspace sig: `int uname(struct utsname *buf);`
             */
            UNAME = 160,//

            /**
             * @brief Wait for some file descriptors to become ready for a certain operation
             * @note Userspace sig: `int pselect6_time64(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, const struct timespec *timeout, const sigset_t *sigmask);`
             */
            PSELECT6_TIME64 = 413,//

            /**
             * @brief Sync a file and its data
             * @note Userspace sig: `int fsync(int fd);`
             */
            FSYNC = 82,

            /**
             * @brief Sync a file's data, and strictly needed metadata
             * @note Userspace sig: `int fdatasync(int fd);`
             */
            FDATASYNC = 83,

            /**
             * @brief Get the clock resolution of a specific clock
             * Right now, all clocks have 1 millisecond resolution.
             * @note Userspace sig: `int clock_getres_time64(clockid_t clock_id, struct timespec *res);`
             */
            CLOCK_GETRES_TIME64 = 406,

            /**
             * @brief Get the current time of a specific clock
             * @note Userspace sig: `int clock_gettime64(clockid_t clock_id, struct timespec *tp);`
             */
            CLOCK_GETTIME64 = 403,

            /**
             * @brief Sleep for a certain time
             * @note Userspace sig: `int clock_nanosleep_time64(clockid_t clock_id, const struct timespec *req, struct timespec *rem);`
             */
            CLOCK_NANOSLEEP_TIME64 = 407,

            /**
             * @brief Set the time of a specific clock
             * @note Userspace sig: `int clock_settime64(clockid_t clock_id, const struct timespec *tp);`
             */
            CLOCK_SETTIME64 = 404,
        };
    } // namespace SyscallID
} // namespace Hamster
