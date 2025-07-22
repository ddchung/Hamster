// Hamster dup and dup3 system calls

#include <syscall/syscall.hpp>
#include <process/process.hpp>
#include <filesystem/vfs.hpp>
#include <errno/errno.h>

namespace Hamster
{
    int sys_dup(Thread &thread)
    {
        int oldfd = deref_fd(thread, get_arg(thread, 0));

        if (oldfd < 0)
        {
            error = EBADF;
            return transfer_error(thread);
        }

        auto &fds = thread.get_process()->fds;

        fd_refcount[oldfd]++;

        ssize_t index = -1;
        for (size_t i = 0; i < fds.size(); ++i)
        {
            if (fds[i].fd < 0)
            {
                index = i;
                break;
            }
        }

        if (index < 0)
        {
            index = fds.size();
            fds.emplace_back();
        }

        fds[index].fd = oldfd;
        fds[index].fd_flags = 0; // Not FD_CLOEXEC

        return set_return(thread, index);
    }

    int sys_dup3(Thread &thread)
    {
        int32_t oldfd = get_arg(thread, 0);
        int32_t newfd = get_arg(thread, 1);
        int32_t flags = get_arg(thread, 2);

        if (oldfd < 0 || newfd < 0)
        {
            error = EBADF;
            return transfer_error(thread);
        }

        if (oldfd == newfd)
        {
            // dup3 says: "
            //  dup3 is the same as dup2, except that: [...] If oldfd equals newfd, then dup3() fails with the error EINVAL.
            // "
            error = EINVAL;
            return transfer_error(thread);
        }

        int vfs_oldfd = deref_fd(thread, oldfd);
        if (vfs_oldfd < 0)
        {
            error = EBADF;
            return transfer_error(thread);
        }

        auto &fds = thread.get_process()->fds;

        size_t old_size = fds.size();
        if (newfd >= (int32_t)old_size)
        {
            fds.resize(newfd + 1);
            for (size_t i = old_size; (int32_t)i <= newfd; ++i)
            {
                fds[i].fd = -1; // Mark as unused
                fds[i].fd_flags = 0;
            }
        }

        if (fds[newfd].fd >= 0)
        {
            fd_refcount[fds[newfd].fd]--;
            if (fd_refcount[fds[newfd].fd] == 0)
            {
                vfs.close(fds[newfd].fd);
                fd_refcount.erase(fds[newfd].fd);
            }
        }

        fds[newfd].fd = vfs_oldfd;
        fds[newfd].fd_flags = (flags & 02000000) ? 0x1 : 0x0;
        fd_refcount[vfs_oldfd]++; // Increment refcount

        return set_return(thread, newfd);
    }
} // namespace Hamster

