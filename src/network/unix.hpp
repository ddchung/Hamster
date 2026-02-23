// Hamster Unix sockets

#pragma once

#include <network/base_socket.hpp>
#include <network/base_network.hpp>
#include <filesystem/base_file.hpp>
#include <memory/stl_sequential.hpp>
#include <abi/structs.hpp>
#include <abi/values.hpp>
#include <cstdint>
#include <cstddef>

namespace Hamster
{
    /**
     * @brief Generate a sockaddr_un from a pathname
     * @param addr The structure to fill
     * @param size The size of the buffer
     * @param path The pathname
     * @return The number of bytes that would have been filled in
     */
    size_t unix_make_sockaddr(sys_sockaddr_un *addr, size_t size, const String &path);

    /**
     * @brief Get a name from a sockaddr_un
     * @param addr The address
     * @param size The size of the address
     * @param path The destination string
     * @return The length of the path on success, or -1 on error
     * @note Fails on empty path
     */
    ssize_t unix_read_sockaddr(const sys_sockaddr_un *addr, size_t size, String &path);

    enum class UnixType : uint8_t
    {
        STREAM,
        DGRAM,
    };

    class BaseUnixSocketHandle : public BaseSpecialDriverHandle
    {
    public:
        SpecialFileType special_type() override { return SpecialFileType::Socket; }
        virtual UnixType unix_type() const = 0;
        virtual bool is_listening() = 0;
        virtual void set_listening(bool) = 0;

        // These don't make sense here, since Unix socket handles
        // won't be used anywhere outside of Unix sockets
        ssize_t write(const uint8_t *buf, size_t size) override;
        ssize_t read(uint8_t *buf, size_t size) override;
        int get_flags() override { return flags; }
        int set_flags(int flags) override { this->flags = flags; return 0; }
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

        // remove: whether to remove us from the node queue
        void cancel_connect(bool remove = true);

    private:
        State state;

        // Hold onto the BaseSpecialFile, not just the socket node handle,
        // for the reference counting of the node
        BaseSpecialFile *node;
        Deque<char> recv_buf;
        String peername, name;
        UnixStreamSocket *peer;
    };

    class UnixStreamNodeHandle : public BaseUnixSocketHandle
    {
    public:
        UnixStreamNodeHandle(int flags, class UnixStreamNode *node);
        UnixType unix_type() const override { return UnixType::STREAM; }
        bool is_listening() override;
        void set_listening(bool listening) override;

        UnixStreamNodeHandle *clone() override;

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

        /**
         * @brief Pop a client from the queue
         * @return The client, or nullptr on error and set `error`
         */
        UnixStreamSocket *accept();

    private:
        class UnixStreamNode *node;
    };

    class UnixStreamNode : public BaseSpecialDriver
    {
    public:
        UnixStreamNode();
        ~UnixStreamNode() override = default;
        UnixStreamNodeHandle *create_handle(int flags) override;

    private:
        friend class UnixStreamNodeHandle;
        Deque<UnixStreamSocket *> client_queue;
        bool is_listening;
    };

    
    class UnixDgramSocket : public BaseSocket
    {
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
        BaseSpecialFile *node;
        BaseSpecialFile *default_peer;
        String name, peer_filter;
    };

    class UnixDgramNodeHandle : public BaseUnixSocketHandle
    {
    public:
        UnixDgramNodeHandle(int flags, class UnixDgramNode *node);
        UnixDgramNodeHandle *clone() override;
        UnixType unix_type() const override { return UnixType::DGRAM; }
        bool is_listening() override;
        void set_listening(bool) override;

        /**
         * @brief Queue up a message
         * @param iov The IO Vector to send
         * @param iovlen The number of iovec structs
         * @param sender The sender of the data
         * @return The number of bytes actually queued, or -1 on error and set `error`
         */
        ssize_t push_data(const IOVec *iov, size_t iovlen, const String &sender);
        

        /**
         * @brief Dequeue a message
         * @param iov The buffer to read into
         * @param iovlen The amount of buffers in `iov`
         * @param sender A string to be filled with the sending address
         * @param filter Filter out messages from other sources than this address. Disabled if empty
         * @return The length of the datagram (that would have been read), or -1 on error and set `error`
         */
        ssize_t pop_data(const IOVec *iov, size_t iovlen, String &sender, const String &filter);

        /**
         * @brief Check if there is any pending message
         * @return true if there is, false if there isn't
         */
        bool has_data();
    
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
        UnixDgramNode();
        ~UnixDgramNode() = default;
        UnixDgramNodeHandle *create_handle(int flags) override;    

    private:
        friend class UnixDgramNodeHandle;
        Deque<Message> messages;
        size_t queued_size;
        bool is_listening;
    };

    class UnixNetwork : public BaseNetwork
    {
    public:
        BaseSocket *socket(int type, int protocol);
    };
} // namespace Hamster

