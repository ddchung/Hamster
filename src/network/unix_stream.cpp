
#include <network/unix.hpp>
#include <memory/allocator.hpp>
#include <process/task.hpp>
#include <errno/errno.h>
#include <cassert>
#include <algorithm>

namespace Hamster
{
    namespace
    {
        UnixStreamNodeHandle *get_handle(BaseSpecialFile *file)
        {
            assert(file);
            BaseSpecialDriverHandle *handle = file->get_handle();
            if (handle->special_type() != SpecialFileType::Socket)
            {
                error = H_ENOTSOCK;
                return nullptr;
            }
            if (((BaseUnixSocketHandle *)handle)->unix_type() != UnixType::STREAM)
            {
                error = H_ENOTSUP;
                return nullptr;
            }
            return (UnixStreamNodeHandle *)handle;
        }

        // Returns a newly allocated handle
        BaseSpecialFile *open_socket(const char *path)
        {
            int fd;

            Task *current_task = Task::get_current_task();
            if (current_task)
                fd = current_task->open_rel_file(H_AT_FDCWD, path, OPEN_RDONLY);
            else
                fd = vfs.open(path, OPEN_RDONLY);
            
            if (fd < 0)
                return nullptr;
            BaseFile *file = vfs.get_file(fd);
            vfs.close(fd);

            if (file->type() != FileType::Special || !get_handle((BaseSpecialFile *)file))
            {
                dealloc(file);
                return nullptr;
            }
            return (BaseSpecialFile *)file;
        }

        int make_socket(const char *path)
        {
            Task *current_task = Task::get_current_task();
            if (current_task)
            {
                int rel_fd = current_task->open_rel_fd(H_AT_FDCWD, path);
                if (rel_fd < 0)
                    return -1;
                int res = vfs.mknod(path, alloc<UnixStreamNode>(), current_task->mask_mode(0777));
                vfs.close(rel_fd);
                return res;
            }
            else
            {
                return vfs.mknod(path, alloc<UnixStreamNode>(), 0777);
            }
        }
    } // namespace

    UnixStreamSocket::UnixStreamSocket()
        : state(State::NONE), node(nullptr), peer(nullptr)
    {
    }

    UnixStreamSocket::~UnixStreamSocket()
    {
        if (state == State::CONNECTING)
        {
            // Remove us from queue
            assert(node);
            UnixStreamNodeHandle *handle = get_handle(node);
            assert(handle);
            int res = handle->cancel_connect(this);
            (void)res;
            assert(res == 0);
        }
        if (state == State::LISTENING)
        {
            // Mark socket as dead
            assert(node);
            UnixStreamNodeHandle *handle = get_handle(node);
            assert(handle);
            handle->set_listening(false);
        }

        dealloc(node);
        node = nullptr;
        peer = nullptr;
    }

    ssize_t UnixStreamSocket::sendto(const IOVec *buf, size_t count, int flags, const sys_sockaddr *addr, sys_socklen_t addrlen)
    {
        if (state != State::CONNECTED)
        {
            error = H_ENOTCONN;
            return -1;
        }

        if (addr != nullptr || addrlen != 0)
        {
            error = H_EISCONN;
            return -1;
        }

        // No flags are supported at this layer
        if (flags != 0)
        {
            error = H_ENOTSUP;
            return -1;
        }

        assert(peer != nullptr);

        size_t size = ioveclen(buf, count);
        if (size > INT32_MAX)
        {
            error = H_EINVAL;
            return -1;
        }

        if (peer->recv_buf.size() >= HAMSTER_UN_MAX_QUEUED)
        {
            // Block until space available
            error = H_EAGAIN;
            return -1;
        }
        size_t to_write = std::min(HAMSTER_UN_MAX_QUEUED - peer->recv_buf.size(), size);
        size_t remaining = to_write;

        for (size_t i = 0; i < count && remaining > 0; ++i)
        {
            const uint8_t *data = (const uint8_t *)buf[i].data;
            size_t s = std::min(buf[i].size, remaining);
            peer->recv_buf.insert(peer->recv_buf.end(), data, data + s);
            remaining -= s;
        }

        return size;
    }

    ssize_t UnixStreamSocket::recvfrom(const IOVec *buf, size_t count, int flags, sys_sockaddr *addr, sys_socklen_t *addrlen)
    {
        if ((bool)addr != (bool)addrlen)
        {
            error = H_EINVAL;
            return -1;
        }

        if ((flags & ~(H_MSG_WAITALL | H_MSG_PEEK)) != 0)
        {
            error = H_ENOTSUP;
            return -1;
        }

        size_t size = ioveclen(buf, count);
        if (size > INT32_MAX)
        {
            error = H_EINVAL;
            return -1;
        }

        if ((flags & H_MSG_WAITALL) && recv_buf.size() < size)
        {
            // Block until we have enough bytes
            error = H_EAGAIN;
            return -1;
        }

        size_t to_read = std::min(recv_buf.size(), size);
        size_t remaining = to_read;
        auto it = recv_buf.begin();

        for (size_t i = 0; i < count && remaining > 0; ++i)
        {
            uint8_t *data = (uint8_t *)buf[i].data;
            size_t s = std::min(remaining, buf[i].size);
            std::copy_n(it, s, data);
            remaining -= s;
            it += s;
        }

        assert(it - recv_buf.begin() == to_read);
        assert(remaining == 0);

        if ((flags & H_MSG_PEEK) == 0)
        {
            // pop only if not peeking
            recv_buf.erase(recv_buf.begin(), recv_buf.begin() + to_read);
        }

        return to_read;
    }

    int UnixStreamSocket::bind(const sys_sockaddr *addr, sys_socklen_t addrlen)
    {
        if (node != nullptr || state != State::NONE)
        {
            error = H_EINVAL;
            return -1;
        }

        String path;
        if (unix_read_sockaddr((sys_sockaddr_un *)addr, addrlen, path) < 0)
            return -1;
        
        BaseSpecialFile *file;
        if (file = open_socket(path.c_str()))
        {
            // Already exists
            dealloc(file);
            error = H_EADDRINUSE;
            return -1;
        }

        if (make_socket(path.c_str()) < 0)
            return -1;
        file = open_socket(path.c_str());
        assert(file != nullptr);

        node = file;
        name = std::move(path);
        return 0;
    }

    int UnixStreamSocket::connect(const sys_sockaddr *addr, sys_socklen_t addrlen)
    {
        if (state != State::NONE)
        {
            error = H_EISCONN;
            return -1;
        }

        String path;
        if (unix_read_sockaddr((sys_sockaddr_un *)addr, addrlen, path) < 0)
            return -1;

        BaseSpecialFile *file = open_socket(path.c_str());
        if (!file)
            return -1;
        
        if (node != nullptr)
        {
            // bind() was called previously
            dealloc(node);
            node = nullptr;
        }

        UnixStreamNodeHandle *handle = get_handle(node);
        assert(handle);
        if (handle->connect(this) < 0)
        {
            dealloc(node);
            return -1;
        }

        node = file;
        state = State::CONNECTING;
        return 0;
    }

    int UnixStreamSocket::listen(int backlog)
    {
        if (state != State::NONE || !node)
        {
            error = H_EINVAL;
            return -1;
        }

        UnixStreamNodeHandle *handle = get_handle(node);
        assert(handle);

        if (handle->is_listening())
        {
            error = H_EADDRINUSE;
            return -1;
        }

        handle->set_listening(true);
        state = State::LISTENING;
        return 0;
    }

    BaseSocket *UnixStreamSocket::accept(sys_sockaddr *addr, sys_socklen_t *addrlen)
    {
        if ((bool)addr != (bool)addrlen ||  state != State::LISTENING)
        {
            error = H_EINVAL;
            return nullptr;
        }

        assert(node);
        UnixStreamNodeHandle *handle = get_handle(node);
        assert (handle);

        UnixStreamSocket *client = handle->accept();
        if (!client)
            return nullptr;
        
        if (client->state != State::CONNECTING)
        {
            error = H_EISCONN;
            return nullptr;
        }
        
        UnixStreamSocket *new_sock = alloc<UnixStreamSocket>();
        
        // Set state and connect eachother
        client->state = State::CONNECTED;
        new_sock->state = State::CONNECTED;
        client->peer = new_sock;
        new_sock->peer = client;
        client->peername = new_sock->name;
        new_sock->peername = client->name;

        // clean up node handle, not needed anymore
        dealloc(client->node);
        client->node = nullptr;
        
        if (addr)
            *addrlen = unix_make_sockaddr((sys_sockaddr_un *)addr, *addrlen, client->name);
        
        return new_sock;
    }

    int UnixStreamSocket::poll(int ops)
    {
        // asking for read but nothing
        if ((ops & POLL_READ) && recv_buf.size() == 0)
            return 0;
        
        // asking for write and disconnected/full
        if ((ops & POLL_WRITE) && (peer == nullptr || peer->recv_buf.size() >= HAMSTER_UN_MAX_QUEUED))
            return 0;
        return 1;
    }
} // namespace Hamster

