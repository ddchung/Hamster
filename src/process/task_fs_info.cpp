// Hamster task FS info

#include <process/task_fs_info.hpp>
#include <memory/allocator.hpp>
#include <filesystem/vfs.hpp>
#include <errno/errno.h>
#include <filesystem>
#include <cstring>

namespace Hamster
{
    int TaskFSInfo::open_rel_fd(const char *path, BaseTaskFD *at_fd) const
    {
        if (!path || path[0] == '\0')
        {
            error = H_EINVAL; // Invalid path
            return -1;
        }

        if (path[0] == '/')
        {
            // Absolute path
            const char *root_path = this->root_path.c_str();

            return vfs.open(root_path, OPEN_RDWR | OPEN_DIRECTORY);
        }
        else if (!at_fd)
        {
            // AT_FDCWD
            const char *cwd_path = this->cwd_path.c_str();
            return vfs.open(cwd_path, OPEN_RDWR | OPEN_DIRECTORY);
        }
        else
        {
            int vfs_fd = at_fd->get_vfs_fd();
            if (vfs_fd < 0)
                return -1;
            
            // Ensure it's a directory
            switch (vfs.is_directory(vfs_fd))
            {
            case 0:
                error = H_ENOTDIR;
                return -1;
            case 1:
                break;
            default:
                return -1;
            }
            
            return vfs.dup(vfs_fd);
        }
    }

    int TaskFSInfo::chroot(const char *newroot)
    {
        if (!newroot || !newroot[0])
        {
            error = H_EINVAL;
            return -1;
        }

        // Select appropriate starting point
        if (newroot[0] == '/')
            root_path = std::filesystem::path(root_path + newroot).lexically_normal().generic_string();
        else
            root_path = std::filesystem::path(cwd_path + newroot).lexically_normal().generic_string();
        
        return 0;
    }

    int TaskFSInfo::chdir(const char *newcwd)
    {
        if (!newcwd || !newcwd[0])
        {
            error = H_EINVAL;
            return -1;
        }

        // Select appropriate starting point
        if (newcwd[0] == '/')
            cwd_path = std::filesystem::path(root_path + "/" + newcwd).lexically_normal().generic_string();
        else
            // note: String cwd_path;
            cwd_path = std::filesystem::path(cwd_path + "/" + newcwd).lexically_normal().generic_string();
        
        return 0;
    }

    char *TaskFSInfo::getcwd() const
    {
        auto s = std::filesystem::path(cwd_path).lexically_relative(root_path).generic_string();
        s = std::filesystem::path("/" + s).lexically_normal().generic_string();
        assert(s.size() != 0);

        char *buf = alloc<char>(s.size() + 1);
        strcpy(buf, s.c_str());
        return buf;
    }

    char *TaskFSInfo::get_abs_cwd() const
    {
        char *buf = alloc<char>(cwd_path.size() + 1);
        strcpy(buf, cwd_path.c_str());
        return buf;
    }
} // namespace Hamster

