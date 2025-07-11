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
     * @param ph_num Number of program headers in the ELF file. The program headers are stored starting from `HAMSTER_STACK_TOP + 1`
     * @param brk The starting program break: the highest address of the program, rounded up to the page boundary
     * @return 0 on success, -1 on failure.
     * @note The machine type must be RISC-V 
     */
    int load_elf(int fd, MemorySpace& mem_space, uint64_t& entry_point, uint64_t &ph_num, uint64_t &brk);
} // namespace Hamster

