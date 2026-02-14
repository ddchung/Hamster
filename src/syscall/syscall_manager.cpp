
#include <syscall/syscall_manager.hpp>
#include <syscall/syscall.hpp>
#include <memory/allocator.hpp>
#include <abi/syscall_id.hpp>
#include <logger/logger.hpp>
#include <process/task.hpp>
#include <cassert>

namespace Hamster
{
    int SyscallManager::register_syscall(uint32_t id, const Syscall &syscall)
    {
        assert(syscall.num_args >= 0 && syscall.num_args <= 6);
        if (syscalls.contains(id))
        {
            error = H_EEXIST;
            return -1;
        }
        syscalls[id] = syscall;
        return 0;
    }

    int SyscallManager::unregister_syscall(uint32_t id)
    {
        if (!syscalls.contains(id))
        {
            error = H_ENOENT;
            return -1;
        }
        syscalls.erase(id);
        return 0;
    }

    int32_t SyscallManager::do_syscall(uint32_t id, Task &task, uint32_t *args)
    {
        const Syscall *sys = nullptr;
        
        decltype(syscalls)::const_iterator it = syscalls.find(id);
        if (it != syscalls.end())
            sys = &it->second;
        else if ((it = default_syscalls.find(id)) != default_syscalls.end())
            sys = &it->second;
        else
            return -H_ENOSYS;
        
        {
            auto log = logger("syscall", "do_syscall", Logger::LEVEL_DEBUG);
            log << "TID " << Logger::COLOR_CYAN << task.get_tid() << Logger::COLOR_DEFAULT << ": "
                << Logger::COLOR_GREEN << sys->name << Logger::COLOR_DEFAULT << "(";
            for (int i = 0; i < sys->num_args; ++i)
            {
                const SyscallArg &arg = sys->args[i];
                if (i > 0)
                    log << ", ";
                log << Logger::COLOR_YELLOW << arg.name << Logger::COLOR_DEFAULT << "=" << Logger::COLOR_MAGENTA;
                switch (arg.type)
                {
                case SyscallArg::ARG_NONE:
                    log << "(none)";
                    break;
                case SyscallArg::ARG_INT:
                    log << (int32_t)args[i];
                    break;
                case SyscallArg::ARG_UINT:
                    log << (uint32_t)args[i];
                    break;
                case SyscallArg::ARG_STR: {
                    const char *str = task.mem_get_string(args[i]);
                    if (str)
                        log << "\"" << str << "\"";
                    else
                        log << "(invalid string)";
                    dealloc(str);
                    break;
                }
                case SyscallArg::ARG_PTR:
                    log << (void *)(uintptr_t)args[i];
                    break;
                }
                log << Logger::COLOR_DEFAULT;
            }
            log << ")";
        } // end logger scope

        int32_t res;
        switch (sys->num_args)
        {
        case 0:
            res = ((int32_t (*)(Task &))sys->handler)(task);
            break;
        case 1:
            res = ((int32_t (*)(Task &, uint32_t))sys->handler)(task, args[0]);
            break;
        case 2:
            res = ((int32_t (*)(Task &, uint32_t, uint32_t))sys->handler)(task, args[0], args[1]);
            break;
        case 3:
            res = ((int32_t (*)(Task &, uint32_t, uint32_t, uint32_t))sys->handler)(task, args[0], args[1], args[2]);
            break;
        case 4:
            res = ((int32_t (*)(Task &, uint32_t, uint32_t, uint32_t, uint32_t))sys->handler)(task, args[0], args[1], args[2], args[3]);
            break;
        case 5:
            res = ((int32_t (*)(Task &, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t))sys->handler)(task, args[0], args[1], args[2], args[3], args[4]);
            break;
        case 6:
            res = ((int32_t (*)(Task &, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t))sys->handler)(task, args[0], args[1], args[2], args[3], args[4], args[5]);
            break;
        default:
            assert(!"Invalid number of syscall arguments");
        }

        auto log = logger("syscall", "do_syscall", Logger::LEVEL_DEBUG);
        log << "TID " << Logger::COLOR_CYAN << task.get_tid() << Logger::COLOR_DEFAULT << ": " << Logger::COLOR_GREEN << sys->name << Logger::COLOR_DEFAULT;

        if (task.is_blocking())
            log << " started blocking";
        else
        {
            if (res < 0 && res >= -4096)
                log << Logger::COLOR_RED << " error ";
            else
                log << Logger::COLOR_GREEN << " returned ";
            log << res << Logger::COLOR_DEFAULT;
        }

        return res;
    }

    using enum SyscallID::ID;
    using enum SyscallManager::SyscallArg::Type;

    // Default system call numbers
    const Map<uint32_t, SyscallManager::Syscall> SyscallManager::default_syscalls = {
        //              num_args, args,                                                               name,           handler
        {EXIT,        { 1,       {{ARG_INT, "status"}},                                               "exit",         (void *)sys_exit}},
        {GETPID,      { 0,       {},                                                                  "getpid",       (void *)sys_getpid}},
        {GETTID,      { 0,       {},                                                                  "gettid",       (void *)sys_gettid}},
        {SETPGID,     { 2,       {{ARG_INT, "pid"}, {ARG_INT, "pgid"}},                               "setpgid",      (void *)sys_setpgid}},
        {GETPGID,     { 1,       {{ARG_INT, "pid"}},                                                  "getpgid",      (void *)sys_getpgid}},
        {GETSID,      { 1,       {{ARG_INT, "pid"}},                                                  "getsid",       (void *)sys_getsid}},
        {SETSID,      { 0,       {},                                                                  "setsid",       (void *)sys_setsid}},
        {SCHED_YIELD, { 0,       {},                                                                  "sched_yield",  (void *)sys_sched_yield}},
        {GETPPID,     { 0,       {},                                                                  "getppid",      (void *)sys_getppid}},
        {CLONE,       { 5,       {{ARG_UINT, "flags"}, {ARG_PTR, "stack"}, {ARG_PTR, "ptid"},
                                  {ARG_UINT, "tls"}, {ARG_PTR, "ctid"}},                              "clone",        (void *)sys_clone}},
        {EXECVE,      { 3,       {{ARG_STR, "filename"}, {ARG_PTR, "argv"}, {ARG_PTR, "envp"}},       "execve",       (void *)sys_execve}},
        {EXECVEAT,    { 5,       {{ARG_INT, "dirfd"}, {ARG_STR, "filename"}, {ARG_PTR, "argv"},
                                  {ARG_PTR, "envp"}, {ARG_INT, "flags"}},                             "execveat",     (void *)sys_execveat}},
        {WAITID,      { 5,       {{ARG_INT, "which"}, {ARG_INT, "pid"}, {ARG_PTR, "infop"},
                                  {ARG_INT, "options"}, {ARG_PTR, "rusage"}},                         "waitid",       (void *)sys_waitid}},
        {WAIT4,       { 4,       {{ARG_INT, "pid"}, {ARG_PTR, "status"},
                                  {ARG_INT, "options"}, {ARG_PTR, "rusage"}},                         "wait4",        (void *)sys_wait4}},
        {KILL,        { 2,       {{ARG_INT, "pid"}, {ARG_INT, "sig"}},                                "kill",         (void *)sys_kill}},
        {TGKILL,      { 3,       {{ARG_INT, "tgid"}, {ARG_INT, "tid"}, {ARG_INT, "sig"}},             "tgkill",       (void *)sys_tgkill}},
        {GETRANDOM,   { 3,       {{ARG_PTR, "buf"}, {ARG_UINT, "buflen"},
                                  {ARG_UINT, "flags"}},                                               "getrandom",    (void *)sys_getrandom}},

        {SETUID,      { 1,       {{ARG_UINT, "uid"}},                                                 "setuid",       (void *)sys_setuid}},
        {SETREUID,    { 2,       {{ARG_UINT, "ruid"}, {ARG_UINT, "euid"}},                            "setreuid",     (void *)sys_setreuid}},
        {SETRESUID,   { 3,       {{ARG_UINT, "ruid"}, {ARG_UINT, "euid"},
                                  {ARG_UINT, "suid"}},                                                "setresuid",    (void *)sys_setresuid}},
        {SETGID,      { 1,       {{ARG_UINT, "gid"}},                                                 "setgid",       (void *)sys_setgid}},
        {SETREGID,    { 2,       {{ARG_UINT, "rgid"}, {ARG_UINT, "egid"}},                            "setregid",     (void *)sys_setregid}},
        {SETRESGID,   { 3,       {{ARG_UINT, "rgid"}, {ARG_UINT, "egid"},
                                  {ARG_UINT, "sgid"}},                                                "setresgid",    (void *)sys_setresgid}},

        {OPENAT,      { 4,       {{ARG_INT, "dirfd"}, {ARG_STR, "pathname"},
                                  {ARG_INT, "flags"}, {ARG_UINT, "mode"}},                            "openat",       (void *)sys_openat}},
        {READ,        { 3,       {{ARG_INT, "fd"}, {ARG_PTR, "buf"},
                                  {ARG_UINT, "count"}},                                               "read",         (void *)sys_read}},
        {WRITE,       { 3,       {{ARG_INT, "fd"}, {ARG_PTR, "buf"},
                                  {ARG_UINT, "count"}},                                               "write",        (void *)sys_write}},
        {CLOSE,       { 1,       {{ARG_INT, "fd"}},                                                   "close",        (void *)sys_close}},

        {SENDFILE64,  { 4,       {{ARG_INT, "out_fd"}, {ARG_INT, "in_fd"},
                                  {ARG_PTR, "offset"}, {ARG_UINT, "count"}},                          "sendfile64",   (void *)sys_sendfile64}},
        {SPLICE,      { 6,       {{ARG_INT, "fd_in"}, {ARG_PTR, "off_in"},
                                  {ARG_INT, "fd_out"}, {ARG_PTR, "off_out"},
                                  {ARG_UINT, "len"}, {ARG_UINT, "flags"}},                            "splice",       (void *)sys_splice}},

        {STATFS,      { 3,       {{ARG_STR, "path"}, {ARG_UINT, "size"},
                                  {ARG_PTR, "buf"}},                                                  "statfs",       (void *)sys_statfs}},
        {FSTATFS,     { 3,       {{ARG_INT, "fd"}, {ARG_UINT, "size"},
                                  {ARG_PTR, "buf"}},                                                  "fstatfs",      (void *)sys_fstatfs}},

        {MOUNT,       { 5,       {{ARG_STR, "source"}, {ARG_STR, "target"},
                                  {ARG_STR, "filesystemtype"}, {ARG_UINT, "mountflags"},
                                  {ARG_PTR, "data"}},                                                 "mount",        (void *)sys_mount}},
        {UMOUNT2,     { 2,       {{ARG_STR, "target"}, {ARG_INT, "flags"}},                           "umount2",      (void *)sys_umount2}},

        {FCHOWNAT,    { 5,       {{ARG_INT, "dirfd"}, {ARG_STR, "pathname"},
                                  {ARG_UINT, "owner"}, {ARG_UINT, "group"},
                                  {ARG_INT, "flags"}},                                                "fchownat",     (void *)sys_fchownat}},
        {FCHOWN,      { 3,       {{ARG_INT, "fd"}, {ARG_UINT, "owner"},
                                  {ARG_UINT, "group"}},                                               "fchown",       (void *)sys_fchown}},
        {FCHMODAT,    { 3,       {{ARG_INT, "dirfd"}, {ARG_STR, "pathname"},
                                  {ARG_UINT, "mode"}},                                                "fchmodat",     (void *)sys_fchmodat}},
        {FCHMODAT2,   { 4,       {{ARG_INT, "dirfd"}, {ARG_STR, "pathname"},
                                  {ARG_UINT, "mode"}, {ARG_INT, "flags"}},                            "fchmodat2",    (void *)sys_fchmodat2}},
        {FCHMOD,      { 2,       {{ARG_INT, "fd"}, {ARG_UINT, "mode"}},                               "fchmod",       (void *)sys_fchmod}},

        {FTRUNCATE64, { 3,       {{ARG_INT, "fd"}, {ARG_UINT, "off_high"},
                                  {ARG_UINT, "off_low"}},                                             "ftruncate64",  (void *)sys_ftruncate64}},
        {TRUNCATE64,  { 3,       {{ARG_STR, "path"}, {ARG_UINT, "off_high"},
                                  {ARG_UINT, "off_low"}},                                             "truncate64",   (void *)sys_truncate64}},
        {LLSEEK,      { 5,       {{ARG_INT, "fd"}, {ARG_UINT, "off_high"},
                                  {ARG_UINT, "off_low"}, {ARG_PTR, "result"},
                                  {ARG_INT, "whence"}},                                               "_llseek",      (void *)sys_llseek}},
        {NEWFSTATAT,  { 4,       {{ARG_INT, "dirfd"}, {ARG_STR, "pathname"},
                                  {ARG_PTR, "statbuf"}, {ARG_INT, "flags"}},                          "newfstatat",   (void *)sys_newfstatat}},
        {NEWFSTAT,    { 2,       {{ARG_INT, "fd"}, {ARG_PTR, "statbuf"}},                             "newfstat",     (void *)sys_newfstat}},
        {DUP,         { 1,       {{ARG_INT, "oldfd"}},                                                 "dup",          (void *)sys_dup}},
        {DUP3,        { 3,       {{ARG_INT, "oldfd"}, {ARG_INT, "newfd"},
                                  {ARG_INT, "flags"}},                                                "dup3",         (void *)sys_dup3}},

        {MKDIRAT,     { 3,       {{ARG_INT, "dirfd"}, {ARG_STR, "pathname"},
                                  {ARG_UINT, "mode"}},                                                "mkdirat",      (void *)sys_mkdirat}},
        {UNLINKAT,    { 3,       {{ARG_INT, "dirfd"}, {ARG_STR, "pathname"},
                                  {ARG_INT, "flags"}},                                                "unlinkat",     (void *)sys_unlinkat}},
        {LINKAT,      { 5,       {{ARG_INT, "olddirfd"}, {ARG_STR, "oldpath"},
                                  {ARG_INT, "newdirfd"}, {ARG_STR, "newpath"},
                                  {ARG_INT, "flags"}},                                                "linkat",       (void *)sys_linkat}},
        {RENAMEAT,    { 4,       {{ARG_INT, "olddirfd"}, {ARG_STR, "oldpath"},
                                  {ARG_INT, "newdirfd"}, {ARG_STR, "newpath"}},                       "renameat",     (void *)sys_renameat}},
        {RENAMEAT2,   { 5,       {{ARG_INT, "olddirfd"}, {ARG_STR, "oldpath"},
                                  {ARG_INT, "newdirfd"}, {ARG_STR, "newpath"},
                                  {ARG_UINT, "flags"}},                                               "renameat2",    (void *)sys_renameat2}},

        {GETDENTS64,  { 3,       {{ARG_INT, "fd"}, {ARG_PTR, "dirp"},
                                  {ARG_UINT, "count"}},                                               "getdents64",   (void *)sys_getdents64}},
        {CHDIR,       { 1,       {{ARG_STR, "path"}},                                                  "chdir",        (void *)sys_chdir}},
        {GETCWD,      { 2,       {{ARG_PTR, "buf"}, {ARG_UINT, "size"}},                              "getcwd",       (void *)sys_getcwd}},
        {FACCESSAT,   { 3,       {{ARG_INT, "dirfd"}, {ARG_STR, "pathname"},
                                  {ARG_INT, "mode"}},                                                 "faccessat",    (void *)sys_faccessat}},
        {FACCESSAT2,  { 4,       {{ARG_INT, "dirfd"}, {ARG_STR, "pathname"},
                                  {ARG_INT, "mode"}, {ARG_INT, "flags"}},                             "faccessat2",   (void *)sys_faccessat2}},

        {PIPE2,       { 2,       {{ARG_PTR, "pipefd"}, {ARG_INT, "flags"}},                           "pipe2",        (void *)sys_pipe2}},
        {BRK,         { 1,       {{ARG_PTR, "addr"}},                                                 "brk",          (void *)sys_brk}},
        {MMAP2,       { 6,       {{ARG_PTR, "addr"}, {ARG_UINT, "length"},
                                  {ARG_UINT, "prot"}, {ARG_UINT, "flags"},
                                  {ARG_INT, "fd"}, {ARG_UINT, "offset"}},                            "mmap2",        (void *)sys_mmap2}},
        {MREMAP,      { 5,       {{ARG_PTR, "old_addr"}, {ARG_UINT, "old_size"},
                                  {ARG_UINT, "new_size"}, {ARG_UINT, "flags"},
                                  {ARG_PTR, "new_addr"}},                                             "mremap",       (void *)sys_mremap}},
        {MUNMAP,      { 2,       {{ARG_PTR, "addr"}, {ARG_UINT, "length"}},                           "munmap",       (void *)sys_munmap}},
        {MPROTECT,    { 3,       {{ARG_PTR, "addr"}, {ARG_UINT, "len"},
                                  {ARG_INT, "prot"}},                                                 "mprotect",     (void *)sys_mprotect}},

        {STATX,       { 5,       {{ARG_INT, "dirfd"}, {ARG_STR, "pathname"},
                                  {ARG_INT, "flags"}, {ARG_UINT, "mask"},
                                  {ARG_PTR, "statxbuf"}},                                             "statx",        (void *)sys_statx}},
        {READLINKAT,  { 4,       {{ARG_INT, "dirfd"}, {ARG_STR, "pathname"},
                                  {ARG_PTR, "buf"}, {ARG_UINT, "bufsiz"}},                            "readlinkat",   (void *)sys_readlinkat}},
        {SYMLINKAT,   { 3,       {{ARG_STR, "target"}, {ARG_INT, "newdirfd"},
                                  {ARG_STR, "linkpath"}},                                             "symlinkat",    (void *)sys_symlinkat}},

        {GETUID,      { 0,       {},                                                                  "getuid",       (void *)sys_getuid}},
        {GETEUID,     { 0,       {},                                                                  "geteuid",      (void *)sys_geteuid}},
        {GETRESUID,   { 3,       {{ARG_PTR, "ruid"}, {ARG_PTR, "euid"},
                                  {ARG_PTR, "suid"}},                                                 "getresuid",    (void *)sys_getresuid}},
        {GETGID,      { 0,       {},                                                                  "getgid",       (void *)sys_getgid}},
        {GETEGID,     { 0,       {},                                                                  "getegid",      (void *)sys_getegid}},
        {GETRESGID,   { 3,       {{ARG_PTR, "rgid"}, {ARG_PTR, "egid"},
                                  {ARG_PTR, "sgid"}},                                                 "getresgid",    (void *)sys_getresgid}},
        {GETGROUPS,   { 2,       {{ARG_UINT, "size"}, {ARG_PTR, "list"}},                             "getgroups",    (void *)sys_getgroups}},
        {SETGROUPS,   { 2,       {{ARG_UINT, "size"}, {ARG_PTR, "list"}},                             "setgroups",    (void *)sys_setgroups}},

        {IOCTL,       { 3,       {{ARG_INT, "fd"}, {ARG_INT, "request"},
                                  {ARG_PTR, "arg"}},                                                  "ioctl",        (void *)sys_ioctl}},
        {FCNTL64,     { 3,       {{ARG_INT, "fd"}, {ARG_INT, "cmd"},
                                  {ARG_PTR, "arg"}},                                                  "fcntl64",      (void *)sys_fcntl64}},
        {PRCTL,       { 5,       {{ARG_INT, "option"}, {ARG_PTR, "arg2"},
                                  {ARG_PTR, "arg3"}, {ARG_PTR, "arg4"},
                                  {ARG_PTR, "arg5"}},                                                 "prctl",        (void *)sys_prctl}},

        {EXIT_GROUP,  { 1,       {{ARG_INT, "status"}},                                               "exit_group",   (void *)sys_exit_group}},

        {RT_SIGACTION,{ 4,       {{ARG_INT, "signum"}, {ARG_PTR, "act"},
                                  {ARG_PTR, "oldact"}, {ARG_UINT, "sigsetsize"}},                     "rt_sigaction", (void *)sys_rt_sigaction}},
        {RT_SIGPENDING,{2,       {{ARG_PTR, "set"}, {ARG_UINT, "sigsetsize"}},                        "rt_sigpending",(void *)sys_rt_sigpending}},
        {RT_SIGPROCMASK,{4,      {{ARG_INT, "how"}, {ARG_PTR, "set"},
                                  {ARG_PTR, "oldset"}, {ARG_UINT, "sigsetsize"}},                     "rt_sigprocmask",(void *)sys_rt_sigprocmask}},
        {RT_SIGQUEUEINFO,{3,     {{ARG_INT, "tgid"}, {ARG_INT, "sig"},
                                  {ARG_PTR, "info"}},                                                 "rt_sigqueueinfo",(void *)sys_rt_sigqueueinfo}},
        {RT_SIGRETURN,{ 0,       {},                                                                  "rt_sigreturn", (void *)sys_rt_sigreturn}},
        {RT_SIGSUSPEND,{2,       {{ARG_PTR, "unewset"}, {ARG_UINT, "sigsetsize"}},                    "rt_sigsuspend",(void *)sys_rt_sigsuspend}},
        {RT_SIGTIMEDWAIT_TIME64,{4,{{ARG_PTR, "set"}, {ARG_PTR, "info"},
                                  {ARG_PTR, "timeout"}, {ARG_UINT, "sigsetsize"}},                    "rt_sigtimedwait_time64",(void *)sys_rt_sigtimedwait_time64}},
        {RT_TGSIGQUEUEINFO,{4,   {{ARG_INT, "tgid"}, {ARG_INT, "tid"},
                                  {ARG_INT, "sig"}, {ARG_PTR, "info"}},                               "rt_tgsigqueueinfo",(void *)sys_rt_tgsigqueueinfo}},

        {UNAME,       { 1,       {{ARG_PTR, "buf"}},                                                  "uname",        (void *)sys_uname}},
        {PSELECT6_TIME64,{6,     {{ARG_INT, "nfds"}, {ARG_PTR, "readfds"},
                                  {ARG_PTR, "writefds"}, {ARG_PTR, "exceptfds"},
                                  {ARG_PTR, "timeout"}, {ARG_PTR, "sigmask"}},                        "pselect6_time64",(void *)sys_pselect6_time64}},
        {FSYNC,       { 1,       {{ARG_INT, "fd"}},                                                   "fsync",        (void *)sys_fsync}},
        {FDATASYNC,   { 1,       {{ARG_INT, "fd"}},                                                   "fdatasync",    (void *)sys_fdatasync}},

        {CLOCK_GETRES_TIME64,{2, {{ARG_INT, "clock_id"}, {ARG_PTR, "res"}},                           "clock_getres_time64",(void *)sys_clock_getres_time64}},
        {CLOCK_GETTIME64,{2,     {{ARG_INT, "clock_id"}, {ARG_PTR, "tp"}},                            "clock_gettime64",(void *)sys_clock_gettime64}},
        {CLOCK_NANOSLEEP_TIME64,{4,{{ARG_INT, "clock_id"}, {ARG_INT, "flags"},
                                  {ARG_PTR, "req"}, {ARG_PTR, "rem"}},                                "clock_nanosleep_time64",(void *)sys_clock_nanosleep_time64}},
        {CLOCK_SETTIME64,{2,     {{ARG_INT, "clock_id"}, {ARG_PTR, "tp"}},                            "clock_settime64",(void *)sys_clock_settime64}},

        {CHROOT,      { 1,       {{ARG_STR, "path"}},                                                 "chroot",       (void *)sys_chroot}},
        {CLOSE_RANGE, { 3,       {{ARG_UINT, "first"}, {ARG_UINT, "last"},
                                  {ARG_UINT, "flags"}},                                               "close_range",  (void *)sys_close_range}},
        {COPY_FILE_RANGE,{6,     {{ARG_INT, "fd_in"}, {ARG_PTR, "off_in"},
                                  {ARG_INT, "fd_out"}, {ARG_PTR, "off_out"},
                                  {ARG_UINT, "len"}, {ARG_UINT, "flags"}},                            "copy_file_range",(void *)sys_copy_file_range}},

        {GET_ROBUST_LIST,{3,     {{ARG_INT, "pid"}, {ARG_PTR, "head_ptr"},
                                  {ARG_PTR, "len"}},                                                  "get_robust_list",(void *)sys_get_robust_list}},
        {SET_ROBUST_LIST,{2,     {{ARG_PTR, "head"}, {ARG_UINT, "len"}},                              "set_robust_list",(void *)sys_set_robust_list}},
        {SET_TID_ADDRESS,{1,     {{ARG_PTR, "tidptr"}},                                               "set_tid_address",(void *)sys_set_tid_address}},

        {PREAD64,     { 6,       {{ARG_INT, "fd"}, {ARG_PTR, "buf"},
                                  {ARG_UINT, "count"}, {ARG_UINT, "pad"},
                                  {ARG_UINT, "off_low"}, {ARG_UINT, "off_high"}},                     "pread64",      (void *)sys_pread64}},
        {PREADV,      { 5,       {{ARG_INT, "fd"}, {ARG_PTR, "vec"},
                                  {ARG_UINT, "vlen"}, {ARG_UINT, "pos_low"},
                                  {ARG_UINT, "pos_high"}},                                            "preadv",       (void *)sys_preadv}},
        {PREADV2,     { 6,       {{ARG_INT, "fd"}, {ARG_PTR, "vec"},
                                  {ARG_UINT, "vlen"}, {ARG_UINT, "pos_low"},
                                  {ARG_UINT, "pos_high"}, {ARG_UINT, "flags"}},                       "preadv2",      (void *)sys_preadv2}},
        {PWRITE64,    { 6,       {{ARG_INT, "fd"}, {ARG_PTR, "buf"},
                                  {ARG_UINT, "count"}, {ARG_UINT, "pad"},
                                  {ARG_UINT, "off_low"}, {ARG_UINT, "off_high"}},                     "pwrite64",     (void *)sys_pwrite64}},
        {PWRITEV,     { 5,       {{ARG_INT, "fd"}, {ARG_PTR, "vec"},
                                  {ARG_UINT, "vlen"}, {ARG_UINT, "pos_low"},
                                  {ARG_UINT, "pos_high"}},                                            "pwritev",      (void *)sys_pwritev}},
        {PWRITEV2,    { 6,       {{ARG_INT, "fd"}, {ARG_PTR, "vec"},
                                  {ARG_UINT, "vlen"}, {ARG_UINT, "pos_low"},
                                  {ARG_UINT, "pos_high"}, {ARG_UINT, "flags"}},                       "pwritev2",     (void *)sys_pwritev2}},

        {RISCV_FLUSH_ICACHE,{3,  {{ARG_PTR, "start"}, {ARG_PTR, "end"},
                                  {ARG_UINT, "flags"}},                                               "riscv_flush_icache",(void *)sys_riscv_flush_icache}},

        {SIGALTSTACK, { 2,       {{ARG_PTR, "ss"}, {ARG_PTR, "old_ss"}},                              "sigaltstack",  (void *)sys_sigaltstack}},
        {FUTEX_TIME64,{ 6,       {{ARG_PTR, "uaddr"}, {ARG_INT, "futex_op"},
                                  {ARG_UINT, "val"}, {ARG_PTR, "timeout"},
                                  {ARG_PTR, "uaddr2"}, {ARG_UINT, "val3"}},                           "futex_time64", (void *)sys_futex_time64}},
        {READV,       { 3,       {{ARG_INT, "fd"}, {ARG_PTR, "vec"},
                                  {ARG_UINT, "vlen"}},                                                "readv",        (void *)sys_readv}},
        {WRITEV,      { 3,       {{ARG_INT, "fd"}, {ARG_PTR, "vec"},
                                  {ARG_UINT, "vlen"}},                                                "writev",       (void *)sys_writev}},
        {TKILL,       { 2,       {{ARG_INT, "tid"}, {ARG_INT, "sig"}},                                "tkill",        (void *)sys_tkill}},
        {PPOLL_TIME64,{ 5,       {{ARG_PTR, "fds"}, {ARG_UINT, "nfds"},
                                  {ARG_PTR, "timeout"}, {ARG_PTR, "sigmask"},
                                  {ARG_UINT, "sigset_size"}},                                         "ppoll_time64", (void *)sys_ppoll_time64}},

        {SOCKET,      { 3,       {{ARG_INT, "domain"}, {ARG_INT, "type"},
                                  {ARG_INT, "protocol"}},                                             "socket",       (void *)sys_socket}},
        {BIND,        { 3,       {{ARG_INT, "sockfd"}, {ARG_PTR, "addr"},
                                  {ARG_UINT, "addrlen"}},                                             "bind",         (void *)sys_bind}},
        {LISTEN,      { 2,       {{ARG_INT, "sockfd"}, {ARG_INT, "backlog"}},                         "listen",       (void *)sys_listen}},
        {ACCEPT,      { 3,       {{ARG_INT, "sockfd"}, {ARG_PTR, "addr"},
                                  {ARG_PTR, "addrlen"}},                                              "accept",       (void *)sys_accept}},
        {ACCEPT4,     { 4,       {{ARG_INT, "sockfd"}, {ARG_PTR, "addr"},
                                  {ARG_PTR, "addrlen"}, {ARG_INT, "flags"}},                          "accept4",      (void *)sys_accept4}},
        {CONNECT,     { 3,       {{ARG_INT, "sockfd"}, {ARG_PTR, "addr"},
                                  {ARG_UINT, "addrlen"}},                                             "connect",      (void *)sys_connect}},
        {SENDTO,      { 6,       {{ARG_INT, "sockfd"}, {ARG_PTR, "buf"},
                                  {ARG_UINT, "len"}, {ARG_INT, "flags"},
                                  {ARG_PTR, "dest_addr"}, {ARG_UINT, "addrlen"}},                     "sendto",       (void *)sys_sendto}},
        {RECVFROM,    { 6,       {{ARG_INT, "sockfd"}, {ARG_PTR, "buf"},
                                  {ARG_UINT, "len"}, {ARG_INT, "flags"},
                                  {ARG_PTR, "src_addr"}, {ARG_PTR, "addrlen"}},                       "recvfrom",     (void *)sys_recvfrom}},
        {SETSOCKOPT,  { 5,       {{ARG_INT, "sockfd"}, {ARG_INT, "level"},
                                  {ARG_INT, "optname"}, {ARG_PTR, "optval"},
                                  {ARG_UINT, "optlen"}},                                              "setsockopt",   (void *)sys_setsockopt}},
        {GETSOCKOPT,  { 5,       {{ARG_INT, "sockfd"}, {ARG_INT, "level"},
                                  {ARG_INT, "optname"}, {ARG_PTR, "optval"},
                                  {ARG_PTR, "optlen"}},                                               "getsockopt",   (void *)sys_getsockopt}},
        {GETSOCKNAME, { 3,       {{ARG_INT, "sockfd"}, {ARG_PTR, "addr"},
                                  {ARG_PTR, "addrlen"}},                                              "getsockname",  (void *)sys_getsockname}},
        {GETPEERNAME, { 3,       {{ARG_INT, "sockfd"}, {ARG_PTR, "addr"},
                                  {ARG_PTR, "addrlen"}},                                              "getpeername",  (void *)sys_getpeername}},
        {SHUTDOWN,    { 2,       {{ARG_INT, "sockfd"}, {ARG_INT, "how"}},                             "shutdown",     (void *)sys_shutdown}},
        {SENDMSG,     { 3,       {{ARG_INT, "sockfd"}, {ARG_PTR, "msg"},
                                  {ARG_INT, "flags"}},                                                "sendmsg",      (void *)sys_sendmsg}},
        {RECVMSG,     { 3,       {{ARG_INT, "sockfd"}, {ARG_PTR, "msg"},
                                  {ARG_INT, "flags"}},                                                "recvmsg",      (void *)sys_recvmsg}},
    };
} // namespace Hamster

