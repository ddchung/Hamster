#include <network/network_manager.hpp>
#include <errno/errno.h>

namespace Hamster
{
    int NetworkManager::register_network(int type, BaseNetwork *network)
    {
        auto [_, ok] = networks.emplace(type, network);
        if (!ok)
        {
            error = H_EEXIST;
            return -1;
        }
        return 0;
    }

    BaseSocket *NetworkManager::socket(int domain, int type, int protocol)
    {
        auto it = networks.find(domain);
        if (it == networks.end())
        {
            error = H_EAFNOSUPPORT;
            return nullptr;
        }
        return it->second->socket(type, protocol);
    }
} // namespace Hamster
