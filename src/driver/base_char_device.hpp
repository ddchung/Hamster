// Character device helper, for simpler character devices
// Note that if you want more functionality, please make a subclass of
// the character device classes in `filesystem/base_file.hpp`
//

#pragma once

#include <filesystem/base_file.hpp>
#include <memory/allocator.hpp>
#include <errno/errno.h>
#include <cstdint>

namespace Hamster
{
    // Specialize individual functions in this class, to implement
    // your character device
    // If you don't specialize a function, it will have a default implementation

    /*
    Example:

    // This struct is used only as a type identifier
    struct MyDevice {};

    template <>
    ssize_t CharacterDeviceImpl<MyDevice>::write(const void *buffer, size_t size)
    {
        // Implement your device-specific write logic here
    }

    template <>
    ssize_t CharacterDeviceImpl<MyDevice>::read(void *buffer, size_t size)
    {
        // Implement your device-specific read logic here
    }

    // replace {123, 456} with your device's major and minor numbers
    device_manager.register_device({123, 456}, alloc<CharacterDevice<MyDevice>>());

    */

    template <class>
    class CharacterDeviceImpl
    {
    public:
        static ssize_t write(const void *buffer, size_t size);
        static ssize_t read(void *buffer, size_t size);
        static int ioctl(int request, IoctlArg arg);
        static int poll(int op);
    };

    template <class>
    class CharacterDevice : public BaseSpecialDriver
    {
        class Handle : public BaseCharacterDeviceHandle
        {
        public:
            Handle(int flags);
            Handle(const Handle&) = delete;
            Handle& operator=(const Handle&) = delete;
            Handle(Handle&&) = default;
            Handle& operator=(Handle&&) = default;
            ~Handle() = default;

            ssize_t write(const uint8_t *buffer, size_t size) override;
            ssize_t read(uint8_t *buffer, size_t size) override;
            int get_flags() override;
            int set_flags(int flags) override;
            int ioctl(int request, IoctlArg arg) override;
            int64_t seek(int64_t offset, int whence) override;
            int64_t tell() override;
            int poll(int op) override;
        
        private:
            int flags;
        };
    public:
        BaseSpecialDriverHandle *create_handle(int flags);
    };

    // Defaults

    template <class T>
    ssize_t CharacterDeviceImpl<T>::write(const void *buffer, size_t size)
    {
        error = H_EIO;
        return -1;
    }

    template <class T>
    ssize_t CharacterDeviceImpl<T>::read(void *buffer, size_t size)
    {
        error = H_EIO;
        return -1;
    }

    template <class T>
    int CharacterDeviceImpl<T>::ioctl(int request, IoctlArg arg)
    {
        error = H_ENOTTY;
        return -1;
    }

    template <class T>
    int CharacterDeviceImpl<T>::poll(int op)
    {
        // Always ready
        return 1;
    }

    template <class T>
    CharacterDevice<T>::Handle::Handle(int flags) : flags(flags) {}

    template <class T>
    ssize_t CharacterDevice<T>::Handle::write(const uint8_t *buffer, size_t size)
    {
        if ((flags & OPEN_ACCMODE) == OPEN_RDONLY)
        {
            error = H_EACCES;
            return -1;
        }
        return CharacterDeviceImpl<T>::write(buffer, size);
    }

    template <class T>
    ssize_t CharacterDevice<T>::Handle::read(uint8_t *buffer, size_t size)
    {
        if ((flags & OPEN_ACCMODE) == OPEN_WRONLY)
        {
            error = H_EACCES;
            return -1;
        }
        return CharacterDeviceImpl<T>::read(buffer, size);
    }

    template <class T>
    int CharacterDevice<T>::Handle::get_flags()
    {
        return flags;
    }

    template <class T>
    int CharacterDevice<T>::Handle::set_flags(int flags)
    {
        this->flags = flags;
        return 0;
    }

    template <class T>
    int CharacterDevice<T>::Handle::ioctl(int request, IoctlArg arg)
    {
        return CharacterDeviceImpl<T>::ioctl(request, arg);
    }

    template <class T>
    int64_t CharacterDevice<T>::Handle::seek(int64_t offset, int whence)
    {
        error = H_ESPIPE;
        return -1;
    }

    template <class T>
    int64_t CharacterDevice<T>::Handle::tell()
    {
        error = H_ESPIPE;
        return -1;
    }

    template <class T>
    int CharacterDevice<T>::Handle::poll(int op)
    {
        return CharacterDeviceImpl<T>::poll(op);
    }

    template <class T>
    BaseSpecialDriverHandle *CharacterDevice<T>::create_handle(int flags)
    {
        return alloc<Handle>(1, flags);
    }
} // namespace Hamster

