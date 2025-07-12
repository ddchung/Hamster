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
         * @param mode The mode to open the file with, if `flags & OPEN_CREAT`
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
         * @brief Move a file
         * @param old_path The path to the file to move
         * @param new_path The path to move the file to
         * @return 0 on success, or on error return -1 and set `error`
         */
        int rename(const char *old_path, const char *new_path);

        /**
         * @brief Move a file from one path to another relative to two directories
         * @param old_dir The file descriptor of the directory to move the file from
         * @param old_path The path to the file to move, starting from `old_dir`
         * @param new_dir The file descriptor of the directory to move the file to
         * @param new_path The path to move the file to, starting from `new_dir`
         * @return 0 on success, or on error return -1 and set `error`
         * @note This is similar to `rename`, but the paths are relative to the directories
         */
        int renameat(int old_dir, const char *old_path, int new_dir, const char *new_path);

        /**
         * @brief Remove a file at a given path
         * @param path The path to the file to remove
         * @return 0 on success, or on error return -1 and set `error`
         * @note This does not close any file descriptors that point to the file
         * @note This will remove symlinks, not follow them
         */
        int remove(const char *path);

        /**
         * @brief Remove a file at a given path relative to a directory
         * @param dir The file descriptor of the directory to remove the file from
         * @param path The path to the file to remove, starting from the directory
         * @return 0 on success, or on error return -1 and set `error`
         * @note This does not close any file descriptors that point to the file
         * @note This will remove symlinks, not follow them
         */
        int removeat(int dir, const char *path);

        /**
         * @brief Stat a file described by a file descriptor
         * @param fd The file descriptor to stat
         * @return 0 on success, or on error return -1 and set `error`
         * @note This cannot be used to stat a symlink, as a file descriptor cannot
         *       * point to a symlink
         */
        int stat(int fd, sys_stat *buf);

        /**
         * @brief Stat a file at a given path
         * @param path The path to the file
         * @return 0 on success, or on error return -1 and set `error`
         * @note This can be used to stat a symlink, as it will not follow it
         */
        int lstat(const char *path, sys_stat *buf);

        /**
         * @brief Stat a file at a given path relative to a directory
         * @param dir The file descriptor of the directory to stat the file relative to
         * @param path The path to the file, starting from the directory
         * @return 0 on success, or on error return -1 and set `error`
         * @note This can be used to stat a symlink, as it will not follow it
         * @note This is similar to `lstat`, but the path is relative to the directory
         */
        int lstatat(int dir, const char *path, sys_stat *buf);

        /**
         * @brief Link a file at a given path to another path
         * @param target The path to the file to link
         * @param path The path to the new link
         * @return 0 on success, or on error return -1 and set `error`
         * @note This creates a hard link, not a symlink
         */
        int link(const char *target, const char *path);

        /**
         * @brief Link a file at a given path relative to a directory to another path relative to another directory
         * @param old_dir The file descriptor of the directory to link the file from
         * @param old_path The path to the file to link, starting from `old_dir`
         * @param new_dir The file descriptor of the directory to link the file to
         * @param new_path The path to the new link, starting from `new_dir`
         * @return 0 on success, or on error return -1 and set `error`
         * @note This creates a hard link, not a symlink
         */
        int linkat(int old_dir, const char *old_path, int new_dir, const char *new_path);

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
         * @param whence One of `H_SEEK_SET`, `H_SEEK_CUR`, or `H_SEEK_END`
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
         * @brief Get the target that a symlink points to relative to a directory
         * @param dir The file descriptor of the directory to get the symlink target from
         * @param path The path to the symlink, starting from the directory
         * @return A newly allocated string containing the target, or nullptr on error
         * @note Be sure to free the string when you're done with it
         * @note This is similar to `get_target`, but the path is relative to the directory
         * @note This does not validate the symlink, it only treats it as a string
         */
        char *get_targetat(int dir, const char *path);

        /**
         * @brief Set the target of a symlink
         * @param path The path to the symlink
         * @param target The new target of the symlink
         * @return 0 on success, or on error return -1 and set `error`
         * @note This does not validate `target` but it only treats it as a string
         */
        int set_target(const char *path, const char *target);

        /**
         * @brief Set the target of a symlink relative to a directory
         * @param dir The file descriptor of the directory to set the symlink target in
         * @param path The path to the symlink, starting from the directory
         * @param target The new target of the symlink
         * @return 0 on success, or on error return -1 and set `error`
         * @note This does not validate `target` but it only treats it as a string
         */
        int set_targetat(int dir, const char *path, const char *target);

        /**
         * @brief List the contents of a directory
         * @param fd The file descriptor of the directory to list
         * @param count The number of entries to list, by default it will list all entries
         * @return A newly allocated array of newly allocated strings
         * @note Be sure to free both the array and the strings within
         */
        char * const *list(int fd, size_t count = SIZE_MAX);

        /**
         * @brief Open a file relative to a directory
         * @param dir The file descriptor of the directory to open the file relative to
         * @param path The path to the file, starting from the directory
         * @param flags The flags to open the file with
         * @param mode The mode to open the file with, if `flags & OPEN_CREAT`
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
         * @brief Create a file at a given path, without opening a file descriptor
         * @param path The path to the file to be created
         * @param mode The mode to create the file with
         * @return 0 on success, or on error return -1 and set `error`
         * @note It is undefined what happens if the file already exists
         */
        int mkfile(const char *path, int mode);

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
         * @brief Create a file at a given path relative to a directory, without opening a file descriptor
         * @param dir The file descriptor of the directory to create the file in
         * @param path The path to the file to be created, starting from the directory
         * @param mode The mode to create the file with
         * @return 0 on success, or on error return -1 and set `error`
         * @note It is undefined what happens if the file already exists
         */
        int mkfileat(int dir, const char *path, int mode);

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
         * @brief Create a directory at a given path, without opening a file descriptor
         * @param path The path to the directory to be created
         * @param mode The mode to create the directory with
         * @return 0 on success, or on error return -1 and set `error`
         * @note It is undefined what happens if the directory already exists
         */
        int mkdir(const char *path, int mode);

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
         * @brief Create a directory at a given path relative to a directory, without opening a file descriptor
         * @param dir The file descriptor of the directory to create the directory in
         * @param path The path to the directory to be created, starting from the directory
         * @param mode The mode to create the directory with
         * @return 0 on success, or on error return -1 and set `error`
         * @note It is undefined what happens if the directory already exists
         */
        int mkdirat(int dir, const char *path, int mode);

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
         * @param driver The thing that will handle operations on the special file
         * @param flags The flags to open the special file with
         * @param mode The mode to create the special file with
         * @return A file descriptor on success, or on error return -1 and set `error`
         * @warning This takes ownership of `driver`, and will deallocate it later
         */
        int mksfile(const char *path, int flags, BaseSpecialDriver *driver, int mode);

        /**
         * @brief Create a special file at a given path, without opening a file descriptor
         * @param path The path to the special file to be created
         * @param driver The thing that will handle operations on the special file
         * @param mode The mode to create the special file with
         * @return 0 on success, or on error return -1 and set `error`
         * @warning This takes ownership of `driver`, and will deallocate it later
         */
        int mksfile(const char *path, BaseSpecialDriver *driver, int mode);

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

        /**
         * @brief Create a special file at a given path relative to a directory, without opening a file descriptor
         * @param dir The file descriptor of the directory to create the special file in
         * @param path The path to the special file to be created, starting from the directory
         * @param driver The thing that will handle operations on the special file
         * @param mode The mode to create the special file with
         * @return 0 on success, or on error return -1 and set `error`
         * @warning This takes ownership of `driver`, and will deallocate it later
         */
        int mksfileat(int dir, const char *path, BaseSpecialDriver *driver, int mode);

        /**
         * @brief Check whether a given file is a TTY device
         * @param fd The file descriptor to check
         * @return 1 if it is a TTY device, 0 if it is not, or on error return -1 and set `error`
         * @note For non-character devices, this returns 0
         */
        int isatty(int fd);

        /**
         * @brief Duplicate a file descriptor
         * @param fd The file descriptor to duplicate
         * @return A new file descriptor on success, or on error return -1 and set `error`
         */
        int dup(int fd);

    private:
        VFSData *data;
    };

    extern VFS vfs;
} // namespace Hamster

