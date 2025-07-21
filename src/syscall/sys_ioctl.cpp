// Hamster ioctl system call implementation

#include <syscall/syscall.hpp>
#include <process/process.hpp>
#include <filesystem/vfs.hpp>
#include <abi/values.hpp>
#include <errno/errno.h>

namespace Hamster
{
    namespace
    {
        // Buffer for IOCTL operations taking a pointer
        uint8_t IOCTL_BUF[512];

        // ioctl operation table, checking whether it takes a pointer or not
        int is_ioctl_pointer(int request)
        {
            switch (request)
            {
            case H_TCGETS:
            case H_TCSETS:
            case H_TCSETSW:
            case H_TCSETSF:
            case H_TCGETA:
            case H_TCSETA:
            case H_TCSETAW:
            case H_TCSETAF:
            case H_TIOCGPGRP:
            case H_TIOCSPGRP:
            case H_TIOCOUTQ:
            case H_TIOCSTI:
            case H_TIOCGWINSZ:
            case H_TIOCSWINSZ:
            case H_TIOCMGET:
            case H_TIOCMBIS:
            case H_TIOCMBIC:
            case H_TIOCMSET:
            case H_TIOCGSOFTCAR:
            case H_TIOCSSOFTCAR:
            case H_FIONREAD:
            case H_TIOCLINUX:
            case H_TIOCGSERIAL:
            case H_TIOCSSERIAL:
            case H_TIOCPKT:
            case H_FIONBIO:
            case H_TIOCSETD:
            case H_TIOCGETD:
                return 1;
            case H_TCSBRK:
            case H_TCXONC:
            case H_TCFLSH:
            case H_TIOCEXCL:
            case H_TIOCNXCL:
            case H_TIOCSCTTY:
            case H_TIOCCONS:
            case H_TIOCNOTTY:
            case H_TCSBRKP:
                return 0;
            default:
                error = ENOTSUP;
                return -1;
            }
        }
    }

    int sys_ioctl(Thread &thread)
    {
        int fd = deref_fd(thread, get_arg(thread, 0));
        int32_t request = get_arg(thread, 1);
        int32_t arg = get_arg(thread, 2);

        if (fd < 0)
        {
            error = EBADF;
            return transfer_error(thread);
        }

        int is_pointer = is_ioctl_pointer(request);
        if (is_pointer < 0)
        {
            return transfer_error(thread);
        }

        IoctlArg ioctl_arg;
        if (is_pointer)
        {
            if (arg == 0)
            {
                error = EFAULT;
                return transfer_error(thread);
            }

            if (thread.get_process()->memory_space.memcpy(IOCTL_BUF, arg, sizeof(IOCTL_BUF)) < 0)
            {
                error = EFAULT;
                return transfer_error(thread);
            }

            ioctl_arg.p = IOCTL_BUF;
        }
        else
        {
            ioctl_arg.i = arg;
        }

        int res = vfs.ioctl(fd, request, ioctl_arg);
        if (res == -1)
        {
            return transfer_error(thread);
        }

        if (is_pointer)
        {
            if (thread.get_process()->memory_space.memcpy(arg, IOCTL_BUF, sizeof(IOCTL_BUF)) < 0)
            {
                error = EFAULT;
                return transfer_error(thread);
            }
        }

        return set_return(thread, res);
    }
} // namespace Hamster

