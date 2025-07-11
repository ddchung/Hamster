// Hamster brk syscall

#include <syscall/syscall.hpp>
#include <process/process.hpp>

namespace Hamster
{
    int sys_brk(Thread &thread)
    {
        uint32_t new_brk = get_arg(thread, 0);
        
        Process *proc = thread.get_process();

        if (new_brk)
            proc->brk = new_brk;

        return set_return(thread, proc->brk);
    }
} // namespace Hamster

