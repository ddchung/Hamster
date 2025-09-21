// Hamster rt_sigaction and rt_sigreturn system calls

#include <syscall/syscall.hpp>
#include <process/scheduler.hpp>
#include <abi/structs.hpp>
#include <abi/values.hpp>
#include <errno/errno.h>
#include <cstring>

namespace Hamster
{
    namespace
    {
        void sighand_user(Task *task, sys_siginfo *siginfo, sys_sigaction *action)
        {
            assert(task);
            assert(siginfo);
            assert(action);

            // Call the signal handler
            // Userspace signature: `void handler(int signo, siginfo_t *info, void *context);`

            auto &memory = task->memory->obj.memory;
            auto &emulator = task->emulator;
            auto &sp = emulator.x[2]; // sp is x2 in RISC-V

            // Generate a ucontext
            sys_ucontext ucontext = {};
            ucontext.flags = 0; // No special flags for now
            ucontext.link = 0; // No link for now
            ucontext.sigmask.sig[0] = ~(task->sig_mask); // Note that we store the inverted mask
            ucontext.sigmask.sig[1] = 0; // Realtime signals not used
            ucontext.context.regs.pc = emulator.pc;
            // Don't store the x0 (zero) register
            memcpy(ucontext.context.regs.regs, emulator.x + 1, sizeof(emulator.x) - sizeof(emulator.x[0]));
            ucontext.context.fpstate.d.fcsr = emulator.fcsr;
            memcpy(ucontext.context.fpstate.d.f, emulator.f, sizeof(emulator.f));

            // Trampoline
            if (action->flags & H_SA_RESTORER)
                emulator.x[1] = action->restorer; // return address
            else
            {
                sp -= sizeof(H_SIGHAND_TRAMPOLINE);
                if (memory.memcpy_alloc(sp, H_SIGHAND_TRAMPOLINE, sizeof(H_SIGHAND_TRAMPOLINE)) < 0)
                    return;
                emulator.x[1] = sp; // return address
            }

            // signo
            emulator.x[10] = siginfo->signo;

            if (action->flags & H_SA_SIGINFO)
            {
                sp -= sizeof(sys_siginfo);
                if (memory.memcpy_alloc(sp, siginfo, sizeof(sys_siginfo)) < 0)
                    return;
                
                // siginfo
                emulator.x[11] = sp;
            }

            // we push the ucontext onto the stack regardless of 
            // whether SA_SIGINFO is set, so that the return code can correctly pop it
            // out to restore state
            sp -= sizeof(sys_ucontext);
            if (memory.memcpy_alloc(sp, &ucontext, sizeof(sys_ucontext)) < 0)
                return;
            
            // ucontext
            if (action->flags & H_SA_SIGINFO)
                emulator.x[12] = sp;

            // Program counter
            emulator.pc = action->handler;

            if (action->flags & H_SA_RESETHAND || siginfo->signo == H_SIGSEGV || siginfo->signo == H_SIGILL)
            {
                // Reset to default handler
                task->process->obj.default_signal(siginfo->signo);
            }

            _trace("Task %d: Calling userspace signal handler at 0x%08x\n", task->tid, action->handler);
        }
    } // namespace

    int32_t sys_rt_sigaction(int32_t signum, uint32_t act_loc, uint32_t oldact_loc, uint32_t sigsetsize)
    {
        if (signum < 1 || signum >= H_SIGRTMAX || signum == H_SIGKILL || signum == H_SIGSTOP || signum == H_SIGCONT)
        {
            errno = H_EINVAL;
            return -1;
        }

        if (sigsetsize != sizeof(sys_sigset))
        {
            errno = H_EINVAL;
            return -1;
        }

        Task *task = scheduler.get_current_task();
        assert(task != nullptr);

        sys_sigaction action = {};

        auto &handler = task->process->obj.signal_handlers->obj.sig_handlers[signum];

        if (oldact_loc)
        {
            // Read the old action
            action = handler.action;

            if (task->copy_to_user(action, oldact_loc) < 0)
            {
                error = H_EFAULT;
                return -1;
            }
        }
        if (act_loc)
        {
            // Set new action
            
            if (task->copy_from_user(action, act_loc) < 0)
            {
                error = H_EFAULT;
                return -1;
            }

            switch (action.handler)
            {
            case H_SIG_DFL:
                // Reset to default handler
                task->process->obj.default_signal(signum);
                break;
            case H_SIG_IGN:
                // Ignore the signal
                task->process->obj.ignore_signal(signum);
                break;
            default:
                // Set a custom handler
                handler.fn = sighand_user;
                break;
            }
            handler.action = action;
        }

        return 0;
    }

    int32_t sys_rt_sigreturn()
    {
        Task *task = scheduler.get_current_task();
        assert(task != nullptr);

        // Restore the signal context
        auto &emulator = task->emulator;
        auto &memory = task->memory->obj.memory;
        auto &sp = emulator.x[2]; // sp is x2 in RISC-V

        // Get the saved context from the stack
        sys_ucontext ucontext = {};
        if (memory.memcpy(&ucontext, sp, sizeof(sys_ucontext)) < 0)
            return -1;

        // Restore the program counter
        emulator.pc = ucontext.context.regs.pc;

        // Restore the general purpose registers
        memcpy(emulator.x + 1, ucontext.context.regs.regs, sizeof(emulator.x) - sizeof(emulator.x[0]));

        // Restore the floating point state
        emulator.fcsr = ucontext.context.fpstate.d.fcsr;
        memcpy(emulator.f, ucontext.context.fpstate.d.f, sizeof(emulator.f));

        // Restore the signal mask
        task->sig_mask = ~ucontext.sigmask.sig[0];

        // Don't move back the stack pointer, as we just restored its original value

        return 0;
    }
} // namespace Hamster

