// Ram Filesystem

#pragma once

#include <filesystem/file.hpp>

namespace Hamster
{
    struct RamFsData;

    class RamFs : public BaseFilesystem
    {
    public:
        RamFs();
        ~RamFs() override;

        RamFs(const RamFs &) = delete;
        RamFs &operator=(const RamFs &) = delete;

        RamFs(RamFs &&);
        RamFs &operator=(RamFs &&);

        using BaseFilesystem::open;
        BaseFile *open(const char *path, int flags, va_list args) override;
        int rename(const char *old_path, const char *new_path) override;
        int unlink(const char *path) override;
        int link(const char *old_path, const char *new_path) override;
        int stat(const char *path, struct ::stat *buf) override;
        int lstat(const char *path, struct ::stat *buf) override;

        BaseRegularFile *mkfile(const char *name, int flags, int mode) override;
        BaseDirectory *mkdir(const char *name, int flags, int mode) override;
        BaseSymbolicFile *mksfile(const char *name, int flags, FileType type, int mode) override;

        int symlink(const char *old_path, const char *new_path) override;
        int readlink(const char *path, char *buf, size_t buf_size) override;
        char *readlink(const char *path) override;

        FileType type(const char *path) override;

        bool is_busy() override;

    private:
        RamFsData *data;
    };
} // namespace Hamster
