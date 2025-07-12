#include <filesystem/vfs_special.hpp>
#include <memory/allocator.hpp>
#include <errno/errno.h>

namespace Hamster
{

    SpecialDriverManager::SpecialDriverManager() = default;
    SpecialDriverManager::SpecialDriverManager(SpecialDriverManager &&other)
        : drivers(std::move(other.drivers))
    {
        other.drivers = Vector<BaseSpecialDriver *>();
    }
    SpecialDriverManager &SpecialDriverManager::operator=(SpecialDriverManager &&other)
    {
        if (this == &other)
            return *this;
        std::swap(drivers, other.drivers);
        return *this;
    }
    SpecialDriverManager::~SpecialDriverManager() { remove_all_drivers(); }

    int SpecialDriverManager::remove_all_drivers()
    {
        for (BaseSpecialDriver *driver : drivers)
        {
            dealloc(driver);
        }
        drivers.clear();
        return 0;
    }

    int SpecialDriverManager::add_driver(BaseSpecialDriver *driver)
    {
        if (!driver)
        {
            error = EINVAL;
            return -1;
        }
        int id = -1;
        for (int i = 0; i < (int)drivers.size(); ++i)
        {
            if (drivers[i] == nullptr)
            {
                id = i;
                break;
            }
        }
        if (id == -1)
        {
            id = drivers.size();
            drivers.push_back(nullptr);
        }
        drivers[id] = driver;
        return id;
    }

    int SpecialDriverManager::remove_driver(int id)
    {
        if (id < 0 || id >= (int)drivers.size())
        {
            error = EINVAL;
            return -1;
        }
        if (drivers[id] == nullptr)
        {
            error = EINVAL;
            return -1;
        }
        dealloc(drivers[id]);
        drivers[id] = nullptr;
        return 0;
    }

    BaseSpecialDriver *SpecialDriverManager::get_driver(int id)
    {
        if (id < 0 || id >= (int)drivers.size())
        {
            error = EINVAL;
            return nullptr;
        }
        if (drivers[id] == nullptr)
        {
            error = EINVAL;
            return nullptr;
        }
        return drivers[id];
    }

} // namespace Hamster
