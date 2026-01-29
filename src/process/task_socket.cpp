#include <process/task_socket.hpp>
#include <memory/allocator.hpp>

namespace Hamster
{
    TaskSocket::TaskSocket(int flags, BaseSocket *socket)
        : socket(socket), flags(flags)
    {
    }

    TaskSocket::~TaskSocket()
    {
        dealloc(socket);
    }

    ssize_t TaskSocket::readv(const IOVec *iovec, size_t iovcnt)
    {
        return socket->recvfrom(iovec, iovcnt, 0, nullptr, nullptr);
    }

    ssize_t TaskSocket::writev(const IOVec *iovec, size_t iovcnt)
    {
        return socket->sendto(iovec, iovcnt, 0, nullptr, 0);
    }

    // TODO: proper stat
    int TaskSocket::stat(sys_stat *buf)
    {
        memset(buf, 0, sizeof(sys_stat));
        buf->mode = STAT_IFSOCK | 0666;
        buf->size = size();
        return 0;
    }

    int64_t TaskSocket::size()
    {
        return 0;
    }

    // TODO: Socket ioctls
    int TaskSocket::ioctl(int req, IoctlArg arg)
    {
        error = H_ENOTSUP;
        return -1;
    }

    int TaskSocket::set_flags(int flags)
    {
        this->flags = flags;
        return 0;
    }

    int TaskSocket::get_flags()
    {
        return flags;
    }

    int TaskSocket::poll(int op)
    {
        return socket->poll(op);
    }

    int TaskSocket::sync()
    {
        return 0;
    }

    int TaskSocket::datasync()
    {
        return 0;
    }
} // namespace Hamster

