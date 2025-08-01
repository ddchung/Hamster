// Hamster file

#pragma once

#include <abi/structs.hpp>
#include <abi/values.hpp>
#include <cstdint>
#include <cstddef>

// ssize_t
#include <unistd.h>

namespace Hamster
{
    enum class FileType : uint8_t
    {
        Regular,
        Directory,
        Symlink,

        // Special files are just tags that are used by the kernel to identify the file type
        // and occupy little to no space on disk
        Special,
    };

    struct DeviceID
    {
        uint32_t major;
        uint32_t minor;

        bool operator==(const DeviceID &other) const = default;
    };

    class BaseFilesystem;

    class BaseFile
    {
    public:
        virtual ~BaseFile() = default;

        /**
         * @brief Get the type of the file.
         * @return The type of the file
         */
        virtual FileType type() const = 0;

        /**
         * @brief Replicate this file
         * @return A newly allocated file object, or nullptr on error
         */
        virtual BaseFile *clone() = 0;

        /**
         * @brief Get the owning filesystem
         * @return A weak pointer to the owning filesystem, or nullptr on error
         */
        virtual BaseFilesystem *get_filesystem() = 0;

        /**
         * @brief Get an integer identifier for the file, valid on the same filesystem, for the lifetime of the file.
         * @return An integer identifier for the file, or -1 on error
         * @note This can be any number, just as long as it's different for each file on the same filesystem.
         * @note This is used by the VFS to identify files, and it will break if this doesn't return a unique value for each file.
         * @note This can, but is *not* requried to be implemented by returning the inode number
         */
        virtual int get_id() const = 0;

        /**
         * @brief Stat the file.
         * @param buf The buffer to fill with the file information
         * @return 0 on success, or on error return -1 and set `error`
         * @note Only set those fields that are described in POSIX `sys/stat.h`
         */
        virtual int stat(sys_stat *buf) = 0;

        /**
         * @brief Get the mode of the file.
         * @return The mode of the file, or on error return -1 and set `error`
         */
        virtual int get_mode() = 0;

        /**
         * @brief Get the open flags of the file.
         * @return The open flags of the file, or on error return -1 and set `error`
         * @note This is not the same as the mode, and is not set in `stat`
         * @note This should return the flags that were used to open the file
         */
        virtual int get_flags() = 0;

        /**
         * @brief Get the user id of the file.
         * @return The user id of the file, or on error return -1 and set `error`
         */
        virtual int get_uid() = 0;

        /**
         * @brief Get the group id of the file.
         * @return The group id of the file, or on error return -1 and set `error`
         */
        virtual int get_gid() = 0;

        /**
         * @brief Change the mode of the file.
         * @param mode The new mode
         * @return 0 on success, or on error return -1 and set `error`
         * @note Equivelant to POSIX `chmod`
         */
        virtual int chmod(int mode) = 0;

        /**
         * @brief Change the ownership of the file.
         * @param uid The new user id
         * @param gid The new group id
         * @return 0 on success, or on error return -1 and set `error`
         * @note Equivelant to POSIX `chown`
         */
        virtual int chown(int uid, int gid) = 0;
        
        /**
         * @brief Change the flags that the file was opened with.
         * @param flags The new flags
         * @return 0 on success, or on error return -1 and set `error`
         */
        virtual int set_flags(int flags) = 0;
    };

    class BaseRegularFile : public BaseFile
    {
    public:
        virtual FileType type() const override { return FileType::Regular; }

        /**
         * @brief Read from the file.
         * @param buf The buffer to read into
         * @param size The size of the buffer
         * @return The number of bytes read, or on error return -1 and set `error`
         * @note Equivelant to POSIX `read`
         */
        virtual ssize_t read(uint8_t *buf, size_t size) = 0;

        /**
         * @brief Write to the file.
         * @param buf The buffer to write
         * @param size The size of the buffer
         * @return The number of bytes written, or on error return -1 and set `error`
         * @note Equivelant to POSIX `write`
         */
        virtual ssize_t write(const uint8_t *buf, size_t size) = 0;

        /**
         * @brief Seek to a given position in the file.
         * @param offset The offset to seek to
         * @param whence The reference point for the offset
         * @return The new position in the file, or on error return -1 and set `error`
         * @note Equivelant to POSIX `lseek`
         */
        virtual int64_t seek(int64_t offset, int whence) = 0;

        /**
         * @brief Get the current position in the file.
         * @return The current position in the file
         */
        virtual int64_t tell() = 0;

        /**
         * @brief Truncate the file to a given size.
         * @param size The size to truncate the file to
         * @return 0 on success, or on error return -1 and set `error`
         */
        virtual int truncate(int64_t size) = 0;

        /**
         * @brief Get the size of the file.
         * @return The size of the file in bytes
         */
        virtual int64_t size() = 0;
    };

    class BaseSpecialDriverHandle;

    class BaseSpecialFile : public BaseFile
    {
    public:
        virtual FileType type() const override { return FileType::Special; }

        /**
         * @brief Get the Device ID of the special file.
         * @return The Device ID of the special file, or on error return {0, 0} and set `error`
         */
        virtual DeviceID get_device_id() = 0;

        // Don't declare `get_handle` and `set_handle` as virtual, so that base classes cannot override them

        /**
         * @brief A VFS hook to get the special file driver
         * @note Don't use this directly, as it is used by the VFS
         */
        inline BaseSpecialDriverHandle *get_handle() { return handle; }

        /**
         * @brief A VFS hook to set the special file driver
         * @note Don't use this directly, as it is used by the VFS
         */
        inline void set_handle(BaseSpecialDriverHandle *handle) { this->handle = handle; }
    
    private:
        // VFS hook
        BaseSpecialDriverHandle *handle = nullptr;
    };

    class BaseSymlink : public BaseFile
    {
    public:
        virtual FileType type() const override { return FileType::Symlink; }

        /**
         * @brief Read the target of the symlink.
         * @return A newly allocated string with the target of the symlink, or on error, it returns nullptr and sets `error`
         * @note Be sure to free the string
         */
        virtual char *get_target() = 0;

        /**
         * @brief Set the target of the symlink.
         * @param target The new target of the symlink
         * @return 0 on success, or on error return -1 and set `error`
         * @note `target` must be treated only like a string, and must not be validated in any way
         * @warning When implementing, please ensure to copy `target` to a new buffer to avoid dangling pointers
         */
        virtual int set_target(const char *target) = 0;
    };

    class BaseDirectory : public BaseFile
    {
    public:
        virtual FileType type() const override { return FileType::Directory; }

        /**
         * @brief List the files in the directory, starting from this directory's offset
         * @param count The number of entries to list, by default, it will list all entries
         * @return A newly allocated array of newly allocated strings, or on error, it returns nullptr and sets `error`
         * @note Be sure to free both dimensions
         * @note It may return an array with less than `count` entries, if there are not enough files in the directory
         */
        virtual char * const *list(size_t count = SIZE_MAX) = 0;

        /**
         * @brief Change the offset of the directory.
         * @param offset The new offset
         * @param whence One of H_SEEK_SET, H_SEEK_CUR, or H_SEEK_END
         * @return The new offset in the directory, or on error return -1 and set `error`
         * @note Equivelant to POSIX `lseek` on a directory
         */
        virtual int64_t seek(int64_t offset, int whence) = 0;

        /**
         * @brief Get a file in the directory.
         * @param name The name of the file
         * @param flags The flags to open the file with
         * @param mode Potential `mode`, if `flags | OPEN_CREAT`
         * @return A newly allocated `BaseFile` that operates on the opened file, or on error, it returns nullptr and sets `error`
         * @note Be sure to free the file
         * @note `name` is NOT a path, and cannot contain any slashes. It is relative to this directory.
         * @note If `flags | OPEN_CREAT && flags | OPEN_DIRECTORY`, then the file is created as a directory
         * @note A directory can with any of the three `OPEN_RDONLY`, `OPEN_WRONLY`, or `OPEN_RDWR` flags, which enables or disables some of these functions
         * @note Other than these, it is equivelant to POSIX `open`
         * @warning When implementing, you MUST ensure that if `flags | OPEN_DIRECTORY`, then the returned file derives from `BaseDirectory`
         */
        virtual BaseFile *get(const char *name, int flags, int mode = 0) = 0;

        /**
         * @brief Create a file in the directory.
         * @param name The name of the file
         * @param flags The flags to open the file with
         * @param mode The mode to create the file with
         * @return A newly allocated `BaseRegularFile` that operates on the new file, or on error, it returns nullptr and sets `error`
         * @note Be sure to free the file
         * @note `name` is NOT a path, and cannot contain any slashes. It is relative to this directory.
         * @note Equivelant to `this->get(name, (flags & ~OPEN_DIRECTORY) | OPEN_CREAT, mode)`
         */
        virtual BaseRegularFile *mkfile(const char *name, int flags, int mode) = 0;

        /**
         * @brief Create a directory in the directory.
         * @param name The name of the directory
         * @param flags The flags to open the directory with
         * @param mode The mode to create the directory with
         * @return A newly allocated `BaseDirectory` that operates on the new directory, or on error, it returns nullptr and sets `error`
         * @note Be sure to free the file
         * @note `name` is NOT a path, and cannot contain any slashes. It is relative to this directory.
         * @note Equivelant to `this->get(name, flags | OPEN_CREAT | OPEN_DIRECTORY, mode)`
         */
        virtual BaseDirectory *mkdir(const char *name, int flags, int mode) = 0;

        /**
         * @brief Create a symlink in the directory.
         * @param name The name of the symlink
         * @param target The target of the symlink
         * @return A newly allocated `BaseSymlink` that operates on the new symlink, or on error, it returns nullptr and sets `error`
         * @note Be sure to free the file
         * @note `name` is NOT a path, and cannot contain any slashes. It is relative to this directory.
         * @note `target` must be treated only like a string, and must not be validated in any way
         */
        virtual BaseSymlink *mksym(const char *name, const char *target) = 0;

        /**
         * @brief Make a special file in the directory.
         * @param name The name of the special file
         * @param flags The flags to open the special file with
         * @param id The ID of the special file
         * @param mode The mode to create the special file with
         * @return A newly allocated `BaseSpecialFile` that operates on the new special file, or on error, it returns nullptr and sets `error`
         * @note Be sure to free the file
         * @note `name` is NOT a path, and cannot contain any slashes. It is relative to this directory.
         * @note This is NOT equivelant to POSIX `mknod`, as this just makes a stub special file that can only be used to identify the file type
         */
        virtual BaseSpecialFile *mksfile(const char *name, int flags, DeviceID id, int mode) = 0;

        /**
         * @brief Create a hard-link to another file on this filesystem
         * @param file The target file
         * @param name The name of the hard link
         * @return 0 on success, -1 on error and set `error`
         * @note If the file is not part of this filesystem, set `errno` to `EXDEV`
         */
        virtual int link(BaseFile *file, const char *name) = 0;

        /**
         * @brief Remove a file in the directory.
         * @param name The name of the file
         * @return 0 on success, or on error return -1 and set `error`
         * @note `name` is NOT a path, and cannot contain any slashes. It is relative to this directory.
         * @warning NOT equivelant to POSIX `remove`, as this cannot traverse directories, but can only remove files directly present here
         */
        virtual int remove(const char *name) = 0;
    };

    class BaseFilesystem
    {
    public:
        virtual ~BaseFilesystem() = default;

        /**
         * @brief Get the root directory of the filesystem.
         * @return A newly allocated `BaseDirectory` that operates on the root directory, or on error, it returns nullptr and sets `error`
         * @note Be sure to free the directory
         * @note This is equivelant to POSIX `open` on the filesystem's base
         * @note The filesystem's base is not necessarily the root of Hamster's FS, due to mounts
         */
        virtual BaseDirectory *open_root(int flags) = 0;
    };

    /* Special File Hierarchy */
    // This is only used by the VFS to create special files, so that they can do stuff
    // Note that for now, it remains incomplete
    // Please note that there is exactly one special file driver for each special file, so
    // * it is perfectly OK to keep state in the driver, as it is not shared between multiple files
    // Also note that if you want to keep independent state between different file descriptors of the same file,
    // * you can put the state in the handle
    // TODO: complete

    enum class SpecialFileType : uint8_t
    {
        BlockDevice,
        CharacterDevice,
        Fifo,
        Socket,
    };

    struct IoctlArg
    {
        int i;
        void *p;
    };

    class BaseSpecialDriverHandle
    {
    public:
        virtual ~BaseSpecialDriverHandle() = default;

        /**
         * @brief Get the type of the special file.
         * @return The type of the special file
         */
        virtual SpecialFileType special_type() = 0;

        /**
         * @brief Write to the special file.
         * @param buf The buffer to write
         * @param size The size of the buffer
         * @return The number of bytes written, or on error return -1 and set `error`
         * @note Equivelant to POSIX `write`
         */
        virtual ssize_t write(const uint8_t *buf, size_t size) = 0;

        /**
         * @brief Read from the special file.
         * @param buf The buffer to read into
         * @param size The size of the buffer
         * @return The number of bytes read, or on error return -1 and set `error`
         * @note Equivelant to POSIX `read`
         */
        virtual ssize_t read(uint8_t *buf, size_t size) = 0;

        /**
         * @brief Get the flags that were used to create this handle.
         * @return The flags that were used to create this handle, or on error return -1 and set `error`
         */
        virtual int get_flags() = 0;

        /**
         * @brief Set this handle's flags.
         * @param flags The new flags
         * @return 0 on success, or on error return -1 and set `error`
         */
        virtual int set_flags(int flags) = 0;

        /**
         * @brief Perform an ioctl operation on the special file.
         * @param request The request to perform
         * @param arg An optional argument for the request, which can be an integer or a pointer, depending on the request
         * @return It depends on the request, but it is guaranteed to return -1 on error and set `error`, but otherwise
         *       * it depends.
         */
        virtual int ioctl(int request, IoctlArg arg = {}) = 0;
    };

    class BaseCharacterDeviceHandle : public BaseSpecialDriverHandle
    {
    public:
        virtual SpecialFileType special_type() override { return SpecialFileType::CharacterDevice; }

        /**
         * @brief Check if the device is a TTY
         * @return true if the device is a TTY, false otherwise
         */
        virtual bool is_tty() { return false; }
    };

    class BaseFifoHandle : public BaseSpecialDriverHandle
    {
    public:
        virtual SpecialFileType special_type() override { return SpecialFileType::Fifo; }
    };

    class BaseSocketDeviceHandle : public BaseSpecialDriverHandle
    {
    public:
        virtual SpecialFileType special_type() override { return SpecialFileType::Socket; }

        // TODO: Implement special socket operations
    };

    class BaseBlockDeviceHandle : public BaseSpecialDriverHandle
    {
    public:
        virtual SpecialFileType special_type() override { return SpecialFileType::BlockDevice; }

        /**
         * @brief Seek to a given position in the block device.
         * @param offset The offset to seek to
         * @param whence The reference point for the offset
         * @return The new position in the block device, or on error return -1 and set `error`
         * @note Equivelant to POSIX `lseek`
         */
        virtual int64_t seek(int64_t offset, int whence) = 0;

        /**
         * @brief Get the current position in the block device.
         * @return The current position in the block device
         */
        virtual int64_t tell() = 0;

        /**
         * @brief Get the size of the block device.
         * @return The size of the block device in bytes
         */
        virtual int64_t size() = 0;
    };

    class BaseSpecialDriver
    {
    public:
        virtual ~BaseSpecialDriver() = default;

        /**
         * @brief Create a new handle for the special file.
         * @param flags The flags to open the file with
         * @return A newly allocated `BaseSpecialDriverHandle` that operates on the special file, or on error, it returns nullptr and sets `error`
         * @note Be sure to free the handle
         */
        virtual BaseSpecialDriverHandle *create_handle(int flags) = 0;
    };
} // namespace Hamster

