// Hamster file

#pragma once

#include <sys/stat.h>
#include <cstdint>
#include <cstddef>

// expose some macros
#include <fcntl.h>

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

    enum class SpecialFileType : uint8_t
    {
        BlockDevice,
        CharDevice,
        Socket,
        Pipe,
        Fifo,
    };

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
         * @brief Rename this file
         * @param new_name The new name of the file
         * @return 0 on success, or on error return -1 and set `error`
         * @note Equivelant to POSIX `rename`
         */
        virtual int rename(const char *) = 0;

        /**
         * @brief Remove this file
         * @return 0 on success, or on error return -1 and set `error`
         * @note Equivelant to POSIX `unlink` on this file
         */
        virtual int remove() = 0;

        /**
         * @brief Stat the file.
         * @param buf The buffer to fill with the file information
         * @return 0 on success, or on error return -1 and set `error`
         * @note Only set those fields that are described in POSIX `sys/stat.h`
         */
        virtual int stat(struct ::stat *buf) = 0;

        /**
         * @brief Get the mode of the file.
         * @return The mode of the file, or on error return -1 and set `error`
         */
        virtual int get_mode() = 0;

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
         * @brief Get the name of the file.
         * @return A newly allocated string with the name of the file, or on error, it returns nullptr and sets `error`
         * @note Be sure to free the string
         */
        virtual char *get_name() = 0;
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

    class BaseSpecialFile : public BaseFile
    {
    public:
        virtual FileType type() const override { return FileType::Special; }

        /**
         * @brief Get the type of the special file.
         * @return The type of the special file
         */
        virtual SpecialFileType special_type() = 0;
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
         * @brief List the files in the directory.
         * @return A newly allocated array of newly allocated strings, or on error, it returns nullptr and sets `error`
         * @note Be sure to free both dimensions
         */
        virtual char * const *list() = 0;

        /**
         * @brief Get a file in the directory.
         * @param name The name of the file
         * @param flags The flags to open the file with
         * @param mode Potential `mode`, if `flags | O_CREAT`
         * @return A newly allocated `BaseFile` that operates on the opened file, or on error, it returns nullptr and sets `error`
         * @note Be sure to free the file
         * @note `name` is NOT a path, and cannot contain any slashes. It is relative to this directory.
         * @note If `flags | O_CREAT && flags | O_DIRECTORY`, then the file is created as a directory
         * @note Other than these, it is equivelant to POSIX `open`
         * @warning When implementing, you MUST ensure that if `flags | O_DIRECTORY`, then the returned file derives from `BaseDirectory`
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
         * @note Equivelant to `this->get(name, (flags & ~O_DIRECTORY) | O_CREAT, mode)`
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
         * @note Equivelant to `this->get(name, flags | O_CREAT | O_DIRECTORY, mode)`
         */
        virtual BaseDirectory *mkdir(const char *name, int flags, int mode) = 0;

        using BaseFile::remove;
        
        /**
         * @brief Remove a file in the directory.
         * @param name The name of the file
         * @return 0 on success, or on error return -1 and set `error`
         * @note `name` is NOT a path, and cannot contain any slashes. It is relative to this directory.
         * @note Equivelant to POSIX `remove`
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
} // namespace Hamster

