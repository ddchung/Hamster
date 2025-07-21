// Hamster renameat and renameat2 system calls

#include <syscall/syscall.hpp>
#include <abi/syscall_id.hpp>
#include <process/process.hpp>
#include <memory/allocator.hpp>
#include <filesystem/vfs.hpp>
#include <errno/errno.h>

namespace Hamster
{
    int sys_renameat(Thread &thread)
    {
        int32_t olddirfd = get_arg(thread, 0);
        uint32_t oldpath = get_arg(thread, 1);
        int32_t newdirfd = get_arg(thread, 2);
        uint32_t newpath = get_arg(thread, 3);

        if ((olddirfd < 0 && olddirfd != -100) || // -100 is AT_FDCWD
            (newdirfd < 0 && newdirfd != -100))
        {
            error = EBADF;
            return transfer_error(thread);
        }

        if (!oldpath || !newpath)
        {
            error = EFAULT;
            return transfer_error(thread);
        }

        int vfs_old_dfd, vfs_new_dfd;

        if (olddirfd == -100) // AT_FDCWD
        {
            vfs_old_dfd = vfs.open("/", OPEN_RDWR);
        }
        else
        {
            vfs_old_dfd = deref_fd(thread, olddirfd);
        }

        if (vfs_old_dfd < 0)
        {
            error = EBADF;
            return transfer_error(thread);
        }

        if (newdirfd == -100) // AT_FDCWD
        {
            vfs_new_dfd = vfs.open("/", OPEN_RDWR);
        }
        else
        {
            vfs_new_dfd = deref_fd(thread, newdirfd);
        }

        if (vfs_new_dfd < 0)
        {
            // Close the old directory file descriptor if it we opened it
            if (olddirfd == -100)
                vfs.close(vfs_old_dfd);
            error = EBADF;
            return transfer_error(thread);
        }

        // Read the pathnames

        char *old_path_str = thread.get_process()->memory_space.get_string(oldpath);
        char *new_path_str = thread.get_process()->memory_space.get_string(newpath);
        if (!old_path_str || !new_path_str)
        {
            // Close the directory file descriptors if we opened them
            if (olddirfd == -100)
                vfs.close(vfs_old_dfd);
            if (newdirfd == -100)
                vfs.close(vfs_new_dfd);
            dealloc(old_path_str);
            dealloc(new_path_str);
            error = EFAULT;
            return transfer_error(thread);
        }

        // Perform the rename operation
        int res;

        if (old_path_str[0] == '\0' || new_path_str[0] == '\0')
        {
            // If either path is empty, we cannot rename
            res = -1;
            error = ENOENT;
        }
        else
        {
            res = vfs.renameat(vfs_old_dfd, old_path_str, vfs_new_dfd, new_path_str);
        }

        dealloc(old_path_str);
        dealloc(new_path_str);
        
        if (olddirfd == -100)
            vfs.close(vfs_old_dfd);
        if (newdirfd == -100)
            vfs.close(vfs_new_dfd);
        
        if (res < 0)
        {
            return transfer_error(thread);
        }

        return set_return(thread, 0);
    }

    // For now, we don't support renameat2 flags, it will just redirect to renameat
    int sys_renameat2(Thread &thread)
    {
        // Check for unsupported options
        uint32_t flags = get_arg(thread, 4);

        if (flags != 0)
        {
            error = ENOTSUP; // Not supported
            return transfer_error(thread);
        }
        return sys_renameat(thread);
    }
} // namespace Hamster
