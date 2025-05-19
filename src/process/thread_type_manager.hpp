// Hamster thread architecture manager

#pragma once

#include <process/base_thread.hpp>
#include <memory/stl_map.hpp>
#include <cstdint>

namespace Hamster
{
    class ThreadTypeManager
    {
    public:
        ThreadTypeManager() = default;
        ~ThreadTypeManager() = default;

        /**
         * @brief Register a thread type
         * @param type The type of the thread
         * @param factory The factory function to create new threads of this type
         * @return 0 on success, negative error code on failure
         */
        int register_thread_type(uint16_t type, BaseThread* (*factory)(MemorySpace&));

        /**
         * @brief Create a new thread of the given type
         * @param type The type of the thread
         * @param mem_space The memory space to use
         * @return A pointer to the new thread, or nullptr on failure
         */
        BaseThread* create_thread(uint16_t type, MemorySpace& mem_space);

        /**
         * @brief Load an ELF file into the memory space and create a thread of the right type
         * @param fd File descriptor of the ELF file
         * @param mem_space Memory space to load the ELF file into
         * @return A pointer to the new thread, or nullptr on failure
         * @note This will modify the memory space, and it will also set the entry point of the thread
         */
        BaseThread* load_elf(int fd, MemorySpace& mem_space);

        /**
         * @brief Get the factory function for a given thread type
         * @param type The type of the thread
         * @return The factory function, or nullptr if not found
         */
        BaseThread* (*get_factory(uint16_t type))(MemorySpace&);

    private:
        UnorderedMap<uint16_t, BaseThread* (*)(MemorySpace&)> thread_factories;
    };

    extern ThreadTypeManager thread_type_manager;
} // namespace Hamster

