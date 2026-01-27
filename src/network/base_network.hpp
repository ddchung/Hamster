
#pragma once

#include <network/base_socket.hpp>

namespace Hamster
{
    class BaseNetwork
    {
    public:
        virtual ~BaseNetwork() = default;
        BaseNetwork() = default;
        BaseNetwork(BaseNetwork &&) = delete;

        /**
         * @brief Open a socket for this network
         * @param type The type of the socket, `SOCK_STREAM` or `SOCK_DGRAM` for now.
         * @param protocol The protocol of the socket, 0 for default
         * @return A new socket, or nullptr on error and set `error`
         */
        virtual BaseSocket *socket(int type, int protocol) = 0;
    };
} // namespace Hamster
