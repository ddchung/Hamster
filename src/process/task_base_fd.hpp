// Hamster task file descriptor

#pragma once

#include <filesystem/base_file.hpp>
#include <sys/types.h>
#include <cstdint>
#include <cstddef>

namespace Hamster
{
    class BaseTaskFD
    {
    public:
        virtual ~BaseTaskFD() = default;

        BaseTaskFD() = default;
        BaseTaskFD(const BaseTaskFD &) = delete;
        BaseTaskFD &operator=(const BaseTaskFD &) = delete;

        // Common functions
        // See `vfs.hpp` for more info

        virtual ssize_t read(void *buf, size_t size) = 0;
        virtual ssize_t write(const void *buf, size_t size) = 0;
        virtual int64_t seek(int64_t off, int whence) = 0;
        virtual int64_t tell() { return seek(0, H_SEEK_CUR); }
        virtual int stat(sys_stat *buf) = 0;
        virtual int truncate(int64_t size) = 0;
        virtual int64_t size() = 0;
        virtual int ioctl(int req, IoctlArg arg = IoctlArg()) = 0;
        virtual int set_flags(int flags) = 0;
        virtual int get_flags() = 0;
        virtual int poll(int op) = 0;
        virtual int sync() = 0;
        virtual int datasync() = 0;

        /**
         * @brief Get the VFS file descriptor
         * @return The VFS FD, or -1 on error and set `error`
         * @note Only valid for VFS task FDs
         * @note Other file descriptor types should error with `EINVAL`
         */
        virtual int get_vfs_fd() = 0;

        /**
         * @brief Get the file descriptor flags
         * @return The FD flags
         * @note This is *NOT* the flags that this FD was opened with. This contains
         *       FD_CLOEXEC
         */
        int get_fd_flags() { return fd_flags; }

        /**
         * @brief Set the file descriptor flags
         * @param fd_flags The new file descriptor flags
         */
        void set_fd_flags(int fd_flags) { this->fd_flags = fd_flags; }

    private:
        int fd_flags = 0;
    };
} // namespace Hamster

