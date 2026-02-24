
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

    // Additional stream socket test: multiple send/recv
    UnixStreamSocket *client2 = alloc<UnixStreamSocket>();
    assert(client2->connect((sys_sockaddr *)&sockaddr, sizeof(sockaddr)) == 0);
    BaseSocket *accepted2 = sock->accept(nullptr, 0);
    char buf3[] = "StreamTest";
    IOVec iov3;
    iov3.data = &buf3;
    iov3.size = sizeof(buf3);
    assert(client2->sendto(&iov3, 1, 0, nullptr, 0) == (ssize_t)iov3.size);
    char buf4[sizeof(buf3)];
    iov3.data = &buf4;
    assert(accepted2->recvfrom(&iov3, 1, 0, nullptr, 0) == (ssize_t)iov3.size);
    assert(memcmp(buf3, buf4, sizeof(buf3)) == 0);
    dealloc(client2);
    dealloc(accepted2);

    // Unix datagram socket tests
    UnixDgramSocket *dgram1 = alloc<UnixDgramSocket>();
    sys_sockaddr_un sockaddr_d1{H_AF_UNIX, "/dgram1"};
    assert(dgram1->bind((sys_sockaddr *)&sockaddr_d1, sizeof(sockaddr_d1)) == 0);
    UnixDgramSocket *dgram2 = alloc<UnixDgramSocket>();
    sys_sockaddr_un sockaddr_d2{H_AF_UNIX, "/dgram2"};
    assert(dgram2->bind((sys_sockaddr *)&sockaddr_d2, sizeof(sockaddr_d2)) == 0);
    char dmsg[] = "DatagramMsg";
    IOVec diov;
    diov.data = &dmsg;
    diov.size = sizeof(dmsg);
    assert(dgram1->sendto(&diov, 1, 0, (sys_sockaddr *)&sockaddr_d2, sizeof(sockaddr_d2)) == (ssize_t)diov.size);
    char drecv[sizeof(dmsg)];
    diov.data = &drecv;
    sys_sockaddr_un from_addr;
    sys_socklen_t from_len = sizeof(from_addr);
    assert(dgram2->recvfrom(&diov, 1, 0, (sys_sockaddr *)&from_addr, &from_len) == (ssize_t)diov.size);
    assert(memcmp(dmsg, drecv, sizeof(dmsg)) == 0);

    // Edge case: zero-length datagram
    diov.size = 0;
    assert(dgram1->sendto(&diov, 1, 0, (sys_sockaddr *)&sockaddr_d2, sizeof(sockaddr_d2)) == 0);
    assert(dgram2->recvfrom(&diov, 1, 0, nullptr, nullptr) == 0);

    // Poll test for datagram
    assert(dgram1->poll(POLL_WRITE) == 1);
    assert(dgram2->poll(POLL_READ | POLL_WRITE) >= 0);

    // Stream socket: send/recv with multiple IOVec
    UnixStreamSocket *client3 = alloc<UnixStreamSocket>();
    assert(client3->connect((sys_sockaddr *)&sockaddr, sizeof(sockaddr)) == 0);
    BaseSocket *accepted3 = sock->accept(nullptr, 0);
    char s1[] = "Hamster ";
    char s2[] = "Network ";
    char s3[] = "Test!";
    IOVec siov[3];
    siov[0].data = &s1;
    siov[0].size = sizeof(s1);
    siov[1].data = &s2;
    siov[1].size = sizeof(s2);
    siov[2].data = &s3;
    siov[2].size = sizeof(s3);
    ssize_t sent = client3->sendto(siov, 3, 0, nullptr, 0);
    assert(sent == (ssize_t)(sizeof(s1) + sizeof(s2) + sizeof(s3)));
    char r1[sizeof(s1)], r2[sizeof(s2)], r3[sizeof(s3)];
    IOVec riov[3];
    riov[0].data = &r1;
    riov[0].size = sizeof(r1);
    riov[1].data = &r2;
    riov[1].size = sizeof(r2);
    riov[2].data = &r3;
    riov[2].size = sizeof(r3);
    ssize_t recvd = accepted3->recvfrom(riov, 3, 0, nullptr, 0);
    assert(recvd == sent);
    assert(memcmp(s1, r1, sizeof(s1)) == 0);
    assert(memcmp(s2, r2, sizeof(s2)) == 0);
    assert(memcmp(s3, r3, sizeof(s3)) == 0);
    dealloc(client3);
    dealloc(accepted3);

    // Datagram socket: send/recv with multiple IOVec
    UnixDgramSocket *dgram3 = alloc<UnixDgramSocket>();
    sys_sockaddr_un sockaddr_d3{H_AF_UNIX, "/dgram3"};
    assert(dgram3->bind((sys_sockaddr *)&sockaddr_d3, sizeof(sockaddr_d3)) == 0);
    char d1[] = "Hamster ";
    char d2[] = "Datagram ";
    char d3[] = "IOVec!";
    IOVec diov_multi[3];
    diov_multi[0].data = &d1;
    diov_multi[0].size = sizeof(d1);
    diov_multi[1].data = &d2;
    diov_multi[1].size = sizeof(d2);
    diov_multi[2].data = &d3;
    diov_multi[2].size = sizeof(d3);
    ssize_t dsent = dgram1->sendto(diov_multi, 3, 0, (sys_sockaddr *)&sockaddr_d3, sizeof(sockaddr_d3));
    assert(dsent == (ssize_t)(sizeof(d1) + sizeof(d2) + sizeof(d3)));
    char dr1[sizeof(d1)], dr2[sizeof(d2)], dr3[sizeof(d3)];
    IOVec driov[3];
    driov[0].data = &dr1;
    driov[0].size = sizeof(dr1);
    driov[1].data = &dr2;
    driov[1].size = sizeof(dr2);
    driov[2].data = &dr3;
    driov[2].size = sizeof(dr3);
    sys_sockaddr_un from_addr_multi;
    sys_socklen_t from_len_multi = sizeof(from_addr_multi);
    ssize_t drecvd = dgram3->recvfrom(driov, 3, 0, (sys_sockaddr *)&from_addr_multi, &from_len_multi);
    assert(drecvd == dsent);
    assert(memcmp(d1, dr1, sizeof(d1)) == 0);
    assert(memcmp(d2, dr2, sizeof(d2)) == 0);
    assert(memcmp(d3, dr3, sizeof(d3)) == 0);
    dealloc(dgram3);

    // Large buffer test for stream socket
    UnixStreamSocket *client4 = alloc<UnixStreamSocket>();
    assert(client4->connect((sys_sockaddr *)&sockaddr, sizeof(sockaddr)) == 0);
    BaseSocket *accepted4 = sock->accept(nullptr, 0);
    char large_send[HAMSTER_UN_MAX_QUEUED];
    for (size_t i = 0; i < sizeof(large_send); ++i) large_send[i] = (char)(i % 256);
    IOVec large_iov;
    large_iov.data = &large_send;
    large_iov.size = sizeof(large_send);
    assert(client4->sendto(&large_iov, 1, 0, nullptr, 0) == (ssize_t)large_iov.size);
    char large_recv[HAMSTER_UN_MAX_QUEUED];
    large_iov.data = &large_recv;
    assert(accepted4->recvfrom(&large_iov, 1, 0, nullptr, 0) == (ssize_t)large_iov.size);
    assert(memcmp(large_send, large_recv, sizeof(large_send)) == 0);
    dealloc(client4);
    dealloc(accepted4);

    dealloc(dgram1);
    dealloc(dgram2);
    dealloc(client);
    dealloc(accepted);
    dealloc(sock);
    assert(vfs.unmount("/") == 0);
}

#endif // NDEBUG
