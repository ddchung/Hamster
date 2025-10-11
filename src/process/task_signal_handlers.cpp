// Hamster signal handlers

#include <process/task_signal_handlers.hpp>
#include <errno/errno.h>
#include <cassert>

namespace Hamster
{
    TaskSignalHandlers::TaskSignalHandlers()
    {
        for (auto &handler : handlers)
            handler.handler = SIG_DFL;
    }

    int TaskSignalHandlers::set_handler(uint8_t signo, SignalHandler handler, const sys_sigaction &action)
    {
        if (signo < 1 || signo > 64)
        {
            error = H_EINVAL;
            return -1;
        }

        auto &signal_handler = handlers[signo - 1];
        signal_handler.handler = handler;
        signal_handler.action = action;
        return 0;
    }

    int TaskSignalHandlers::set_ignore(uint8_t signo)
    {
        if (signo < 1 || signo > 64)
        {
            error = H_EINVAL;
            return -1;
        }

        handlers[signo - 1].handler = SIG_IGN;
        return 0;
    }

    int TaskSignalHandlers::set_default(uint8_t signo)
    {
        if (signo < 1 || signo > 64)
        {
            error = H_EINVAL;
            return -1;
        }

        handlers[signo - 1].handler = SIG_DFL;
        return 0;
    }

    int TaskSignalHandlers::is_handler(uint8_t signo)
    {
        if (signo < 1 || signo > 64)
        {
            error = H_EINVAL;
            return -1;
        }

        return handlers[signo - 1].handler != SIG_DFL &&
               handlers[signo - 1].handler != SIG_IGN;
    }

    int TaskSignalHandlers::is_ignored(uint8_t signo)
    {
        if (signo < 1 || signo > 64)
        {
            error = H_EINVAL;
            return -1;
        }

        return handlers[signo - 1].handler == SIG_IGN;
    }

    int TaskSignalHandlers::is_default(uint8_t signo)
    {
        if (signo < 1 || signo > 64)
        {
            error = H_EINVAL;
            return -1;
        }

        return handlers[signo - 1].handler == SIG_DFL;
    }

    int TaskSignalHandlers::handle_signal(uint8_t signo, Task &task, const sys_siginfo &siginfo)
    {
        if (signo < 1 || signo > 64)
        {
            error = H_EINVAL;
            return -1;
        }

        const auto &handler = handlers[signo - 1];

        if (handler.handler == SIG_DFL)
            default_handlers[signo - 1](task, siginfo, {});
        else if (handler.handler == SIG_IGN)
            sighand_ign(task, siginfo, {});
        else
            handler.handler(task, siginfo, handler.action);

        return 0;
    }

    const SignalHandler TaskSignalHandlers::default_handlers[64]{
        sighand_dfl_term, // 1 - SIGHUP (terminate)
        sighand_dfl_term, // 2 - SIGINT (terminate)
        sighand_dfl_term, // 3 - SIGQUIT (terminate + core)
        sighand_dfl_dump, // 4 - SIGILL (terminate + core)
        sighand_dfl_dump, // 5 - SIGTRAP (terminate + core)
        sighand_dfl_dump, // 6 - SIGABRT/SIGIOT (terminate + core)
        sighand_dfl_dump, // 7 - SIGBUS (terminate + core)
        sighand_dfl_dump, // 8 - SIGFPE (terminate + core)
        sighand_dfl_term, // 9 - SIGKILL (terminate, cannot catch/ignore)
        sighand_dfl_term, // 10 - SIGUSR1 (terminate)
        sighand_dfl_dump, // 11 - SIGSEGV (terminate + core)
        sighand_dfl_term, // 12 - SIGUSR2 (terminate)
        sighand_dfl_term, // 13 - SIGPIPE (terminate)
        sighand_dfl_term, // 14 - SIGALRM (terminate)
        sighand_dfl_term, // 15 - SIGTERM (terminate)
        sighand_dfl_term, // 16 - SIGSTKFLT (terminate)
        sighand_dfl_nop,  // 17 - SIGCHLD (ignore)
        sighand_dfl_cont, // 18 - SIGCONT (continue, if stopped)
        sighand_dfl_stop, // 19 - SIGSTOP (stop, cannot catch/ignore)
        sighand_dfl_stop, // 20 - SIGTSTP (stop)
        sighand_dfl_stop, // 21 - SIGTTIN (stop)
        sighand_dfl_stop, // 22 - SIGTTOU (stop)
        sighand_dfl_nop,  // 23 - SIGURG (ignore)
        sighand_dfl_dump, // 24 - SIGXCPU (terminate + core)
        sighand_dfl_dump, // 25 - SIGXFSZ (terminate + core)
        sighand_dfl_term, // 26 - SIGVTALRM (terminate)
        sighand_dfl_term, // 27 - SIGPROF (terminate)
        sighand_dfl_nop,  // 28 - SIGWINCH (ignore)
        sighand_dfl_nop,  // 29 - SIGIO/SIGPOLL (ignore)
        sighand_dfl_term, // 30 - SIGPWR (terminate)
        sighand_dfl_dump, // 31 - SIGSYS (terminate + core)

        // 32 - 63 Realtime signals, NOP by default
        sighand_dfl_nop, // 32 - SIGRTMIN
        sighand_dfl_nop, // 33 - SIGRTMIN+1
        sighand_dfl_nop, // 34 - SIGRTMIN+2
        sighand_dfl_nop, // 35 - etc.
        sighand_dfl_nop, // 36
        sighand_dfl_nop, // 37
        sighand_dfl_nop, // 38
        sighand_dfl_nop, // 39
        sighand_dfl_nop, // 40
        sighand_dfl_nop, // 41
        sighand_dfl_nop, // 42
        sighand_dfl_nop, // 43
        sighand_dfl_nop, // 44
        sighand_dfl_nop, // 45
        sighand_dfl_nop, // 46
        sighand_dfl_nop, // 47
        sighand_dfl_nop, // 48
        sighand_dfl_nop, // 49
        sighand_dfl_nop, // 50
        sighand_dfl_nop, // 51
        sighand_dfl_nop, // 52
        sighand_dfl_nop, // 53
        sighand_dfl_nop, // 54
        sighand_dfl_nop, // 55
        sighand_dfl_nop, // 56
        sighand_dfl_nop, // 57
        sighand_dfl_nop, // 58
        sighand_dfl_nop, // 59
        sighand_dfl_nop, // 60
        sighand_dfl_nop, // 61
        sighand_dfl_nop, // 62
        sighand_dfl_nop, // 63
        sighand_dfl_nop, // 64 - SIGRTMAX
    };

    __attribute__((weak)) void TaskSignalHandlers::sighand_dfl_nop(Task &, const sys_siginfo &, const sys_sigaction &) {}
    __attribute__((weak)) void TaskSignalHandlers::sighand_dfl_term(Task &, const sys_siginfo &, const sys_sigaction &) {}
    __attribute__((weak)) void TaskSignalHandlers::sighand_dfl_dump(Task &, const sys_siginfo &, const sys_sigaction &) {}
    __attribute__((weak)) void TaskSignalHandlers::sighand_dfl_stop(Task &, const sys_siginfo &, const sys_sigaction &) {}
    __attribute__((weak)) void TaskSignalHandlers::sighand_dfl_cont(Task &, const sys_siginfo &, const sys_sigaction &) {}
    __attribute__((weak)) void TaskSignalHandlers::sighand_ign(Task &, const sys_siginfo &, const sys_sigaction &) {}
} // namespace Hamster
