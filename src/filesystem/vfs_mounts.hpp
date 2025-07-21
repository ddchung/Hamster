#pragma once
#include <filesystem/base_file.hpp>
#include <memory/stl_sequential.hpp>
#include <cstring>
#include <errno/errno.h>

namespace Hamster {
class MountPoint {
public:
    MountPoint(const char *path, BaseFilesystem *fs);
    MountPoint(const MountPoint &) = delete;
    MountPoint &operator=(const MountPoint &) = delete;
    MountPoint(MountPoint &&other);
    MountPoint &operator=(MountPoint &&other);
    ~MountPoint();
    char *path;
    BaseFilesystem *fs;
};

struct RelativePath {
    MountPoint *mount;
    const char *path;
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
    static constexpr uint32_t FLAG_MOUNTPOINT = 1 << 0;
    static constexpr uint32_t MOUNT_ID_MASK = 0xFFFF << 16;
private:
    Vector<MountPoint *> mounts;
};
} // namespace Hamster
