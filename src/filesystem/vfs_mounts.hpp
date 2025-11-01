#pragma once
#include <filesystem/base_file.hpp>
#include <memory/stl_map.hpp>
#include <cstring>
#include <errno/errno.h>

namespace Hamster {
class MountPoint {
public:
    MountPoint(BaseFilesystem *fs);
    MountPoint(const MountPoint &) = delete;
    MountPoint &operator=(const MountPoint &) = delete;
    MountPoint(MountPoint &&other);
    MountPoint &operator=(MountPoint &&other);
    ~MountPoint();
    BaseFilesystem *fs;
    uint32_t children;

    // Weak pointer
    MountPoint *parent;
};

class Mounts {
public:
    Mounts();
    Mounts(const Mounts &) = delete;
    Mounts &operator=(const Mounts &) = delete;
    Mounts(Mounts &&other);
    Mounts &operator=(Mounts &&other);
    ~Mounts();
    BaseDirectory *resolve_mount(BaseFile *file);
    BaseFile *resolve_symlink(BaseSymlink *link, int flags, BaseDirectory *dir = nullptr);
    // Open a file or directory, resolving symlinks if necessary
    // If flags contains OPEN_NOFOLLOW, and the last component is a symlink, it will return the symlink itself
    BaseFile *lopen(const char *path, int flags, int mode, BaseDirectory *dir = nullptr);
    int access(const char *path, int uid, int *groups, size_t numgroups, int mode, BaseDirectory *dir = nullptr, int flags = 0);
    int mount(const char *path, BaseFilesystem *fs);
    int mount_root(BaseFilesystem *fs);
    int unmount(const char *path);
    int unmount_root();
private:
    UnorderedMap<int, MountPoint> mounts;
    MountPoint *root_mount;
};
} // namespace Hamster
