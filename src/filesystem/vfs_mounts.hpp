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
    BaseFile *lopen(const char *path, int flags, int mode, BaseDirectory *dir = nullptr);
    int mount(const char *path, BaseFilesystem *fs);
    int mount_root(BaseFilesystem *fs);
    int unmount(const char *path);
    int unmount_root();
private:
    UnorderedMap<int, MountPoint> mounts;
    MountPoint *root_mount;
};
} // namespace Hamster
