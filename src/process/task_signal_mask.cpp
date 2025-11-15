// Hamster signal mask

#include <process/task_signal_mask.hpp>
#include <errno/errno.h>

#define SIGNAL_BIT(signo) (1ULL << ((signo) - 1))

namespace Hamster
{
    void TaskSignalMask::block(sys_sigset signals)
    {
        mask |= signals.sig[0];
        mask |= (uint64_t)signals.sig[1] << 32;
    }

    void TaskSignalMask::from_sigset(sys_sigset sigset)
    {
        mask = sigset.sig[0] | (uint64_t) sigset.sig[1] << 32;
    }

    void TaskSignalMask::set_blocked(uint8_t signo, bool blocked)
    {
        if (signo < 1 || signo > 64)
            return;
        if (blocked)
            mask |= SIGNAL_BIT(signo);
        else
            mask &= ~SIGNAL_BIT(signo);
    }

    int TaskSignalMask::check(uint8_t signo) const
    {
        if (signo < 1 || signo > 64)
        {
            error = H_EINVAL;
            return -1;
        }
        return (mask & SIGNAL_BIT(signo)) != 0;
    }

    uint64_t TaskSignalMask::convert(bool invert) const
    {
        return invert ? ~mask : mask;
    }

    sys_sigset TaskSignalMask::to_sigset() const
    {
        sys_sigset set;
        set.sig[0] = mask & 0xFFFFFFFF;
        set.sig[1] = mask >> 32;
        return set;
    }
} // namespace Hamster

