// Hamster U-Z system calls

#include <syscall/syscall.hpp>
#include <process/task.hpp>
#include <process/task_vfs_fd.hpp>
#include <abi/values.hpp>
#include <abi/structs.hpp>

namespace Hamster
{
    int32_t sys_write(Task &task, int32_t fd, uint32_t buf_loc, uint32_t count)
    {
        task.block([](Task &task, uint32_t task_fd, uint32_t buf_loc, uint32_t count, uint32_t, uint32_t, uint32_t) -> int {
            BaseTaskFD *fd = task.get_fd(task_fd);
            if (!fd)
                return -1;
            
            bool is_nonblock = fd->get_flags() & OPEN_NONBLOCK;

            // Check if file is writable
            switch (fd->poll(POLL_WRITE))
            {
            case 0:
                if (is_nonblock)
                    return -H_EAGAIN;
                error = H_EAGAIN;
                return -1;
            case 1:
                break;
            default:
                return -1;
            }

            // Write up to end of VM page

            uint32_t to_write = std::min(count, HAMSTER_PAGE_SIZE - (buf_loc % HAMSTER_PAGE_SIZE));

            const void *it = task.mem_make_iterator_read(buf_loc);
            if (!it)
                return -1;
            
            int res = fd->write(it, to_write);
            if (res == -1 && error == H_EAGAIN && is_nonblock)
                return -H_EAGAIN;
            return res;
        });

        return 0;
    }

    int32_t sys_waitid(Task &task, int32_t idtype, int32_t id, uint32_t siginfo_loc, int32_t options, uint32_t rusage_loc)
    {
        task.block([](Task &task, uint32_t idtype, uint32_t id, uint32_t siginfo_loc, uint32_t options, uint32_t rusage_loc, uint32_t) {
            sys_siginfo siginfo = {};
            int res = task.waitid(idtype, id, &siginfo, options);

            // note: this forwards EAGAIN, which continues blocking
            if (res < 0)
                return -1;

            if (siginfo_loc && (task.copy_to_memory(siginfo_loc, siginfo) < 0
                || task.memset(rusage_loc, 0, sizeof(sys_rusage)) < 0))
            {
                error = H_EFAULT;
                return -1;
            }
            return 0;
        });
        return 0;
    }

    int32_t sys_unlinkat(Task &task, int32_t thread_dfd, uint32_t path_loc, int32_t flags)
    {
        // Get the path
        char *path_str = task.mem_get_string(path_loc);
        if (path_str == nullptr)
        {
            error = H_EFAULT;
            return cvt_error();
        }

        int vfs_rel_fd = task.open_rel_fd(thread_dfd, path_str);
        if (vfs_rel_fd < 0)
        {
            dealloc(path_str);
            return cvt_error();
        }

        // Check if it's a directory
        sys_stat statbuf;
        int result = vfs.lstatat(vfs_rel_fd, path_str, &statbuf);
        if (result == 0)
        {
            int err = 0;
            if (is_directory(statbuf.mode) && (flags & H_AT_REMOVEDIR) == 0)
                err = H_EISDIR;
            else if (!is_directory(statbuf.mode) && (flags & H_AT_REMOVEDIR) != 0)
                err = H_ENOTDIR;
            
            if (err)
            {
                dealloc(path_str);
                vfs.close(vfs_rel_fd);
                return -err;
            }
        }
        else
        {
            _trace("WARN: sys_unlinkat: lstatat failed for path '%s' with unexpected error %d, return value %d\n",
                   path_str, error, result);
            _trace("WARN: sys_unlinkat: continuing anyway with remove operation..\n");
            _trace("WARN: sys_unlinkat: note: from %s:%d\n", __FILE__, __LINE__);
        }

        // Unlink the file or directory
        result = vfs.removeat(vfs_rel_fd, path_str);
        dealloc(path_str);
        vfs.close(vfs_rel_fd);
        
        if (result < 0)
            return cvt_error();
        return 0;
    }
} // namespace Hamster

