// Hamster VFS task FD

#pragma once

#include <process/task_base_fd.hpp>
#include <filesystem/vfs.hpp>

namespace Hamster
{
    class TaskVFSFD : public BaseTaskFD
    {
    public:
        /**
         * @brief Make a task VFS fd
         * @param fd The VFS file descriptor
         * @note This takes ownership of `fd`
         */
        TaskVFSFD(int fd) : fd(fd), dir_offset(0), is_dir(vfs.is_directory(fd) == 1) {}
        ~TaskVFSFD() override { vfs.close(fd); fd = -1; }
        TaskVFSFD(TaskVFSFD &&) = delete;
        TaskVFSFD &operator=(TaskVFSFD &&) = delete;

        ssize_t read(void *buf, size_t size) override { return vfs.read(fd, buf, size); }
        ssize_t write(const void *buf, size_t size) override { return vfs.write(fd, buf, size); }
        int64_t seek(int64_t off, int whence) override
        {
            if (is_dir)
            {
                switch (whence)
                {
                case H_SEEK_SET:
                    if (off >= 0)
                        dir_offset = off;
                    break;
                case H_SEEK_CUR:
                    if ((int64_t)dir_offset + off >= 0)
                        dir_offset += off;
                    break;
                }
                return 0;
            }
            else
            {
                return vfs.seek(fd, off, whence);
            }
        }
        int64_t tell() override { return is_dir ? dir_offset : vfs.tell(fd); }
        int stat(sys_stat *buf) override { return vfs.stat(fd, buf); }
        int truncate(int64_t size) override { return vfs.truncate(fd, size); }
        int64_t size() override { return vfs.size(fd); }
        int ioctl(int req, IoctlArg arg = IoctlArg()) override { return vfs.ioctl(fd, req, arg); }
        int set_flags(int flags) override { return vfs.set_flags(fd, flags); }
        int get_flags() override { return vfs.get_flags(fd); }
        int poll(int op) override { return vfs.poll(fd, op); }
        int sync() override { return vfs.sync(fd); }
        int datasync() override { return vfs.datasync(fd); }
        int get_vfs_fd() override { return fd; }
        char *const *list() override { return vfs.list(fd); }

    private:
        int fd;
        uint64_t dir_offset : 63;
        uint64_t is_dir : 1;
    };
} // namespace Hamster

