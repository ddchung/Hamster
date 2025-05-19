// Thread type manager implementation

#include <process/thread_type_manager.hpp>
#include <process/base_thread.hpp>
#include <elf/elf_loader.hpp>
#include <errno/errno.h>

namespace Hamster
{
    int ThreadTypeManager::register_thread_type(uint16_t type, BaseThread* (*factory)(MemorySpace&))
    {
        if (thread_factories.find(type) != thread_factories.end())
        {
            error = EEXIST;
            return -1; // Thread type already registered
        }

        thread_factories[type] = factory;
        return 0;
    }

    BaseThread* ThreadTypeManager::create_thread(uint16_t type, MemorySpace& mem_space)
    {
        auto it = thread_factories.find(type);
        if (it == thread_factories.end())
        {
            error = ENOTSUP;
            return nullptr; // Thread type not supported
        }

        return it->second(mem_space);
    }

    BaseThread* ThreadTypeManager::load_elf(int fd, MemorySpace& mem_space)
    {
        uint64_t entry_point = 0;
        uint16_t machine_type = 0;

        if (Hamster::load_elf(fd, mem_space, entry_point, machine_type) < 0)
        {
            error = EIO;
            return nullptr; // Failed to load ELF file
        }

        auto it = thread_factories.find(machine_type);
        if (it == thread_factories.end())
        {
            error = ENOTSUP;
            return nullptr; // Thread type not supported
        }

        BaseThread* thread = it->second(mem_space);
        thread->set_start_addr(entry_point);
        return thread;
    }

    BaseThread* (*ThreadTypeManager::get_factory(uint16_t type))(MemorySpace&)
    {
        auto it = thread_factories.find(type);
        if (it == thread_factories.end())
        {
            return nullptr; // Thread type not found
        }

        return it->second;
    }
} // namespace Hamster

