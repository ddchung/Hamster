
#include <process/task.hpp>
#include <filesystem/vfs.hpp>
#include <elf/elf_loader.hpp>
#include <elf.h>
#include <cassert>

namespace Hamster
{
    Task *spawn(const char *path, const char *const *argv, const char *const *envp)
    {
        int fd = vfs.open(path, OPEN_RDONLY);
        if (fd < 0)
            return nullptr;
        Task *res = Task::create_task(fd, argv, envp);
        vfs.close(fd);
        return res;
    }

    int Process::exec(int fd, Task *new_leader, const char *const *argv, const char *const *envp)
    {
        static const char *empty[] = {nullptr};

        if (!argv)
            argv = empty;
        if (!envp)
            envp = empty;

        assert(tasks.size() > 0);

        if (vfs.seek(fd, 0, H_SEEK_SET) != 0)
            return -1;

        int64_t size = vfs.size(fd);
        if (size < 0)
            return -1;
        
        if (!tasks.contains(new_leader))
        {
            error = H_ESRCH;
            return -1;
        }

        leader = new_leader;

        // Erase all other tasks
        for (auto it = tasks.begin(); it != tasks.end();)
        {
            if (*it != leader)
                // This also removes the task
                (*it++)->exit(0);
            else
                ++it;
        }

        assert(tasks.size() == 1);
        assert(*tasks.begin() == leader);

        return leader->load_executable(fd, argv, envp);
    }
} // namespace Hamster
