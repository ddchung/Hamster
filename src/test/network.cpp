
#include <filesystem/vfs.hpp>
#include <filesystem/ramfs.hpp>
#include <network/network_manager.hpp>
#include <network/unix.hpp>

using namespace Hamster;

#ifndef NDEBUG

void test_network()
{
    assert(vfs.mount("/", alloc<RamFs>()) == 0);
    UnixStreamSocket *sock = alloc<UnixStreamSocket>();
    sys_sockaddr_un sockaddr{H_AF_UNIX, "/sock"};
    assert(sock->bind((sys_sockaddr *)&sockaddr, sizeof(sockaddr)) == 0);
    assert(sock->listen(10) == 0);

    UnixStreamSocket *client = alloc<UnixStreamSocket>();
    assert(client->connect((sys_sockaddr *)&sockaddr, sizeof(sockaddr)) == 0);

    BaseSocket *accepted = sock->accept(nullptr, 0);
    assert(accepted != nullptr);
    assert(client->poll(POLL_WRITE) == 1);
    
    char buf[] = "Hello, World";
    IOVec iov;
    iov.data = &buf;
    iov.size = sizeof(buf);
    assert(client->sendto(&iov, 1, 0, nullptr, 0) == (ssize_t)iov.size);

    char buf2[sizeof(buf)];
    iov.data = &buf2;
    assert(accepted->recvfrom(&iov, 1, 0, nullptr, 0) == (ssize_t)iov.size);
    assert(memcmp(buf, buf2, sizeof(buf)) == 0);

    dealloc(client);
    dealloc(accepted);
    dealloc(sock);
    assert(vfs.unmount("/") == 0);
}

#endif // NDEBUG
