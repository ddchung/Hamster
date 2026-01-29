
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <poll.h>
#include <network/base_network.hpp>
#include <network/network_manager.hpp>
#include <memory/allocator.hpp>

#ifndef __linux__
# error "native networking only supports linux for now."
#endif

void swap_error();

class NativeNetwork : public Hamster::BaseNetwork
{
    class NativeSocket : public Hamster::BaseSocket
    {
    public:
        NativeSocket(int fd) : fd(fd) 
        {
            // Get the current flags
            int flags = fcntl(fd, F_GETFL, 0);
            if (flags == -1)
            {
                close(fd);
                fd = -1;
            }
            if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
            {
                close(fd);
                fd = -1;
            }
        }
        ~NativeSocket() override { close(fd); }

        ssize_t sendto(const Hamster::IOVec *buf, size_t count, int flags, const Hamster::sys_sockaddr *addr, Hamster::sys_socklen_t addrlen) override
        {
            struct ::msghdr hdr = {};
            hdr.msg_name = (void *)addr;
            hdr.msg_namelen = addrlen;
            hdr.msg_iov = (struct ::iovec *)buf;
            hdr.msg_iovlen = count;
            ssize_t res = ::sendmsg(fd, &hdr, MSG_NOSIGNAL | MSG_DONTWAIT);
            if (res == 0)
                swap_error();
            return res;
        }

        ssize_t recvfrom(const Hamster::IOVec *buf, size_t count, int flags, Hamster::sys_sockaddr *addr, Hamster::sys_socklen_t *addrlen) override
        {
            struct ::msghdr hdr = {};
            hdr.msg_name = (void *)addr;
            hdr.msg_iov = (struct ::iovec *)buf;
            hdr.msg_iovlen = count;
            ssize_t res = ::sendmsg(fd, &hdr, MSG_DONTWAIT);
            if (res == 0)
                swap_error();
            if (addrlen)
                *addrlen = hdr.msg_namelen;
            return res;
        }
        
        int bind(const Hamster::sys_sockaddr *addr, Hamster::sys_socklen_t socklen) override
        {
            if (::bind(fd, (struct ::sockaddr *)addr, socklen) < 0)
            {
                swap_error();
                return -1;
            }
            return 0;
        }

        int connect(const Hamster::sys_sockaddr *addr, Hamster::sys_socklen_t socklen) override
        {
            if (::connect(fd, (struct ::sockaddr *)addr, socklen) < 0)
            {
                swap_error();
                return -1;
            }
            return 0;
        }

        int listen(int backlog) override
        {
            if (::listen(fd, backlog) < 0)
            {
                swap_error();
                return -1;
            }
            return 0;
        }

        Hamster::BaseSocket *accept(Hamster::sys_sockaddr *addr, Hamster::sys_socklen_t *socklen) override
        {
            int res = ::accept(fd, (struct ::sockaddr *)addr, (::socklen_t *)socklen);
            if (res < 0)
            {
                swap_error();
                return nullptr;
            }
            return Hamster::alloc<NativeSocket>(1, res);
        }

        int poll(int ops) override
        {
            struct ::pollfd pfd = {};
            pfd.fd = fd;
            pfd.events = POLLIN | POLLOUT;

            if (::poll(&pfd, 1, 0) < 0)
            {
                swap_error();
                return -1;
            }
            if ((ops & Hamster::POLL_READ) && !(pfd.revents & POLLIN))
                return 0;
            if ((ops & Hamster::POLL_WRITE) && !(pfd.revents & POLLOUT))
                return 0;
            return 1;
        }

    private:
        int fd;
    };

public:

    Hamster::BaseSocket *socket(int type, int protocol) override
    {
        int res = ::socket(AF_INET, type, protocol);
        if (res < 0)
        {
            swap_error();
            return nullptr;
        }
        return Hamster::alloc<NativeSocket>(1, res);
    }
};

