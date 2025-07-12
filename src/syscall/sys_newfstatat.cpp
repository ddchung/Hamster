// Hamster newfstatat syscall implementation

#include <syscall/syscall.hpp>
#include <abi/syscall_id.hpp>
#include <abi/structs.hpp>
#include <filesystem/vfs.hpp>
#include <errno/errno.h>
#include <process/process.hpp>
#include <memory/allocator.hpp>
#include <cstring>

namespace Hamster
{
    int sys_newfstatat(Thread &thread)
    {
        // int newfstatat(int dirfd, const char *pathname, struct stat *statbuf, int flags);
        int32_t dirfd = get_arg(thread, 0);
        uint32_t pathname_addr = get_arg(thread, 1);
        uint32_t statbuf_addr = get_arg(thread, 2);
        int32_t flags = get_arg(thread, 3);

        sys_stat statbuf;

        // Validate options

        if (dirfd < 0 && dirfd != -100) // -100 is AT_FDCWD
        {
            error = EINVAL;
            return transfer_error(thread);
        }

        if (!pathname_addr || !statbuf_addr)
        {
            error = EFAULT;
            return transfer_error(thread);
        }

        Process *process = thread.get_process();

        int vfs_dfd = deref_fd(thread, dirfd);

        int res = -1;

        char *pathname = process->memory_space.get_string(pathname_addr);

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

        if (flags & 0x1000 && pathname && pathname[0] != '\0')
            flags &= ~0x1000; // Clear AT_EMPTY_PATH if path isn't empty
        
        if (flags & 0x1000)
        {
            if (dirfd == -100)
            {
                res = (vfs.*stat_fn)(process->cwd.c_str(), &statbuf);
            }
            else if (vfs_dfd < 0)
            {
                error = EBADF;
                res = -1;
            }
            else
            {
                res = vfs.stat(vfs_dfd, &statbuf);
            }
        }
        else
        {
            if (!pathname || pathname[0] == '\0')
            {
                error = EINVAL;
                res = -1;
            }
            else if (dirfd == -100)
            {
                char *buf = alloc<char>(process->cwd.size() + 1 + strlen(pathname) + 1);
                strcpy(buf, process->cwd.c_str());
                strcat(buf, "/");
                strcat(buf, pathname);
                res = (vfs.*stat_fn)(buf, &statbuf);
                dealloc(buf);
            }
            else if (vfs_dfd < 0)
            {
                error = EBADF;
                res = -1;
            }
            else
            {
                res = (vfs.*statat_fn)(vfs_dfd, pathname, &statbuf);
            }
        }

        dealloc(pathname);

        if (res < 0)
        {
            return transfer_error(thread);
        }

        // Copy the stat structure to user space
        if (!process->memory_space.memcpy(statbuf_addr, &statbuf, sizeof(sys_stat)))
        {
            error = EFAULT;
            return transfer_error(thread);
        }

        // Return success
        return set_return(thread, 0);
    }
} // namespace Hamster

