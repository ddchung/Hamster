// Hamster statx system call implementation

#include <syscall/syscall.hpp>
#include <abi/structs.hpp>
#include <abi/syscall_id.hpp>
#include <filesystem/vfs.hpp>
#include <process/process.hpp>
#include <memory/allocator.hpp>
#include <errno/errno.h>
#include <cstring>


namespace Hamster
{
    int sys_statx(Thread &thread)
    {
        // Note: For now, we don't support full statx support, we will just convert a regular stat to statx

        // int statx(int dirfd, const char *pathname, int flags, unsigned int mask, struct statx *statxbuf);
        int32_t thread_dfd = get_arg(thread, 0);
        uint32_t path = get_arg(thread, 1);
        int32_t flags = get_arg(thread, 2);
        // uint32_t mask = get_arg(thread, 3); // not used for now
        uint32_t statxbuf_addr = get_arg(thread, 4);

        bool is_ref_root = false;
        // Get the path from the thread's memory space

        Process *process = thread.get_process();

        char *path_str = process->memory_space.get_string(path);

        if (!path_str)
        {
            return transfer_error(thread);
        }

        _trace("statx: dirfd=%d, path=\"%s\", flags=0x%x, statxbuf_addr=0x%x, ret: ", thread_dfd, path_str, flags, statxbuf_addr);

        if (path_str[0] == '\0' && !(flags & 0x1000)) // AT_EMPTY_PATH
        {
            error = ENOENT;
            dealloc(path_str);
            return transfer_error(thread);
        }

        if (path_str[0] == '/')
        {
            // Absolute path, use the root directory
            is_ref_root = true;
        }
        else if (thread_dfd == -100)
        {
            // AT_FDCWD, use the current working directory
            is_ref_root = true;

            // +1 for '/' and +1 for '\0'
            char *new_buffer = alloc<char>(process->cwd.length() + 1 + strlen(path_str) + 1);

            strcpy(new_buffer, process->cwd.c_str());
            strcat(new_buffer, "/");
            strcat(new_buffer, path_str);

            _trace("new path=\"%s\" ret: ", new_buffer);

            dealloc(path_str);
            path_str = new_buffer;
        }
        else if (thread_dfd < 0)
        {
            // Invalid directory file descriptor
            dealloc(path_str);
            error = EBADF;
            return transfer_error(thread);
        }

        int res = 0;
        sys_stat statbuf;

        int (VFS::*stat_fn)(const char*, sys_stat*);
        int (VFS::*statat_fn)(int, const char*, sys_stat*);

        if (flags & 0x100)
        {
            // AT_SYMLINK_NOFOLLOW
            stat_fn = &VFS::lstat;
            statat_fn = &VFS::lstatat;
        }
        else
        {
            // Follow symlinks
            stat_fn = &VFS::stat;
            statat_fn = &VFS::statat;
        }

        if (is_ref_root)
        {
            res = (vfs.*stat_fn)(path_str, &statbuf);
        }
        else
        {
            // Open the file relative to the directory file descriptor
            int dfd = deref_fd(thread, thread_dfd);
            if (dfd < 0)
            {
                dealloc(path_str);
                error = EBADF;
                return transfer_error(thread);
            }

            res = (vfs.*statat_fn)(dfd, path_str, &statbuf);
        }

        dealloc(path_str);

        if (res < 0)
        {
            return transfer_error(thread);
        }

        // Convert sys_stat to statx

        //(note: use struct to refer to the sys_statx struct, not this function)
        struct sys_statx statxbuf = {};

        statxbuf.mask = 0x7FFU; // Set to STATX_BASIC_STATS only, since we don't support all fields
        
        statxbuf.rdev_major = statbuf.rdev >> 32;
        statxbuf.rdev_minor = statbuf.rdev & 0xFFFFFFFF;
        statxbuf.ino = statbuf.ino;
        statxbuf.mode = statbuf.mode;
        statxbuf.nlink = statbuf.nlink;
        statxbuf.uid = statbuf.uid;
        statxbuf.gid = statbuf.gid;
        statxbuf.dev_major = statbuf.dev >> 32;
        statxbuf.dev_minor = statbuf.dev & 0xFFFFFFFF;
        statxbuf.size = statbuf.size;
        statxbuf.blksize = statbuf.blksize;
        statxbuf.blocks = statbuf.blocks;
        statxbuf.atime.sec = statbuf.atime;
        statxbuf.atime.nsec = statbuf.atime_nsec;
        statxbuf.mtime.sec = statbuf.mtime;
        statxbuf.mtime.nsec = statbuf.mtime_nsec;
        statxbuf.ctime.sec = statbuf.ctime;
        statxbuf.ctime.nsec = statbuf.ctime_nsec;

        // Copy to userspace

        if (process->memory_space.memcpy(statxbuf_addr, &statxbuf, sizeof(statxbuf)) != 0)
        {
            error = EFAULT;
            return transfer_error(thread);
        }

        return set_return(thread, 0);
    }
} // namespace Hamster

