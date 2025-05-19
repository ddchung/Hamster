// Hamster VFS

#pragma once

#include <filesystem/base_file.hpp>

namespace Hamster
{
    class VFSData;

    class VFS
    {
    public:
        // move-only
    
        VFS();
        ~VFS();
        VFS(const VFS &) = delete;
        VFS &operator=(const VFS &) = delete;
        VFS(VFS &&);
        VFS &operator=(VFS &&);    

        /**
         * @brief Mount a filesystem at a given path
         * @param path The path to mount the filesystem at
         * @param fs The filesystem to mount
         * @return 0 on success, or on error return -1 and set `error`
         * @note Takes ownership of `fs`, and will deallocate it later
         */
        int mount(const char *path, BaseFilesystem *fs);

        /**
         * @brief Unmount a filesystem at a given path
         * @param path The path to unmount the filesystem from
         * @return 0 on success, or on error return -1 and set `error`
         */
        int unmount(const char *path);

        /* POSIX and POSIX-like functions */
        /* These all return -1 and set `error` on error */

        /**
         * @brief Open a file at a given path
         * @param path The path to the file
         * @param flags The flags to open the file with
         * @param mode The mode to open the file with, if `flags & O_CREAT`
         * @return A file descriptor on success, or on error return -1 and set `error`
         */
        int open(const char *path, int flags, int mode = 0);

        /**
         * @brief Close a file descriptor
         * @param fd The file descriptor to close
         * @return 0 on success, or on error return -1 and set `error`
         * @note This frees all resources associated with the file descriptor
         */
        int close(int fd);

        /**
         * @brief Rename a file described by a file descriptor
         * @param fd The file descriptor to rename
         * @param new_name The new name of the file
         * @return 0 on success, or on error return -1 and set `error`
         * @warning This cannot move the file, but just change its name in-place
         */
        int rename(int fd, const char *new_name);

        /**
         * @brief Move a file
         * @param old_path The path to the file to move
         * @param new_path The path to move the file to
         * @return 0 on success, or on error return -1 and set `error`
         */
        int rename(const char *old_path, const char *new_path);

        /**
         * @brief Remove a file described by a file descriptor
         * @param fd The file descriptor to remove
         * @return 0 on success, or on error return -1 and set `error`
         * @warning This invalidates all file descriptors that point to the file
         * @warning This does not close the file descriptor, so you must do that yourself
         */
        int remove(int fd);

        /**
         * @brief Stat a file described by a file descriptor
         * @param fd The file descriptor to stat
         * @return 0 on success, or on error return -1 and set `error`
         * @note This cannot be used to stat a symlink, as a file descriptor cannot
         *       * point to a symlink
         */
        int stat(int fd, struct ::stat *buf);

        /**
         * @brief Stat a file at a given path
         * @param path The path to the file
         * @return 0 on success, or on error return -1 and set `error`
         * @note This can be used to stat a symlink, as it will not follow it
         */
        int lstat(const char *path, struct ::stat *buf);

        /**
         * Get some attributes of a file or its descriptor
         */

        int get_mode(int fd);

        int get_flags(int fd);

        int get_uid(int fd);

        int get_gid(int fd);

        /**
         * @brief Change the mode of a file described by a file descriptor
         * @param fd The file descriptor to change the mode of
         * @param mode The new mode of the file
         * @return 0 on success, or on error return -1 and set `error`
         * @note `mode` must only set the permission bits, not the file type
         */
        int chmod(int fd, int mode);

        /**
         * @brief Change the file ownership of a file described by a file descriptor
         * @param fd The file descriptor to change the ownership of
         * @param uid The new user ID of the file
         * @param gid The new group ID of the file
         * @return 0 on success, or on error return -1 and set `error`
         */
        int chown(int fd, int uid, int gid);

        /**
         * @brief Get the name (not path) of a file described by a file descriptor
         * @param fd The file descriptor to get the name of
         * @return The name of the file, or nullptr on error
         */
        char *basename(int fd);

        /**
         * @brief Read from a file
         * @param fd The file descriptor to read from
         * @param buf The buffer to read into
         * @param size The size of the buffer
         * @return The number of bytes read, or on error return -1 and set `error`
         */
        ssize_t read(int fd, void *buf, size_t size);

        /**
         * @brief Write to a file
         * @param fd The file descriptor to write to
         * @param buf The buffer to write from
         * @param size The size of the buffer
         * @return The number of bytes written, or on error return -1 and set `error`
         */
        ssize_t write(int fd, const void *buf, size_t size);

        /**
         * @brief Seek to a position in a file
         * @param fd The file descriptor to seek
         * @param offset The offset to seek to
         * @param whence One of `SEEK_SET`, `SEEK_CUR`, or `SEEK_END`
         * @return 0 on success, or on error return -1 and set `error`
         */
        int seek(int fd, int64_t offset, int whence);

        /**
         * @brief Get the current position in a file
         * @param fd The file descriptor to get the position of
         * @return The current position in the file, or on error return -1 and set `error`
         */
        int64_t tell(int fd);

        /**
         * @brief Change the size of a file
         * @param fd The file descriptor to change the size of
         * @param size The new size of the file
         * @return 0 on success, or on error return -1 and set `error`
         * @note This does not change the current position in the file
         * @note This can go both ways, so it can shorten or lengthen the file
         */
        int truncate(int fd, int64_t size);

        /**
         * @brief Get the size of a file
         * @param fd The file descriptor to get the size of
         * @return The size of the file, or on error return -1 and set `error`
         * @note This does not change the current position in the file
         */
        int64_t size(int fd);

        /**
         * @brief Get the target that a symlink points to
         * @param path The path to the symlink
         * @return A newly allocated string containing the target, or nullptr on error
         * @note Be sure to free the string when you're done with it
         */
        char *get_target(const char *path);

        /**
         * @brief Set the target of a symlink
         * @param path The path to the symlink
         * @param target The new target of the symlink
         * @return 0 on success, or on error return -1 and set `error`
         * @note This does not validate `target` but it only treats it as a string
         */
        int set_target(const char *path, const char *target);

        /**
         * @brief List the contents of a directory
         * @param fd The file descriptor of the directory to list
         * @return A newly allocated array of newly allocated strings
         * @note Be sure to free both the array and the strings within
         */
        char * const *list(int fd);

        /**
         * @brief Open a file relative to a directory
         * @param dir The file descriptor of the directory to open the file relative to
         * @param path The path to the file, starting from the directory
         * @param flags The flags to open the file with
         * @param mode The mode to open the file with, if `flags & O_CREAT`
         * @return A file descriptor on success, or on error return -1 and set `error`
         * @note This is similar to `open`, but the path is relative to the directory
         */
        int openat(int dir, const char *path, int flags, int mode = 0);

        /**
         * @brief Create a file at a given path
         * @param path The path to the file to be created
         * @param flags The flags to open the file with
         * @param mode The mode to create the file with
         * @return A file descriptor on success, or on error return -1 and set `error`
         * @note It is undefined what happens if the file already exists
         */
        int mkfile(const char *path, int flags, int mode);

        /**
         * @brief Create a file at a given path relative to a directory
         * @param dir The file descriptor of the directory to create the file in
         * @param path The path to the file to be created, starting from the directory
         * @param flags The flags to open the file with
         * @param mode The mode to create the file with
         * @return A file descriptor on success, or on error return -1 and set `error`
         * @note It is undefined what happens if the file already exists
         */
        int mkfileat(int dir, const char *path, int flags, int mode);

        /**
         * @brief Create a directory at a given path
         * @param path The path to the directory to be created
         * @param flags The flags to open the directory with
         * @param mode The mode to create the directory with
         * @return A file descriptor on success, or on error return -1 and set `error`
         * @note It is undefined what happens if the directory already exists
         */
        int mkdir(const char *path, int flags, int mode);

        /**
         * @brief Create a directory at a given path relative to a directory
         * @param dir The file descriptor of the directory to create the directory in
         * @param path The path to the directory to be created, starting from the directory
         * @param flags The flags to open the directory with
         * @param mode The mode to create the directory with
         * @return A file descriptor on success, or on error return -1 and set `error`
         * @note It is undefined what happens if the directory already exists
         */
        int mkdirat(int dir, const char *path, int flags, int mode);

        /**
         * @brief Create a symlink
         * @param path The path to the new symlink
         * @param target The target of the symlink
         * @return 0 on success, or on error return -1 and set `error`
         * @note This does not validate `target` but it only treats it as a string
         */
        int symlink(const char *path, const char *target);

        /**
         * @brief Create a symlink at a given path relative to a directory
         * @param dir The file descriptor of the directory to create the symlink in
         * @param path The path to the new symlink, starting from the directory
         * @param target The target of the symlink
         * @return 0 on success, or on error return -1 and set `error`
         * @note This does not validate `target` but it only treats it as a string
         */
        int symlinkat(int dir, const char *path, const char *target);

        /**
         * @brief Create a special file at a given path
         * @param path The path to the special file to be created
         * @param driver The thing that will handle operations on the special file\
         * @param flags The flags to open the special file with
         * @param mode The mode to create the special file with
         * @return A file descriptor on success, or on error return -1 and set `error`
         * @warning This takes ownership of `driver`, and will deallocate it later
         */
        int mksfile(const char *path, int flags, BaseSpecialDriver *driver, int mode);

        /**
         * @brief Create a special file at a given path relative to a directory
         * @param dir The file descriptor of the directory to create the special file in
         * @param path The path to the special file to be created, starting from the directory
         * @param flags The flags to open the special file with
         * @param driver The thing that will handle operations on the special file
         * @param mode The mode to create the special file with
         * @return A file descriptor on success, or on error return -1 and set `error`
         * @warning This takes ownership of `driver`, and will deallocate it later
         */
        int mksfileat(int dir, const char *path, int flags, BaseSpecialDriver *driver, int mode);

    private:
        VFSData *data;
    };

    extern VFS vfs;
} // namespace Hamster

