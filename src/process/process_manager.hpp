// manager of processes

#pragma once

#include <process/process.hpp>
#include <memory/stl_sequential.hpp>
#include <platform/config.hpp>
#include <cstdint>

namespace Hamster
{
    // is singleton
    class ProcessManager
    {
        ProcessManager() = default;
        ProcessManager(const ProcessManager&) = delete;
        ProcessManager &operator=(const ProcessManager&) = delete;
        ProcessManager(ProcessManager&&) = delete;
        ProcessManager &operator=(ProcessManager&&) = delete;
        ~ProcessManager() = default;

        struct ProcessInfo
        {
            Process process;
            uint32_t pid;
            char name[HAMSTER_PROCESS_NAME_LENGTH + 1];
            bool running : 1;
        };

    public:
        static ProcessManager &get_instance()
        {
            static ProcessManager instance;
            return instance;
        }

        // New process
        // returns the PID or 0 on error
        uint32_t create_process(const char *name);

        // Destroy a process
        int destroy_process(uint32_t pid);

        // Get a process by PID
        // returns a reference to a dummy on error
        Process &get_process(uint32_t pid);

        // get said dummy
        static Process &get_dummy_process();

        void tick_all();
        void tick_all64();

    private:
        Vector<ProcessInfo> processes;
    };
} // namespace Hamster


