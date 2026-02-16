// Hamster task file descriptor

#pragma once

#include <memory/scatter_io.hpp>
#include <filesystem/base_file.hpp>
#include <sys/types.h>
#include <cstdint>
#include <cstddef>

namespace Hamster
{
    enum class TaskFDType
    {
        VFS,
        Pipe,
        Socket,
    };
    
    class BaseTaskFD
    {
    public:
        virtual ~BaseTaskFD() = default;

        BaseTaskFD() = default;
        BaseTaskFD(const BaseTaskFD &) = delete;
        BaseTaskFD &operator=(const BaseTaskFD &) = delete;

        virtual TaskFDType type() const = 0;

        // Common functions
        // See `vfs.hpp` for more info

        // Please implement at least one of read/readv and of write/writev
        virtual ssize_t read(void *buf, size_t size);
        virtual ssize_t write(const void *buf, size_t size);
        virtual ssize_t readv(const IOVec *iovec, size_t iovcnt);
        virtual ssize_t writev(const IOVec *iovec, size_t iovcnt);

        virtual int64_t seek(int64_t off, int whence);
        virtual int64_t tell() { return seek(0, H_SEEK_CUR); }
        virtual int stat(sys_stat *buf) = 0;
        virtual int truncate(int64_t size);
        virtual int64_t size() = 0;
        virtual int ioctl(int req, IoctlArg arg = IoctlArg()) = 0;
        virtual int set_flags(int flags) = 0;
        virtual int get_flags() = 0;
        virtual int poll(int op) = 0;
        virtual int sync() = 0;
        virtual int datasync() = 0;
        virtual char *const *list() { return nullptr; }

        /**
         * @brief Get the VFS file descriptor
         * @return The VFS FD, or -1 on error and set `error`
         * @note Only valid for VFS task FDs
         */
        virtual int get_vfs_fd();
    };
} // namespace Hamster

