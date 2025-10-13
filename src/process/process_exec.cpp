
#include <process/task.hpp>
#include <filesystem/vfs.hpp>
#include <elf/elf_loader.hpp>
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

    int Process::exec(int fd, const char *const *, const char *const *)
    {
        assert(tasks.size() > 0);

        if (vfs.seek(fd, 0, H_SEEK_SET) != 0)
            return -1;

        int64_t size = vfs.size(fd);
        if (size < 0)
            return -1;
        
        // if leader is dead, choose another task and make it leader
        if (!leader)
            leader = *tasks.begin();

        // Erase all other tasks
        for (auto it = tasks.begin(); it != tasks.end();)
        {
            if (*it != leader)
                // This also removes the task
                (*it++)->exit();
            else
                ++it;
        }

        assert(tasks.size() == 1);

        MemorySpace &memory = *leader->get_memory();

        uint64_t entry_point = 0, ph_num = 0, brk = 0;
        if (load_elf(fd, memory, entry_point, ph_num, brk) < 0)
            return -1;

        RiscVEmulator &emulator = leader->get_emulator();
        emulator.pc = entry_point;
        memset(emulator.x, 0, sizeof(emulator.x));

        return 0;
    }
} // namespace Hamster
