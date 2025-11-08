// Hamster task signal functions

#include <process/task.hpp>

namespace Hamster
{
    int Task::send_signal(const sys_siginfo &siginfo)
    {
        return pending_signals.push(siginfo);
    }

    int Task::send_signal_process(const sys_siginfo &siginfo)
    {
        return process->get_pending_signals().push(siginfo);
    }

    size_t Task::pending_signals_size() const
    {
        return pending_signals.size();
    }

    size_t Task::pending_signals_size_process() const
    {
        return process->get_pending_signals().size();
    }

    void Task::block_signal(uint8_t signo)
    {
        signal_mask.block(signo);
    }

    void Task::unblock_signal(uint8_t signo)
    {
        signal_mask.unblock(signo);
    }

    void Task::set_signal_blocked(uint8_t signo, bool blocked)
    {
        signal_mask.set_blocked(signo, blocked);
    }

    int Task::is_signal_blocked(uint8_t signo) const
    {
        return signal_mask.check(signo);
    }

    uint64_t Task::get_signal_mask(bool invert) const
    {
        return signal_mask.convert(invert);
    }

    sys_sigset Task::get_signal_sigset() const
    {
        return signal_mask.to_sigset();
    }

    int Task::is_signal_handler(uint8_t signo)
    {
        return process->get_signal_handlers()->is_handler(signo);
    }

    int Task::is_signal_ignored(uint8_t signo)
    {
        return process->get_signal_handlers()->is_ignored(signo);
    }

    int Task::is_signal_default(uint8_t signo)
    {
        return process->get_signal_handlers()->is_default(signo);
    }

    void TaskSignalHandlers::sighand_dfl_nop(Task &, const sys_siginfo &, const sys_sigaction &)
    {
    }

    void TaskSignalHandlers::sighand_dfl_term(Task &task, const sys_siginfo &siginfo, const sys_sigaction &)
    {
        task.exit_group(make_wait_terminated(siginfo.signo));
    }

    void TaskSignalHandlers::sighand_dfl_dump(Task &task, const sys_siginfo &siginfo, const sys_sigaction &)
    {
        task.exit_group(make_wait_terminated_coredump(siginfo.signo));
    }

    void TaskSignalHandlers::sighand_dfl_stop(Task &task, const sys_siginfo &siginfo, const sys_sigaction &)
    {
        task.pause(siginfo.signo);
    }

    void TaskSignalHandlers::sighand_dfl_cont(Task &task, const sys_siginfo &, const sys_sigaction &)
    {
        task.unpause();
    }

    void TaskSignalHandlers::sighand_ign(Task &, const sys_siginfo &, const sys_sigaction &)
    {
    }
} // namespace Hamster

