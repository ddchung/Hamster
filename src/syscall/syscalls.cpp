// syscall implementations

#pragma once

#include <syscall/syscalls.hpp>
#include <process/process_manager.hpp>
#include <platform/platform.hpp>
#include <cstdint>
#include <cassert>

namespace Hamster
{
    uint32_t Syscalls::exit(uint32_t status)
    {
        ProcessManager::ProcessInfo *current_process = ProcessManager::get_instance().get_current_process();
        assert(current_process != nullptr);

        // TODO: destroy process
        current_process->running = false;

        return status;
    }

    uint32_t Syscalls::log(uint32_t message_addr)
    {
        ProcessManager::ProcessInfo *current_process = ProcessManager::get_instance().get_current_process();
        assert(current_process != nullptr);

        MemorySpace &memory_space = current_process->process.get_memory_space();

        uint64_t addr = message_addr;
        while (true)
        {
            uint8_t &byte = memory_space[addr];
            if (&byte == &Page::get_dummy())
            {
                // unknown territory
                // halt process
                current_process->running = false;
                return -1;
            }
            if (byte == '\0')
                break;
            
            _log((char)byte);
            addr++;
        }

        return 0;
    }
} // namespace Hamster

