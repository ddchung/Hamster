// Hamster scheduler

#include <process/scheduler.hpp>
#include <memory/allocator.hpp>
#include <errno/errno.h>
#include <utility>

namespace Hamster
{
    Scheduler::Scheduler()
        : processes()
    {
        // PID 0 doesn't exist
        processes.push_back(nullptr);
    }
    
    Scheduler::~Scheduler()
    {
        for (Process *process : processes)
        {
            dealloc(process);
        }
        processes.clear();
    }

    Scheduler::Scheduler(Scheduler &&other)
        : processes(std::move(other.processes))
    {
        other.processes = List<Process *>();
    }

    Scheduler &Scheduler::operator=(Scheduler &&other)
    {
        if (this == &other)
            return *this;
        std::swap(processes, other.processes);
        return *this;
    }

    int Scheduler::add_process(Process *process)
    {
        if (!process)
        {
            error = EINVAL;
            return -1;
        }
        
        // Get a new PID
        int new_pid = -1;
        size_t counter = 0;
        for (auto process : processes)
        {
            // Skip PID 0
            if (!process && counter > 0)
            {
                new_pid = counter;
                break;
            }
            ++counter;
        }
        if (new_pid == -1)
        {
            // No free PID found, allocate a new one
            new_pid = processes.size();
            processes.push_back(nullptr);
        }

        process->pid = new_pid;

        auto it = processes.begin();
        std::advance(it, new_pid);
        *it = process;

        // Assume that `process` has its members correctly set

        // OK
        return 0;
    }

    int Scheduler::make_process_elf(const char *path, const char *const *argv, const char *const *envp)
    {
        if (!path)
        {
            error = EINVAL;
            return -1;
        }
        
        Process *process = alloc<Process>();

        process->cwd = "/";
        process->uid = 0;
        process->gid = 0;
        process->euid = 0;
        process->egid = 0;
        process->ppid = 0;
        process->pgid = 0;
        process->sid = 0;
        process->exit_status = 0;

        if (process->load_elf(path, argv, envp) < 0)
        {
            dealloc(process);
            error = EIO;
            return -1;
        }

        if (add_process(process) < 0)
        {
            dealloc(process);
            error = EIO;
            return -1;
        }

        // OK
        return 0;
    }

    size_t Scheduler::tick()
    {
        size_t ticked_count = 0;
        for (Process *&process : processes)
        {
            if (!process)
                continue; // Skip null processes
            else if (process->threads.empty())
                continue; // Skip ended processes
            else
            {
                // Tick each thread in the process
                process->threads.remove_if([&](Thread &thread) {
                    if (thread.get_state() == ThreadState::ENDED)
                        return true; // Remove ended threads
                    if (thread.is_paused())
                    {
                        ++ticked_count;
                        thread.get_current_pause_callback()(thread);
                        return false; // Keep paused threads
                    }
                    thread.tick(); // Tick the thread
                    ++ticked_count; // Count the ticked thread
                    return false; // Keep running threads
                });
            }
        }
        return ticked_count; // Return the number of threads that were ticked
    }

    uint32_t Scheduler::get_exit_status(uint32_t ppid, int &exit_status)
    {
        bool found = false;
        for (auto it = processes.begin(); it != processes.end(); ++it)
        {
            Process *process = *it;
            if (!process || process->ppid != ppid)
                continue; // Skip processes that don't match the parent PID

            found = true;

            if (!process->threads.empty())
            {
                continue; // try to find an ended child
            }

            exit_status = process->exit_status; // Get the exit status

            uint32_t pid = process->pid; // Get the PID
            dealloc(process); // Deallocate the process
            *it = nullptr;

            return pid;
        }

        error = found ? EBUSY : ECHILD;
        return 0;
    }

    Process *Scheduler::get_process(uint32_t pid) const
    {
        if (pid >= processes.size())
            return nullptr; // Invalid PID
        auto it = processes.begin();
        std::advance(it, pid);
        return *it;
    }
} // namespace Hamster
