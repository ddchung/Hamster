// Hamster Unix sockets

#pragma once

#include <network/base_socket.hpp>
#include <filesystem/base_file.hpp>
#include <memory/stl_sequential.hpp>
#include <cstdint>
#include <cstddef>

namespace Hamster
{
    enum class UnixType : uint8_t
    {
        STREAM,
        DGRAM,
    };

    class BaseUnixSocketHandle : public BaseSpecialDriverHandle
    {
    public:
        virtual UnixType unix_type() const = 0;
        virtual bool is_listening() = 0;
        virtual void set_unlisten() = 0;

        // These don't make sense here, since Unix socket handles
        // won't be used anywhere outside of Unix sockets
        ssize_t write(const uint8_t *buf, size_t size) override;
        ssize_t read(uint8_t *buf, size_t size) override;
        int get_flags() override;
        int set_flags(int flags) override;
        int ioctl(int req, IoctlArg arg = IoctlArg()) override;

    protected:
        int flags;
    };

    class UnixStreamSocket : public BaseSocket
    {
        enum class State
        {
            NONE,
            CONNECTING,
            CONNECTED,
            LISTENING,
        };
    public:
        UnixStreamSocket();
        ~UnixStreamSocket() override;
        ssize_t sendto(const IOVec *buf, size_t count, int flags, const sys_sockaddr *addr, sys_socklen_t addrlen) override;
        ssize_t recvfrom(const IOVec *buf, size_t count, int flags, sys_sockaddr *addr, sys_socklen_t *addrlen) override;
        int bind(const sys_sockaddr *addr, sys_socklen_t addrlen) override;
        int connect(const sys_sockaddr *addr, sys_socklen_t addrlen) override;
        int listen(int backlog) override;
        BaseSocket *accept(sys_sockaddr *addr, sys_socklen_t *addrlen) override;
        int poll(int ops) override;    

    private:
        BaseSpecialFile *node;
        Deque<char> recv_buf;
        String peername, name;
        UnixStreamSocket *peer;
    };

    class UnixStreamNodeHandle : public BaseUnixSocketHandle
    {
    public:
        UnixType unix_type() const override { return UnixType::STREAM; }
        bool is_listening() override;
        void set_unlisten() override;

        /**
         * @brief Push a connector
         * @param client The client to connect
         * @return 0 on success, -1 on error
         */
        int connect(UnixStreamSocket *client);

        /**
         * @brief Cancel connecting
         * @param client The client to remove from the queue
         * @return 0 on success, -1 on error
         */
        int cancel_connect(UnixStreamSocket *client);

    private:
        class UnixStreamNode *node;
    };

    class UnixStreamNode : public BaseSpecialDriver
    {
    public:
        ~UnixStreamNode() override = default;
        UnixStreamNodeHandle *create_handle(int flags) override;

    private:
        Deque<UnixStreamSocket *> client_queue;
        bool is_listening;
    };

    
    class UnixDgramSocket : public BaseSocket
    {
        enum class State
        {
            NONE,
            LISTENING,
        };
    public:
        UnixDgramSocket();
        ~UnixDgramSocket() override;
        ssize_t sendto(const IOVec *buf, size_t count, int flags, const sys_sockaddr *addr, sys_socklen_t addrlen) override;
        ssize_t recvfrom(const IOVec *buf, size_t count, int flags, sys_sockaddr *addr, sys_socklen_t *addrlen) override;
        int bind(const sys_sockaddr *addr, sys_socklen_t addrlen) override;
        int connect(const sys_sockaddr *addr, sys_socklen_t addrlen) override;
        int listen(int backlog) override;
        BaseSocket *accept(sys_sockaddr *addr, sys_socklen_t *addrlen) override;
        int poll(int ops) override;

    private:
        State state;
        BaseSpecialFile *node;
        BaseSpecialFile *default_peer;
        String name;
    };

    class UnixDgramNodeHandle : public BaseUnixSocketHandle
    {
    public:
        UnixType unix_type() const override { return UnixType::DGRAM; }
        bool is_listening() override;
        void set_unlisten() override;

        /**
         * @brief Queue up a message
         * @param iov The IO Vector to send
         * @param iovlen The number of iovec structs
         * @return The number of bytes actually queued, or -1 on error and set `error`
         */
        ssize_t push_data(const IOVec *iov, size_t iovlen);
        // Same thing, but pop data into iovecs
        ssize_t pop_data(const IOVec *iov, size_t iovlen);
    
    private:
        class UnixDgramNode *node;
    };

    class UnixDgramNode : public BaseSpecialDriver
    {
        struct Message
        {
            Vector<uint8_t> data;
            String sender;
        };
    public:
        ~UnixDgramNode() = default;
        UnixDgramNodeHandle *create_handle(int flags) override;    

    private:
        Deque<Message> messages;
    };
} // namespace Hamster

