// Hamster device manager

#include <filesystem/device_manager.hpp>
#include <memory/allocator.hpp>
#include <errno/errno.h>
#include <utility>

namespace Hamster
{
    DeviceManager::~DeviceManager()
    {
        for (auto &[_, dev] : drivers)
        {
            dealloc(dev);
        }

        drivers.clear();
    }

    DeviceManager::DeviceManager(DeviceManager &&other)
    {
        if (this != &other)
            std::swap(drivers, other.drivers);
    }

    DeviceManager &DeviceManager::operator=(DeviceManager &&other)
    {
        if (this != &other)
            std::swap(drivers, other.drivers);
        return *this;
    }

    int DeviceManager::register_device(const DeviceID &id, BaseSpecialDriver *driver)
    {
        if (drivers.contains(id))
        {
            error = H_EEXIST;
            return -1;
        }

        drivers[id] = driver;

        return 0;
    }

    BaseSpecialDriverHandle *DeviceManager::create_handle(const DeviceID &id, int flags)
    {
        auto it = drivers.find(id);
        if (it == drivers.end())
        {
            error = H_EBADF;
            return nullptr;
        }

        return it->second->create_handle(flags);
    }

    BaseSpecialDriver *DeviceManager::get_driver(const DeviceID &id)
    {
        auto it = drivers.find(id);
        if (it == drivers.end())
        {
            return nullptr;
        }

        return it->second;
    }
} // namespace Hamster


