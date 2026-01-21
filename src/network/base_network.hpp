// Hamster base network layer
// IPv4, UNIX, etc.

#pragma once

#include <abi/structs.hpp>
#include <abi/values.hpp>
#include <cstdint>
#include <cstddef>
#include <sys/types.h>

namespace Hamster
{
    class BaseNetwork
    {
    public:
        virtual ~BaseNetwork() = default;
        BaseNetwork() = default;
        BaseNetwork(BaseNetwork &&) = delete;

        /**
         * @brief Send data through
         * @param buf The buffers to send
         * @param count The number of buffers
         * @param flags Flags for sending
         * @param addr The address to send to, or nullptr
         * @param addrlen The length of the address, or 0
         * @return Number of bytes sent, or -1 on error and set `error`
         */
        virtual ssize_t sendto(const sys_iovec *buf, size_t count, int flags, const sys_sockaddr *addr, sys_socklen_t addrlen) = 0;

        // TODO: proper read behavior on zero-size datagram
        /**
         * @brief Recieve data
         * @param buf The buffers to recieve into
         * @param count The number of buffers
         * @param flags Flags for recieving
         * @param addr The address to recieve from, or nullptr
         * @param addrlen The length of the address, or 0
         * @return Number of bytes recieved, or -1 on error and set `error`
         */
        virtual ssize_t recvfrom(const sys_iovec *buf, size_t count, int flags, sys_sockaddr *addr, sys_socklen_t *addrlen) = 0;

        /**
         * @brief Bind to a local address
         * @param addr The address to bind to
         * @param addrlen The length of the address
         * @return 0 on success, or -1 on error and set `error`
         */
        virtual int bind(const sys_sockaddr *addr, sys_socklen_t addrlen) = 0;

        /**
         * @brief Connect to a remote address
         * @param addr The address to connect to
         * @param addrlen The length of the address
         * @return 0 on success, or -1 on error and set `error`
         * @note Please return immediately always. Error with `H_EINPROGRESS` if the connection is in progress
         */
        virtual int connect(const sys_sockaddr *addr, sys_socklen_t addrlen) = 0;

        /**
         * @brief Listen for incoming connections
         * @param backlog The maximum length of the pending connections queue
         * @return 0 on success, or -1 on error and set `error`
         */
        virtual int listen(int backlog) = 0;

        /**
         * @brief Accept an incoming connection
         * @param addr The address to store the remote address, or nullptr
         * @param addrlen The length of the address, or nullptr
         * @return A new BaseNetwork for the connection, or nullptr on error and set `error`
         * @note If no incoming connection is present, error with `H_EAGAIN`
         */
        virtual BaseNetwork *accept(sys_sockaddr *addr, sys_socklen_t *addrlen) = 0;

        /**
         * @brief Check for read/write availability
         * @param ops Bitmask of `POLL_READ` and `POLL_WRITE`
         * @return 1 if ready for all requested operations, 0 if not ready for any of them, -1 on error and set `error`
         */
        virtual int poll(int ops) = 0;
    };
} // namespace Hamster

