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

    int Task::send_signal_pgroup(const sys_siginfo &siginfo)
    {
        for (Process *process : process->get_process_group()->get_processes())
            process->get_pending_signals().push(siginfo);
        return 0;
    }

    void Task::signal_all_processes(const sys_siginfo &siginfo)
    {
        Set<uint32_t> signaled;
        
        // Mark PID 1 as already signaled, to prevent signaling it
        signaled.insert(1);

        for (const auto &[tid, task] : tasks)
        {
            uint32_t pid = task->get_pid();
            if (!signaled.contains(pid))
            {
                signaled.insert(pid);

                if (check_can_signal(*task, siginfo) < 0)
                    continue; // can't send signal to this process

                task->send_signal_process(siginfo);
            }
        }
    }

    int Task::check_can_signal(Task &other, const sys_siginfo &siginfo)
    {
        // always allow SIGCONT
        if (siginfo.signo == H_SIGCONT)
            return 0;

        int self_uid, self_euid;
        int other_uid, other_suid;

        get_uid(&self_uid, &self_euid);
        other.get_uid(&other_uid, nullptr, &other_suid);

        if (self_uid != other_uid && self_uid != other_suid
         && self_euid != other_uid && self_euid != other_suid)
        {
            // not allowed
            error = H_EPERM;
            return -1;
        }
        return 0;
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

    void Task::set_signal_mask(const sys_sigset &sigset)
    {
        signal_mask.from_sigset(sigset);
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

    int Task::sigaction(uint8_t signo, sys_sigaction action)
    {
        if (signo < 1 || signo > 64)
        {
            error = H_EINVAL;
            return -1;
        }

        switch (action.handler)
        {
        case H_SIG_DFL:
            return process->get_signal_handlers()->set_default(signo);
        case H_SIG_IGN:
            return process->get_signal_handlers()->set_ignore(signo);
        default:
            // Custom handler
            if (memory->ms.check_executable(action.handler) != 0)
            {
                error = H_EFAULT;
                return -1;
            }

            return process->get_signal_handlers()->set_handler(signo, [](Task &task, const sys_siginfo &info, const sys_sigaction &action) {
                // Defer all userspace signal handlers such that only one runs at a time
                // TODO: nested signals
                if (task.is_handling_signal)
                {
                    // re-send
                    task.send_signal(info);
                    return;
                }

                task.is_handling_signal = true;
                task.signal_saved_state = task.save_state();

                // Mask signal if needed
                if ((action.flags & H_SA_NODEFER) == 0)
                    task.signal_mask.block(info.signo);
                
                task.signal_mask.block(action.mask);

                auto &sp = task.get_emulator().x[2];

                // align stack pointer
                sp &= ~0xF;

                if (action.flags & H_SA_RESTORER)
                    task.get_emulator().x[1] = action.restorer; // ra - return address
                else
                {
                    // Push restorer onto stack
                    sp -= sizeof(H_SIGHAND_TRAMPOLINE);
                    if (task.memcpy(sp, H_SIGHAND_TRAMPOLINE, sizeof(H_SIGHAND_TRAMPOLINE)) < 0)
                        return;
                    task.get_emulator().x[1] = sp; // ra
                }

                // arguments

                task.get_emulator().x[10] = info.signo; // a0 - first argument

                if (action.flags & H_SA_SIGINFO)
                {
                    sp -= sizeof(sys_siginfo);
                    if (task.copy_to_memory(sp, info) < 0)
                        return;
                    task.get_emulator().x[11] = sp; // a1 - second arg

                    sp -= sizeof(sys_ucontext);

                    // ucontext from earlier
                    if (task.copy_to_memory(sp, task.signal_saved_state) < 0)
                        return;
                    task.get_emulator().x[12] = sp; // a2 - third arg
                }

                // Set pc
                task.get_emulator().pc = action.handler;

                // Reset if flagged or fault signal
                if (action.flags & H_SA_RESETHAND || info.signo == H_SIGSEGV || info.signo == H_SIGILL || info.signo == H_SIGFPE)
                    task.process->get_signal_handlers()->set_default(info.signo);
                
                // done
                return;
            }, action);
        }
    }

    sys_sigaction Task::get_sigaction(uint8_t signo)
    {
        sys_sigaction act = {};

        if (process->get_signal_handlers()->is_default(signo))
        {
            act.handler = H_SIG_DFL;
            return act;
        }
        else if (process->get_signal_handlers()->is_ignored(signo))
        {
            act.handler = H_SIG_IGN;
            return act;
        }
        else
        {
            act = process->get_signal_handlers()->get_action(signo);
            return act;
        }
    }

    void Task::sigreturn()
    {
        if (is_handling_signal)
        {
            load_state(signal_saved_state);
            is_handling_signal = false;
        }
    }
} // namespace Hamster

