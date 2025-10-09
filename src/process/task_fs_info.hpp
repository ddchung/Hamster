// Hamster task filesystem information

#pragma once

#include <process/task_base_fd.hpp>
#include <memory/stl_sequential.hpp>
#include <cstdint>

namespace Hamster
{
    class TaskFSInfo
    {
    public:
        /**
         * @brief Open a VFS file descriptor to the appropriate relative directory
         * @param path The userspace provided path
         * @param at_fd The FD slot. Nullptr to signify AT_FDCWD
         * @return A newly opened VFS directory, such that opening `path` relative to this
         *         directory results in the intended file. -1 on error and set `error`
         */
        int open_rel_fd(const char *path, BaseTaskFD *at_fd = nullptr);

        /**
         * @brief Change the current root
         * @param new_root The new root to change to
         * @return 0 on success, -1 on error
         */
        int chroot(const char *new_root);

        /**
         * @brief Change the current working directory
         * @param new_cwd The new CWD, relative to the current one or root
         * @return 0 on success, -1 on error
         * @note If `new_cwd` starts with '/', it is relative to the current root. Else, it is
         *       relative to the CWD
         */
        int chdir(const char *new_cwd);

        /**
         * @brief Get the current working directory
         * @return A newly allocated string containing the CWD
         *         relative to the current root
         */
        char *getcwd();

        /**
         * @brief Get the absolute CWD, relative to VFS /
         * @return A newly allocated string containing the absolute CWD
         */
        char *get_abs_cwd();

    private:
        String root_path{"/"};
        String cwd_path{"/"};
        int umask = 0;
    };
} // namespace Hamster

