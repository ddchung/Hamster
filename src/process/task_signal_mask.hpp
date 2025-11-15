// Hamster signal mask

#pragma once

#include <abi/structs.hpp>
#include <cstdint>

namespace Hamster
{
    class TaskSignalMask
    {
    public:
        /**
         * @brief Block a signal
         * @param signo The signal number to block
         */
        void block(uint8_t signo) { set_blocked(signo, true); }

        /**
         * @brief Block many signals at once
         * @param signals Signals to block
         */
        void block(sys_sigset signals);

        /**
         * @brief Set the signal mask
         * @param sigmask The new signal mask. 1 is blocked, 0 is not
         */
        void from_sigset(sys_sigset sigmask);

        /**
         * @brief Unblock a signal
         * @param signo The signal number to unblock
         */
        void unblock(uint8_t signo) { set_blocked(signo, false); }

        /**
         * @brief Set the blocked state of a signal
         * @param signo The signal number
         * @param blocked Whether it should be blocked
         */
        void set_blocked(uint8_t signo, bool blocked);

        /**
         * @brief Check whether a signal is blocked
         * @param signo The signal to check
         * @return 1 if it's blocked, 0 if it's not, -1 on error and set `error`
         */
        int check(uint8_t signo) const;

        /**
         * @brief Convert the whole mask into a uint64_t
         * @param invert Whether to invert the mask
         * @return The converted signal mask
         * @note By default, blocked signals are a 1 and unblocked is 0. If inverted, it is flipped.
         *       Signal numbers are addressed with bit `1 << (signo - 1)`, so signal #1 will be LSB, and
         *       signal #64 will be MSB
         */
        uint64_t convert(bool invert = false) const;

        /**
         * @brief Convert the mask into a sys_sigset
         * @return The sys_sigset object
         */
        sys_sigset to_sigset() const;

    private:
        uint64_t mask = 0;
    };
} // namespace Hamster

