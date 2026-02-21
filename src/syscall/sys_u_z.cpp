// Hamster U-Z system calls

#include <syscall/syscall.hpp>
#include <process/task.hpp>
#include <process/task_vfs_fd.hpp>
#include <process/task_scatter_io.hpp>
#include <logger/logger.hpp>
#include <abi/values.hpp>
#include <abi/structs.hpp>
#include <inttypes.h>

namespace Hamster
{
    namespace
    {
        int do_write(Task &task, int32_t task_fd, const std::pair<IOVec  *, uint32_t> &iov)
        {
            return task.block([](Task &task, uint64_t data, void *iov) {
                uint32_t task_fd = data >> 32, iovlen = data & 0xFFFF'FFFF;
                int32_t res = 0;

                BaseTaskFD *fd = task.get_fd(task_fd);
                if (!fd)
                {
                    res = cvt_error();
                    goto done;
                }

                res = fd->writev((const IOVec *)iov, iovlen);
                if (res < 0)
                {
                    if (error == H_EAGAIN && !(fd->get_flags() & OPEN_NONBLOCK))
                        return; // continue blocking
                    res = cvt_error();
                }

            done:
                dealloc((IOVec *)iov);
                task.end_block();
                logger("syscall", "do_write", Logger::LEVEL_DEBUG) << "Done write operation, result " << res;
                task.get_emulator().x[10] = res;
            }, iov.second | ((uint64_t)task_fd << 32), iov.first, [](Task &task, uint64_t iovlen, void *iov) {
                dealloc((IOVec *)iov);
                task.get_emulator().x[10] = -H_EINTR;
            });
        }
    } // namespace
    

    int32_t sys_write(Task &task, int32_t task_fd, uint32_t buf_loc, uint32_t count)
    {
        std::pair<IOVec *, uint32_t> iov = make_iovec_buf(task, buf_loc, count, PERM_READ);
        if (!iov.first)
            return cvt_error();
        
        int res = do_write(task, task_fd, iov);
        if (res < 0)
        {
            dealloc(iov.first);
            return cvt_error();
        }
        return 0;
    }

    int32_t sys_waitid(Task &task, int32_t idtype, int32_t id, uint32_t siginfo_loc, int32_t options, uint32_t rusage_loc)
    {
        task.get_emulator().x[10] = idtype;    // a0
        task.get_emulator().x[11] = id;        // a1
        task.get_emulator().x[12] = siginfo_loc; // a2
        task.get_emulator().x[13] = options;   // a3
        task.get_emulator().x[14] = rusage_loc; // a4

        task.block([](Task &task, uint32_t idtype, uint32_t id, uint32_t siginfo_loc, uint32_t options, uint32_t rusage_loc, uint32_t) {
            sys_siginfo siginfo = {};
            int res = task.waitid(idtype, id, &siginfo, options);

            if (res < 0)
            {
                if (error == H_EAGAIN && (options & H_WNOHANG))
                    return -H_EAGAIN;
                // note: this forwards EAGAIN, which continues blocking
                return -1;
            }

            if ((siginfo_loc && task.copy_to_memory(siginfo_loc, siginfo) < 0)
             || (rusage_loc && task.memset(rusage_loc, 0, sizeof(sys_rusage)) < 0))
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
            logger("syscall", "sys_unlinkat", Logger::LEVEL_WARNING) << "lstatat failed for path '" << path_str << "'\n"
                << "Unexpected error: " << error << "\n"
                << "Return value: " << result;
        }

        // Unlink the file or directory
        result = vfs.removeat(vfs_rel_fd, path_str);
        dealloc(path_str);
        vfs.close(vfs_rel_fd);
        
        if (result < 0)
            return cvt_error();
        return 0;
    }

    int32_t sys_uname(Task &task, uint32_t buf_loc)
    {
        constexpr sys_utsname default_uname = {
            .sysname = HAMSTER_SYSNAME,
            .nodename = HAMSTER_NODENAME,
            .release = HAMSTER_RELEASE,
            .version = HAMSTER_VERSION,
            .machine = HAMSTER_MACHINE,
            .domainname = HAMSTER_DOMAINNAME,
        };

        if (task.copy_to_memory(buf_loc, default_uname) < 0)
            return cvt_error();

        return 0;
    }

    int32_t sys_writev(Task &task, int32_t task_fd, uint32_t vec_loc, uint32_t vlen)
    {
        std::pair<IOVec *, uint32_t> iov = make_iovec(task, vec_loc, vlen, PERM_READ);
        if (!iov.first)
            return cvt_error();
        
        int res = do_write(task, task_fd, iov);
        if (res < 0)
        {
            dealloc(iov.first);
            return cvt_error();
        }
        return 0;
    }
} // namespace Hamster

