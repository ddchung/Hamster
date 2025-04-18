// Ram Filesystem

#pragma once

#include <filesystem/file.hpp>

namespace Hamster
{
    class RamFsDirNode;

    class RamFs : public BaseFilesystem
    {
    public:
        RamFs();
        ~RamFs() override;

        BaseFile *open(const char *path, int flags, ...) override;
        BaseFile *open(const char *path, int flags, va_list args) override;
        int rename(const char *old_path, const char *new_path) override;
        int unlink(const char *path) override;
        int link(const char *old_path, const char *new_path) override;
        int stat(const char *path, struct ::stat *buf) override;

        BaseRegularFile *mkfile(const char *name, int flags, int mode) override;
        BaseDirectory *mkdir(const char *name, int flags, int mode) override;
        BaseFifoFile *mkfifo(const char *name, int flags, int mode) override;

    private:
        RamFsDirNode *root;

        BaseFile *open_impl(const char *path, RamFsDirNode *parent, int flags, int mode);
    };
} // namespace Hamster
