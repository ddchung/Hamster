// Hamster thread

#pragma once

#include <memory/memory_space.hpp>
#include <syscall/syscall.hpp>
#include <cstdint>


namespace Hamster
{

    class BaseThread
    {
    public:
        BaseThread(MemorySpace &mem_sp)
            : mem(mem_sp)
        {
        }

        virtual ~BaseThread() = default;

        /**
         * @brief Set the address to start execution
         * @param addr The address to start execution
         * @return 0 on success, negative error code on failure
         * @note This is called at most once, before anything else
         */
        virtual int set_start_addr(uint64_t addr) = 0;

        /**
         * @brief Execute one instruction
         * @return 0 on success, 1 on exit, 2 on syscall, negative error code on failure
         * @note Assume that the memory space is already swapped in, and do not swap it out
         */
        virtual int tick() = 0;

        /**
         * @brief Get the current system call
         * @return The current system call
         * @note This is only called immediately after `tick` returns 2
         */
        virtual Syscall get_syscall() = 0;

        /**
         * @brief Set the system call return value
         * @param ret The return value
         * @note This is only called after `get_syscall`
         */
        virtual void set_syscall_ret(uint64_t ret) = 0;

    protected:
        MemorySpace &mem;
    };
} // namespace Hamster

