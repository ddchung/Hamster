// Hamster getuid, geteuid, getgid, getegid system call implementations

#include <syscall/syscall.hpp>
#include <process/process.hpp>

namespace Hamster
{
    int sys_getuid(Thread &thread)
    { return set_return(thread, thread.get_process()->uid); }

    int sys_geteuid(Thread &thread)
    { return set_return(thread, thread.get_process()->euid); }

    int sys_getgid(Thread &thread)
    { return set_return(thread, thread.get_process()->gid); }

    int sys_getegid(Thread &thread)
    { return set_return(thread, thread.get_process()->egid); }
} // namespace Hamster

