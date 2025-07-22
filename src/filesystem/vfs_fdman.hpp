#pragma once
#include <filesystem/base_file.hpp>
#include <memory/stl_sequential.hpp>
#include <errno/errno.h>

namespace Hamster {
class FDManager {
    struct Entry {
        BaseFile *file;
    };
public:
    FDManager();
    FDManager(const FDManager &) = delete;
    FDManager &operator=(const FDManager &) = delete;
    ~FDManager();
    FDManager(FDManager &&other);
    FDManager &operator=(FDManager &&other);
    int add_fd(BaseFile *file);
    int remove_fd(int fd);
    BaseFile *get_fd(int fd);
    void close_all();
private:
    Vector<Entry> fds;
};
} // namespace Hamster
