// ELF loader

#pragma once

#include <cstdint>
#include <memory/memory_space.hpp>

namespace Hamster
{
    /**
     * @brief Load an ELF file into the memory space.
     * @param fd File descriptor of the ELF file.
     * @param mem_space Memory space to load the ELF file into.
     * @param entry_point Entry point of the loaded ELF file.
     * @param machine_type Machine type of the ELF file.
     * @return 0 on success, -1 on failure.
     */
    int load_elf(int fd, MemorySpace& mem_space, uint64_t& entry_point, uint16_t& machine_type);
} // namespace Hamster

