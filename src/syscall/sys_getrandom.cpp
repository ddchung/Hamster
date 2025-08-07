// Hamster getrandom system call

#include <syscall/syscall.hpp>
#include <process/scheduler.hpp>
#include <errno/errno.h>
#include <stdlib.h>

namespace Hamster
{
    int32_t sys_getrandom(int32_t buf_loc, uint32_t buflen, uint32_t /* flags - not used */)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task);

        if (buflen == 0)
        {
            return 0;
        }

        if (!buf_loc)
        {
            error = EFAULT;
            return cvt_error();
        }

        for (uint32_t i = buf_loc; i < buf_loc + buflen; i++)
        {
            uint8_t random_byte = rand() % 256;

            if (current_task->memory->obj.memory.write_byte(i, random_byte) < 0)
            {
                error = EFAULT;
                return cvt_error();
            }
        }

        return buflen;
    }
} // namespace Hamster

