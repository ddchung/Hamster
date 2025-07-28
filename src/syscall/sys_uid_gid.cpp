// Hamster uid and gid related system calls

#include <syscall/syscall.hpp>
#include <process/scheduler.hpp>
#include <errno/errno.h>

namespace Hamster
{
    int32_t sys_getuid()
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        return current_task->process->obj.uid;
    }

    int32_t sys_geteuid()
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        return current_task->process->obj.euid;
    }

    int32_t sys_getgid()
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        return current_task->process->obj.gid;
    }

    int32_t sys_getegid()
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        return current_task->process->obj.egid;
    }

    int32_t sys_getresuid(uint32_t ruid_loc, uint32_t euid_loc, uint32_t suid_loc)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        if (ruid_loc)
            current_task->memory->obj.memory.memcpy(ruid_loc, &current_task->process->obj.uid, sizeof(uint32_t));
        if (euid_loc)
            current_task->memory->obj.memory.memcpy(euid_loc, &current_task->process->obj.euid, sizeof(uint32_t));
        if (suid_loc)
            // Note: we don't have a saved user ID in the process structure, so just use EUID for now
            current_task->memory->obj.memory.memcpy(suid_loc, &current_task->process->obj.euid, sizeof(uint32_t));

        return 0;
    }

    int32_t sys_getresgid(uint32_t rgid_loc, uint32_t egid_loc, uint32_t sgid_loc)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        if (rgid_loc)
            current_task->memory->obj.memory.memcpy(rgid_loc, &current_task->process->obj.gid, sizeof(uint32_t));
        if (egid_loc)
            current_task->memory->obj.memory.memcpy(egid_loc, &current_task->process->obj.egid, sizeof(uint32_t));
        if (sgid_loc)
            // Note: we don't have a saved group ID in the process structure, so just use EGID for now
            current_task->memory->obj.memory.memcpy(sgid_loc, &current_task->process->obj.egid, sizeof(uint32_t));

        return 0;
    }

    int32_t sys_setuid(uint32_t uid)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        current_task->process->obj.euid = uid;

        // Check for root
        if (current_task->process->obj.uid == 0)
        {
            // If the process is root, we can set the real UID as well
            current_task->process->obj.uid = uid;
        }

        return 0;
    }

    int32_t sys_setgid(uint32_t gid)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        current_task->process->obj.egid = gid;

        // Check for root
        if (current_task->process->obj.gid == 0)
        {
            // If the process is root, we can set the real GID as well
            current_task->process->obj.gid = gid;
        }

        return 0;
    }

    int32_t sys_setreuid(uint32_t ruid, uint32_t euid)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        current_task->process->obj.uid = ruid;
        current_task->process->obj.euid = euid;

        return 0;
    }

    int32_t sys_setregid(uint32_t rgid, uint32_t egid)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        current_task->process->obj.gid = rgid;
        current_task->process->obj.egid = egid;

        return 0;
    }

    int32_t sys_setresuid(uint32_t ruid, uint32_t euid, uint32_t suid)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        current_task->process->obj.uid = ruid;
        current_task->process->obj.euid = euid;
        (void)suid;

        return 0;
    }

    int32_t sys_setresgid(uint32_t rgid, uint32_t egid, uint32_t sgid)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        current_task->process->obj.gid = rgid;
        current_task->process->obj.egid = egid;
        (void)sgid;

        return 0;
    }

} // namespace Hamster
