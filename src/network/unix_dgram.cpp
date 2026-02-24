
#include <network/unix.hpp>
#include <process/task.hpp>
#include <errno/errno.h>
#include <memory/allocator.hpp>

namespace Hamster
{
    namespace
    {
        UnixDgramNodeHandle *get_handle(BaseSpecialFile *file)
        {
            assert(file);
            BaseSpecialDriverHandle *handle = file->get_handle();
            if (handle->special_type() != SpecialFileType::Socket)
            {
                error = H_ENOTSOCK;
                return nullptr;
            }
            if (((BaseUnixSocketHandle *)handle)->unix_type() != UnixType::DGRAM)
            {
                error = H_EPROTOTYPE;
                return nullptr;
            }
            return (UnixDgramNodeHandle *)handle;
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
                int res = vfs.mknod(path, alloc<UnixDgramNode>(), current_task->mask_mode(0777));
                vfs.close(rel_fd);
                return res;
            }
            else
            {
                return vfs.mknod(path, alloc<UnixDgramNode>(), 0777);
            }
        }
    } // namespace

    UnixDgramSocket::UnixDgramSocket()
        : node(), default_peer(), name()
    {
    }

    UnixDgramSocket::~UnixDgramSocket()
    {
        if (node)
            get_handle(node)->set_listening(false);

        dealloc(node);
        dealloc(default_peer);

        node = nullptr;
        default_peer = nullptr;
    }

    ssize_t UnixDgramSocket::sendto(const IOVec *buf, size_t count, int flags, const sys_sockaddr *addr, sys_socklen_t addrlen)
    {
        // TODO: MSG_MORE
        assert(buf);

        if ((bool)addr != (bool)addrlen)
        {
            error = H_EINVAL;
            return -1;
        }

        if (!addr && !default_peer)
        {
            error = H_EDESTADDRREQ;
            return -1;
        }

        BaseSpecialFile *target = nullptr;

        if (addr)
        {
            String peer;
            if (unix_read_sockaddr((const sys_sockaddr_un *)addr, addrlen, peer) < 0)
                return -1;
            target = open_socket(peer.c_str());
            if (!target)
                return -1;
        }
        else
        {
            target = default_peer;
        }

        UnixDgramNodeHandle *handle = get_handle(target);
        assert(handle);
        ssize_t res = handle->push_data(buf, count, name);

        if (addr)
            dealloc(target);
        target = nullptr;

        return res;
    }

    ssize_t UnixDgramSocket::recvfrom(const IOVec *buf, size_t count, int flags, sys_sockaddr *addr, sys_socklen_t *addrlen)
    {
        assert(buf);
        if ((bool)addr != (bool)addrlen)
        {
            error = H_EINVAL;
            return -1;
        }

        // TODO: MSG_PEEK
        if (flags & ~H_MSG_TRUNC)
        {
            error = H_ENOTSUP;
            return -1;
        }

        if (!node)
        {
            // Not bound yet
            error = H_EINVAL;
            return -1;
        }

        size_t nbytes = ioveclen(buf, count);
        if (nbytes > INT32_MAX)
        {
            error = H_EINVAL;
            return -1;
        }

        UnixDgramNodeHandle *handle = get_handle(node);
        assert(handle);

        String sender;
        ssize_t res = handle->pop_data(buf, count, sender, peer_filter);
        if (res < 0)
            return -1;
        
        if (addr)
            *addrlen = unix_make_sockaddr((sys_sockaddr_un *)addr, *addrlen, sender);
        
        if (flags & H_MSG_TRUNC)
            // return length of datagram instead
            return res;
        else
            return std::min<ssize_t>(nbytes, res);
    }

    int UnixDgramSocket::bind(const sys_sockaddr *addr, sys_socklen_t addrlen)
    {
        assert(addr);

        if (node != nullptr)
        {
            error = H_EINVAL;
            return -1;
        }

        String path;
        if (unix_read_sockaddr((sys_sockaddr_un *)addr, addrlen, path) < 0)
            return -1;
        
        BaseSpecialFile *file;
        if ((file = open_socket(path.c_str())))
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

    int UnixDgramSocket::connect(const sys_sockaddr *addr, sys_socklen_t addrlen)
    {
        assert(addr);
        String path;
        if (unix_read_sockaddr((sys_sockaddr_un *)addr, addrlen, path) < 0)
            return -1;
        
        BaseSpecialFile *sock = open_socket(path.c_str());
        if (!sock)
            return -1;

        dealloc(default_peer);
        default_peer = sock;
        peer_filter = std::move(path);
        return 0;
    }

    int UnixDgramSocket::listen(int backlog)
    {
        error = H_EOPNOTSUPP;
        return -1;
    }

    BaseSocket *UnixDgramSocket::accept(sys_sockaddr *addr, sys_socklen_t *addrlen)
    {
        error = H_EOPNOTSUPP;
        return nullptr;
    }

    int UnixDgramSocket::poll(int ops)
    {
        if ((ops & POLL_READ) && (!node || !get_handle(node)->has_data()))
            return 0;
        // TODO: write polling?
        return 1;
    }

    UnixDgramNodeHandle::UnixDgramNodeHandle(int flags, UnixDgramNode *node)
        : node(node)
    {
        this->flags = flags;
    }

    UnixDgramNodeHandle *UnixDgramNodeHandle::clone()
    {
        return alloc<UnixDgramNodeHandle>(1, flags, node);
    }

    bool UnixDgramNodeHandle::is_listening()
    {
        return node->is_listening;
    }

    void UnixDgramNodeHandle::set_listening(bool listening)
    {
        if (node->is_listening == listening)
            return;

        if (node->is_listening)
        {
            node->messages.clear();
            node->queued_size = 0;
        }
        node->is_listening = listening;
    }

    ssize_t UnixDgramNodeHandle::push_data(const IOVec *iov, size_t iovlen, const String &sender)
    {
        assert(iov);

        size_t nbytes = ioveclen(iov, iovlen);
        
        if (node->queued_size + nbytes > HAMSTER_UN_MAX_QUEUED)
        {
            error = H_EAGAIN;
            return -1;
        }

        UnixDgramNode::Message &msg = node->messages.emplace_back();
        msg.data.reserve(nbytes);
        
        for (size_t i = 0; i < iovlen; ++i)
            msg.data.insert(msg.data.end(), (const uint8_t *)iov[i].data, (const uint8_t *)iov[i].data + iov[i].size);
        
        msg.sender = sender;
        node->queued_size += nbytes;

        return nbytes;
    }

    ssize_t UnixDgramNodeHandle::pop_data(const IOVec *iov, size_t iovlen, String &sender, const String &filter)
    {
        assert(iov);

        if (!filter.empty())
        {
            while (node->messages.size() > 0 && node->messages.front().sender != filter)
                node->messages.pop_front();
        }

        if (node->messages.empty())
        {
            error = H_EAGAIN;
            return -1;
        }

        size_t requested = ioveclen(iov, iovlen);
        const auto &msg = node->messages.front();
        size_t size = msg.data.size();

        size_t to_read = std::min(size, requested);
        size_t remaining = to_read;
        for (size_t i = 0; i < iovlen && remaining > 0; ++i)
        {
            const IOVec &vec = iov[i];
            uint8_t *buf = (uint8_t *)vec.data;
            size_t nbytes = std::min(remaining, vec.size);
            memcpy(buf, msg.data.data() + to_read - remaining, nbytes);
            remaining -= nbytes;
        }

        sender = std::move(msg.sender);

        node->messages.pop_front();
        assert(node->queued_size >= size);
        node->queued_size -= size;

        return to_read;
    }

    bool UnixDgramNodeHandle::has_data()
    {
        return node->queued_size > 0;
    }

    UnixDgramNode::UnixDgramNode()
        : queued_size(0), is_listening(false)
    {
    }

    UnixDgramNodeHandle *UnixDgramNode::create_handle(int flags)
    {
        return alloc<UnixDgramNodeHandle>(1, flags, this);
    }
} // namespace Hamster

