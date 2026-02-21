#include <network/unix.hpp>
#include <memory/allocator.hpp>
#include <errno/errno.h>
#include <cstring>
#include <cassert>

namespace Hamster
{
    size_t unix_make_sockaddr(sys_sockaddr_un *addr, size_t size, const String &path)
    {
        memset(addr, 0, size);
        size_t total = sizeof(sys_sa_family_t) + path.length() + 1;
        if (size < sizeof(sys_sa_family_t))
            return total;
        addr->family = H_AF_UNIX;

        size_t to_copy = std::min(total, size)  - sizeof(sys_sa_family_t) - 1;
        memcpy(addr->path, path.data(), to_copy);
        return total;
    }

    ssize_t unix_read_sockaddr(const sys_sockaddr_un *addr, size_t size, String &path)
    {
        if (!addr || size <= sizeof(sys_sa_family_t) || !addr->path[0])
        {
            error = H_EINVAL;
            return -1;
        }

        path.assign(addr->path, size - sizeof(sys_sa_family_t));
        path.resize(strlen(path.c_str())); // Stop at null terminator
        return 0;
    }

    ssize_t BaseUnixSocketHandle::write(const uint8_t *buf, size_t size)
    {
        error = H_ENOTSUP;
        return -1;
    }

    ssize_t BaseUnixSocketHandle::read(uint8_t *buf, size_t size)
    {
        error = H_ENOTSUP;
        return -1;
    }

    int BaseUnixSocketHandle::ioctl(int req, IoctlArg arg)
    {
        error = H_ENOTSUP;
        return -1;
    }

    BaseSocket *UnixNetwork::socket(int type, int protocol)
    {
        if (protocol != 0)
        {
            error = H_EPROTONOSUPPORT;
            return nullptr;
        }

        switch (type)
        {
        case H_SOCK_STREAM:
            return alloc<UnixStreamSocket>();
        default:
            error = H_EPROTOTYPE;
            return nullptr;
        }
    }
} // namespace Hamster

