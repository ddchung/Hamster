// Hamster scheduler

#pragma once

#include <process/thread.hpp>
#include <process/process.hpp>
#include <memory/stl_sequential.hpp>

namespace Hamster
{
    class Scheduler
    {
    public:
        Scheduler();
        ~Scheduler();

        Scheduler(const Scheduler &) = delete;
        Scheduler &operator=(const Scheduler &) = delete;

        Scheduler(Scheduler &&);
        Scheduler &operator=(Scheduler &&);

        /**
         * @brief Add a new process
         * @param process The process to add
         * @return 0 on success, or on error return -1 and set `error`
         * @note This will take ownership of `process`, and will deallocate it later
         * @note This will also set `process->pid` to a unique value
         */
        int add_process(Process *process);

        /**
         * @brief Make a new process from an ELF file
         * @param path The path to the ELF file
         * @param argv The arguments to the program. Note that by convention, the first argument is the program name
         * @param envp The environment variables
         * @return The process ID of the new process on success, or on error return -1 and set `error`
         * @note The CWD will be "/", and the UID and GID will be 0
         */
        int make_process_elf(const char *path, const char * const *argv = nullptr, const char * const *envp = nullptr);

        /**
         * @brief Tick the scheduler
         * @return The number of threads that were ticked
         * @note This will run one tick for every non-paused thread in the scheduler
         */
        size_t tick();

        /**
         * @brief Get a process by ID
         * @param pid The process ID to get
         * @return The process with the given ID, or nullptr if not found
         * @note This is a weak pointer, DO NOT deallocate it
         */
        Process *get_process(uint32_t pid) const;

        // Warning: don't free the processes!
        const List<Process *> &get_processes() const { return processes; }
    private:
        // Indexed by PID
        List<Process *> processes;
    };

    extern Scheduler scheduler;
} // namespace Hamster

