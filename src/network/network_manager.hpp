// manager of different network types

#pragma once

#include <network/base_network.hpp>
#include <memory/unique_ptr.hpp>
#include <memory/stl_map.hpp>

namespace Hamster
{
    class NetworkManager
    {
    public:
        /**
         * @brief Register a network
         * @param type The integral type of the network (e.g. AF_UNIX)
         * @param network The actual network. Takes ownership.
         * @return 0 on success, -1 on error and set `error`
         */
        int register_network(int type, BaseNetwork *network);

        /**
         * @brief Make a socket from a network
         * @param domain The network type
         * @param type Either SOCK_STREAM or SOCK_DGRAM
         * @param protocol The protocol to use, or 0 for default
         * @return The socket, or nullptr on error and set `error`
         */
        BaseSocket *socket(int domain, int type, int protocol);

    private:
        Map<int, UniquePtr<BaseNetwork>> networks; // type -> network
    };

    extern NetworkManager network_manager;
} // namespace Hamster

