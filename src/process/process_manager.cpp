// process manager

#include <process/process_manager.hpp>
#include <platform/config.hpp>
#include <cstdint>
#include <cstring>
#include <algorithm>

namespace Hamster
{
    namespace
    {
        uint32_t pid_counter = 0;
        Process dummy_process;
    }

    uint32_t ProcessManager::create_process(const char *name)
    {
        ProcessInfo &pi = processes.emplace_back();
        pi.pid = ++pid_counter;
        pi.running = true;
        strncpy(pi.name, name, HAMSTER_PROCESS_NAME_LENGTH);
        pi.name[HAMSTER_PROCESS_NAME_LENGTH] = '\0';

        return pid_counter;
    }

    int ProcessManager::destroy_process(uint32_t pid)
    {
        auto it = std::find_if(processes.begin(), processes.end(), [pid](const ProcessInfo &pi) {
            return pi.pid == pid;
        });

        if (it != processes.end())
        {
            processes.erase(it);
            return 0; // success
        }
        return -1; // process not found
    }

    Process &ProcessManager::get_process(uint32_t pid)
    {
        auto it = std::find_if(processes.begin(), processes.end(), [pid](const ProcessInfo &pi) {
            return pi.pid == pid;
        });

        if (it != processes.end())
        {
            return it->process;
        }
        return dummy_process; // return dummy process if not found
    }

    void ProcessManager::tick_all()
    {
        for (size_t i = 0; i < processes.size(); i++)
        {
            if (processes[i].running)
            {
                current_process = &processes[i];
                auto runcode = processes[i].process.tick_all();
                current_process = nullptr;

                if (runcode.terminate)
                {
                    processes[i].running = false;
                    processes.erase(processes.begin() + i);
                    i--;
                }
            }
        }
    }

    void ProcessManager::tick_all64()
    {
        for (size_t i = 0; i < processes.size(); i++)
        {
            if (processes[i].running)
            {
                current_process = &processes[i];
                auto runcode = processes[i].process.tick_all64();
                current_process = nullptr;

                if (runcode.terminate)
                {
                    processes[i].running = false;
                    processes.erase(processes.begin() + i);
                    i--;
                }
            }
        }
    }

    Process &ProcessManager::get_dummy_process()
    {
        return dummy_process;
    }
} // namespace Hamster

