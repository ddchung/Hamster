// Hamster signal handlers

#pragma once

#include <abi/structs.hpp>
#include <cstdint>

namespace Hamster
{
    using SignalHandler = void (*)(class Task &, const sys_siginfo &, const sys_sigaction &);

    class TaskSignalHandlers
    {
        struct Handler
        {
            SignalHandler handler;
            sys_sigaction action;
        };
    public:

        TaskSignalHandlers();

        /**
         * @brief Set the signal handler for a signal
         * @param signo The signal number
         * @param handler The handler for the signal
         * @param action The signal action for this handler. Zero fields by default
         * @return 0 on success, -1 on error and set `error`
         */
        int set_handler(uint8_t signo, SignalHandler handler, const sys_sigaction &action = {});

        /**
         * @brief Ignore a signal
         * @param signo the signal
         * @return 0 on success, -1 on error and set `error`
         */
        int set_ignore(uint8_t signo);
        
        /**
         * @brief Set a signal to its default handler
         * @param signo the signal
         * @return 0 on success, -1 on error and set `error`
         */
        int set_default(uint8_t signo);

        /**
         * @brief Check whether a signal has a handler installed
         * @param signo The signal to check
         * @return 1 if it has a handler, 0 if it doesn't, -1 on error
         */
        int is_handler(uint8_t signo);

        /**
         * @brief Check whether a signal is ignored
         * @param signo The signal to check
         * @return 1 if it's ignored, 0 if it's not, -1 on error and set `error`
         */
        int is_ignored(uint8_t signo);

        /**
         * @brief Check whether a signal is defaulted
         * @param signo The signal to check
         * @return 1 if its action is default, 0 if it isn't, -1 on error
         */
        int is_default(uint8_t signo);

        /**
         * @brief Call a signal handler
         * @param signo The signal number
         * @param task The task
         * @param siginfo The signal information to pass to the handler, if applicable
         * @return 0 on success, -1 on error and set `error`
         */
        int handle_signal(uint8_t signal, Task &task, const sys_siginfo &siginfo);

    private:
        inline static const SignalHandler SIG_DFL = (SignalHandler)0;
        inline static const SignalHandler SIG_IGN = (SignalHandler)1;
        Handler handlers[64];

        // These should be defined elsewhere

        static const SignalHandler default_handlers[64];
        static void sighand_dfl_nop(Task &, const sys_siginfo &, const sys_sigaction &);
        static void sighand_dfl_term(Task &, const sys_siginfo &, const sys_sigaction &);
        static void sighand_dfl_dump(Task &, const sys_siginfo &, const sys_sigaction &);
        static void sighand_dfl_stop(Task &, const sys_siginfo &, const sys_sigaction &);
        static void sighand_dfl_cont(Task &, const sys_siginfo &, const sys_sigaction &);
        static void sighand_ign(Task &, const sys_siginfo &, const sys_sigaction &);
    };
} // namespace Hamster

