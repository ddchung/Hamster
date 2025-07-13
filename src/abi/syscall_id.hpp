// Hamster system call IDs

#pragma once

#include <cstdint>

namespace Hamster
{
    namespace SyscallID
    {

        // minor note: those postfixed with // after the number (e.g. `EXIT = 93,//`) are
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
            WAIT4 = 260,//

            /**
             * @brief Send a signal to a process
             * This system call sends a signal to a specified process or process group.
             * @note Userspace sig: `int kill(pid_t pid, int sig);`,
             */
            KILL = 129,

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
            NEWFSTATAT = 79,//

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
            LINKAT = 37,

            /**
             * @brief Rename a file or directory from a path relative to a directory file descriptor
             * This system call renames a file or directory at the specified path relative to a directory file descriptor.
             * @note Userspace sig: `int renameat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath);`
             */
            RENAMEAT = 38,

            /**
             * @brief Rename a file or directory from a path relative to a directory file descriptor with an additional flag
             * This system call renames a file or directory at the specified path relative to a directory file descriptor,
             * allowing for additional flags to control the operation.
             * @note Userspace sig: `int renameat2(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, unsigned int flags);`
             */
            RENAMEAT2 = 276,

            /**
             * @brief Read the directory entries of a directory file descriptor
             * This system call reads the directory entries of a directory file descriptor and returns them in a
             * buffer.
             * @note Userspace sig: `ssize_t getdents(int fd, struct dirent *dirp, size_t count);`, `ssize_t` = `long`
             */
            GETDENTS64 = 61,

            /**
             * @brief Change the current working directory
             * This system call changes the current working directory of the calling process to the specified path.
             * @note Userspace sig: `int chdir(const char *path);`
             */
            CHDIR = 49,

            /**
             * @brief Get the current working directory
             * This system call retrieves the current working directory of the calling process.
             * @note Userspace sig: `char *getcwd(char *buf, size_t size);`, `size_t` = `unsigned long`
             */
            GETCWD = 17,

            /**
             * @brief Check if a file exists, and potentially if it is read|write|execute accessible
             * @note Userspace sig: `long faccessat(int dirfd, const char *pathname, int mode);`
             */
            FACCESSAT = 48,

            /**
             * @brief Create an unnamed pipe
             * This system call creates a pipe, which is a unidirectional data channel that can
             * be used for inter-process communication.
             * @note Userspace sig: `int pipe(int pipefd[2]);`
             */
            PIPE2 = 59,

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
            MMAP2 = 222,

            /**
             * @brief Unmap a memory region
             * This system call unmaps a previously mapped memory region, releasing the resources associated with it.
             * @note Userspace sig: `int munmap(void *addr, size_t length);`
             */
            MUNMAP = 215,

            /**
             * @brief Change the protection of a memory region
             * This system call changes the memory protection of a specified memory region, allowing or disallowing access to it.
             * @note Userspace sig: `int mprotect(void *addr, size_t len, int prot);`
             */
            MPROTECT = 226,
        };
    } // namespace SyscallID
} // namespace Hamster
