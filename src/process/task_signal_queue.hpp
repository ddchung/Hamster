// Signal queue

#pragma once

#include <process/task_signal_mask.hpp>
#include <memory/stl_sequential.hpp>
#include <memory/stl_map.hpp>
#include <abi/structs.hpp>
#include <cstdint>

namespace Hamster
{
    class TaskSignalQueue
    {
    public:
        /**
         * @brief Send a signal
         * @param siginfo The signal object
         * @return 0 on success, -1 on error
         */
        int push(const sys_siginfo &siginfo);

        /**
         * @brief Get the first signal to be handled
         * @param mask The task's signal mask, to determine the first available signal
         * @return A weak pointer to the signal, or nullptr on error and set `error`
         * @note Pointer valid until signal popped
         */
        const sys_siginfo *peek(const TaskSignalMask &mask) const;

        /**
         * @brief Pop the first signal to be handled
         * @param mask The task's signal mask, to determine the signal to pop
         * @note Does not return anything, use `peek_signal` to access
         */
        void pop(const TaskSignalMask &mask);

        /**
         * @brief Get the number of pending signals
         * @return The number of pending signals. 0 if none
         */
        size_t size() const;

    private:
        Deque<sys_siginfo> signal_queue; // for realtime signals
        Map<uint8_t, sys_siginfo> signal_map; // for normal signals (signo -> siginfo)
    };
} // namespace Hamster

