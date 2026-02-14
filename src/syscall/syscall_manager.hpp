// Hamster system call manager

#pragma once

#include <memory/stl_map.hpp>
#include <process/task.hpp>
#include <cstdint>

namespace Hamster
{
    class SyscallManager
    {
    public:
        // Mainly for logging
        struct SyscallArg
        {
            enum class Type : uint8_t
            {
                ARG_NONE,
                ARG_INT,
                ARG_UINT,
                ARG_STR,
                ARG_PTR,
            } type;
            using enum Type;
            const char *name;
        };

        struct Syscall
        {
            int32_t num_args;
            SyscallArg args[6];
            const char *name;
            void *handler; // Will be casted to the correct type when called
                           // e.g. int32_t(Task&,uint32_t,int32_t)
        };

        /**
         * @brief Register a system call
         * @param id The system call id
         * @param syscall The system call handler and metadata
         * @return 0 on success, -1 on failure and set `error`
         * @note Fails if a custom system call has already been registered
         */
        int register_syscall(uint32_t id, const Syscall &syscall);

        /**
         * @brief Unregister a custom system call, reverting to the default implementation
         * @param id The system call id
         * @return 0 on success, -1 on failure and set `error`
         * @note Fails if the system call is not a custom system call
         */
        int unregister_syscall(uint32_t id);

        /**
         * @brief Perform a system call
         * @param id The system call id
         * @param task The task performing the system call
         * @param args A pointer to the system arguments, up to 6
         * @return Whatever the system call returns. `-H_ENOSYS` if invalid id
         * @note Note that this does not return -1 and set `error` on failure, but returns `-{errno}`
         */
        int32_t do_syscall(uint32_t id, Task &task, uint32_t *args);

    private:
        static const Map<uint32_t, Syscall> default_syscalls;
        Map<uint32_t, Syscall> syscalls;
    };

    extern SyscallManager syscall_manager;
} // namespace Hamster

