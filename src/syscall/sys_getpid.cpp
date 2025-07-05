// Hamster getpid syscall

#include <syscall/syscall.hpp>
#include <abi/syscall_id.hpp>
#include <process/process.hpp>
#include <cassert>

namespace Hamster
{
    int sys_getpid(Thread &thread)
    {
        Process *process = thread.get_process();

        assert(process);

        return set_return(thread, process->pid);
    }
}
