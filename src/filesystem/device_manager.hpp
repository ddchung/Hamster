// Hamster special file manager

#pragma once

#include <filesystem/base_file.hpp>
#include <memory/stl_map.hpp>
#include <cstdint>


template <>
struct std::hash<Hamster::DeviceID>
{
    std::size_t operator()(const Hamster::DeviceID &id) const noexcept
    {
        return std::hash<uint32_t>()(id.major) ^ (std::hash<uint32_t>()(id.minor) << 1);
    }
};

namespace Hamster
{
    class DeviceManager
    {
    public:
        DeviceManager() = default;
        ~DeviceManager();

        DeviceManager(const DeviceManager &) = delete;
        DeviceManager &operator=(const DeviceManager &) = delete;

        DeviceManager(DeviceManager &&);
        DeviceManager &operator=(DeviceManager &&);

        /**
         * @brief Create a new device driver
         * @param id The device ID of the driver
         * @param driver The driver of the id
         * @return 0 on success, or on error return -1 and set `error`
         * @note This takes ownership of the driver
         */
        int register_device(const DeviceID &id, BaseSpecialDriver *driver);

        /**
         * @brief Create a new handle for a device
         * @param id The device ID of the driver
         * @param flags The flags to open the file with
         * @return A newly allocated `BaseSpecialDriverHandle` that operates on the special file, or on error, it returns nullptr and sets `error`
         * @note Be sure to free the handle
         */
        BaseSpecialDriverHandle *create_handle(const DeviceID &id, int flags);
    private:
        UnorderedMap<DeviceID, BaseSpecialDriver *> drivers;
    };

    extern DeviceManager device_manager;
} // namespace Hamster

