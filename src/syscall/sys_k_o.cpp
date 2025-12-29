// Hamster K-O system calls

#include <syscall/syscall.hpp>
#include <process/task.hpp>
#include <process/task_vfs_fd.hpp>
#include <abi/values.hpp>
#include <abi/structs.hpp>

namespace Hamster
{
    int32_t sys_openat(Task &task, int32_t thread_dfd, uint32_t path_loc, int32_t flags, uint32_t mode)
    {
        char *path = task.mem_get_string(path_loc);
        if (!path)
            return cvt_error();
        
        int rel_fd = task.open_rel_fd(thread_dfd, path);
        if (rel_fd < 0)
        {
            dealloc(path);
            return cvt_error();
        }
        
        // Process mode with umask
        mode = task.mask_mode(mode);

        int fd = vfs.openat(rel_fd, path, flags, mode);
        dealloc(path);
        vfs.close(rel_fd);

        if (fd < 0)
            return cvt_error();

        int task_fd = task.allocate_fd();
        if (task_fd < 0)
        {
            vfs.close(fd);
            return cvt_error();
        }

        TaskVFSFD *p_fd = alloc<TaskVFSFD>(1, fd);

        // Make a new TaskVFSFD with the opened file descriptor, and put it in the
        // allocated slot
        if (task.set_fd(p_fd, task_fd) < 0)
        {
            dealloc(p_fd);
            return cvt_error();
        }
        task.set_fd_flags(task_fd, flags & OPEN_CLOEXEC ? H_FD_CLOEXEC : 0);
        return task_fd;
    }

    int32_t sys_mmap2(Task &task, uint32_t addr, uint32_t length, uint32_t prot, uint32_t flags, int32_t fd, uint32_t offset)
    {
        uint8_t perms = (prot & H_PROT_READ ? PERM_READ : 0) |
                        (prot & H_PROT_WRITE ? PERM_WRITE : 0) |
                        (prot & H_PROT_EXEC ? PERM_EXEC : 0);
        uint32_t res = task.mmap(addr, length, perms, flags, fd, offset);
        if (res == UINT32_MAX)
            return cvt_error();
        return res;
    }

    int32_t sys_llseek(Task &task, int32_t task_fd, uint32_t off_high, uint32_t off_low, uint32_t result_loc, int32_t whence)
    {
        int64_t off64 = ((int64_t)off_high << 32) | off_low;

        BaseTaskFD *fd = task.get_fd(task_fd);
        if (!fd)
            return cvt_error();
        
        int64_t res = fd->seek(off64, whence);
        if (res < 0)
            return cvt_error();
        
        if (task.copy_to_memory(result_loc, res) < 0)
            return cvt_error();
        
        return 0;
    }

    int32_t sys_kill(Task &task, int32_t pid, int32_t sig)
    {
        if (sig < 0 || sig > H_SIGRTMAX)
            return -H_EINVAL;

        Task *target;
        sys_siginfo siginfo = make_kill_siginfo(sig, sys_getuid(task), task.get_pid());
        if (pid > 0)
        {
            // Send to specified process
            target = Task::get_task_pid(pid);
            if (!target)
                return cvt_error();
            if (task.check_can_signal(*target, siginfo) < 0)
                return cvt_error();
            if (sig != 0 && target->send_signal_process(siginfo) < 0)
                return cvt_error();
            return 0;
        }
        else if (pid == 0)
        {
            // send to own pgroup
            if (sig != 0 && task.send_signal_pgroup(siginfo) < 0)
                return cvt_error();
            return 0;
        }
        else if (pid == -1)
        {
            // send to all processes except init
            if (sig != 0)
                task.signal_all_processes(siginfo);
            return 0;
        }
        else
        {
            // Send to process group with pgid == -`pid`
            target = Task::get_task_pgid(-pid);
            if (!target)
                return cvt_error();
            if (sig != 0 && target->send_signal_pgroup(siginfo) < 0)
                return cvt_error();
            return 0;
        }
    }

    int32_t sys_munmap(Task &task, uint32_t addr, uint32_t len)
    {
        if (task.munmap(addr, len) < 0)
            return cvt_error();
        return 0;
    }

    int32_t sys_mkdirat(Task &task, int32_t dirfd, uint32_t path_loc, uint32_t mode)
    {
        char *path = task.mem_get_string(path_loc);
        if (!path)
            return cvt_error();
        
        int rel_fd = task.open_rel_fd(dirfd, path);
        
        int res = vfs.mkdirat(rel_fd, path, mode);
        vfs.close(rel_fd);
        dealloc(path);

        if (res < 0)
            return cvt_error();
        return 0;
    }

    int32_t sys_linkat(Task &task, int32_t old_dfd, uint32_t oldpath_loc, int32_t new_dfd, uint32_t newpath_loc, int32_t flags)
    {
        // TODO: support flags
        if (flags != 0)
            return -H_ENOTSUP;

        char *old_path = task.mem_get_string(oldpath_loc);
        char *new_path = task.mem_get_string(newpath_loc);

        if (!old_path || !new_path)
        {
            dealloc(old_path);
            dealloc(new_path);
            return cvt_error();
        }

        int old_rel_fd = task.open_rel_fd(old_dfd, old_path);
        int new_rel_fd = task.open_rel_fd(new_dfd, new_path);

        int res = vfs.linkat(old_rel_fd, old_path, new_rel_fd, new_path);
        vfs.close(old_rel_fd);
        vfs.close(new_rel_fd);
        dealloc(old_path);
        dealloc(new_path);

        if (res < 0)
            return cvt_error();
        return 0;
    }

    int32_t sys_mprotect(Task &task, uint32_t addr, uint32_t size, int32_t prot)
    {
        if (!task.mem_is_mapped(addr, size))
            return -H_ENOMEM;
        
        if (prot & (H_PROT_GROWSUP | H_PROT_GROWSDOWN))
        {
            // We don't support these
            return -H_ENOTSUP;
        }

        uint8_t perms = (prot & H_PROT_READ ? PERM_READ : 0) |
                        (prot & H_PROT_WRITE ? PERM_WRITE : 0) |
                        (prot & H_PROT_EXEC ? PERM_EXEC : 0);

        if (task.mprotect(addr, size, perms) < 0)
            return cvt_error();
        return 0;
    }
} // namespace Hamster

