#include <filesystem/vfs_fdman.hpp>
#include <memory/allocator.hpp>
#include <errno/errno.h>

namespace Hamster
{

    FDManager::FDManager() = default;
    FDManager::~FDManager() { close_all(); }
    FDManager::FDManager(FDManager &&other)
    {
        fds = std::move(other.fds);
        other.fds = Vector<Entry>();
    }
    FDManager &FDManager::operator=(FDManager &&other)
    {
        if (this == &other)
            return *this;
        std::swap(fds, other.fds);
        return *this;
    }

    int FDManager::add_fd(BaseFile *file)
    {
        if (file == nullptr)
            return -1;
        int fd = -1;
        for (int i = 0; i < (int)fds.size(); ++i)
        {
            if (fds[i].file == nullptr)
            {
                fd = i;
                break;
            }
        }
        if (fd == -1)
        {
            fd = fds.size();
            fds.push_back(Entry{nullptr});
        }
        fds[fd].file = file;
        return fd;
    }

    int FDManager::remove_fd(int fd)
    {
        if (fd < 0 || fd >= (int)fds.size())
        {
            error = EBADF;
            return -1;
        }
        if (fds[fd].file == nullptr)
        {
            error = EBADF;
            return -1;
        }
        auto &file = fds[fd].file;
        if (file->type() == FileType::Special)
        {
            BaseSpecialFile *sp_file = (BaseSpecialFile *)file;
            BaseSpecialDriverHandle *handle = sp_file->get_handle();
            dealloc(handle);
            sp_file->set_handle(nullptr);
        }
        dealloc(file);
        file = nullptr;
        return 0;
    }

    BaseFile *FDManager::get_fd(int fd)
    {
        if (fd < 0 || fd >= (int)fds.size())
        {
            error = EBADF;
            return nullptr;
        }
        if (fds[fd].file == nullptr)
        {
            error = EBADF;
            return nullptr;
        }
        return fds[fd].file;
    }

    void FDManager::close_all()
    {
        for (auto &fd : fds)
        {
            if (fd.file == nullptr)
                continue;
            if (fd.file->type() == FileType::Special)
            {
                BaseSpecialFile *sp_file = (BaseSpecialFile *)fd.file;
                BaseSpecialDriverHandle *handle = sp_file->get_handle();
                dealloc(handle);
                sp_file->set_handle(nullptr);
            }
            dealloc(fd.file);
            fd.file = nullptr;
        }
        fds.clear();
    }

} // namespace Hamster
