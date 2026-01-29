#pragma once

#include <process/task_base_fd.hpp>
#include <network/base_socket.hpp>

namespace Hamster
{
    // Wraps a BaseSocket as a Task FD
    class TaskSocket : public BaseTaskFD
    {
    public:
        // Takes ownership of socket
        TaskSocket(int flags, BaseSocket *socket);
        ~TaskSocket() override;
        TaskSocket(TaskSocket &&) = delete;

        TaskFDType type() const override { return TaskFDType::Socket; }

        ssize_t readv(const IOVec *iovec, size_t iovcnt) override;
        ssize_t writev(const IOVec *iovec, size_t iovcnt) override;
        int stat(sys_stat *buf) override;
        int64_t size() override;
        int ioctl(int req, IoctlArg arg = IoctlArg()) override;
        int set_flags(int flags) override;
        int get_flags() override;
        int poll(int op) override;
        int sync() override;
        int datasync() override;

        BaseSocket *get_socket() { return socket; }

    private:
        BaseSocket *socket;
        int flags;
    };
} // namespace Hamster

