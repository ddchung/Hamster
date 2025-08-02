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
            current_task->memory->obj.memory.memcpy(suid_loc, &current_task->process->obj.suid, sizeof(uint32_t));

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
            current_task->memory->obj.memory.memcpy(sgid_loc, &current_task->process->obj.sgid, sizeof(uint32_t));

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
            current_task->process->obj.suid = uid;
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
            current_task->process->obj.sgid = gid;
        }

        return 0;
    }

    int32_t sys_setreuid(uint32_t ruid, uint32_t euid)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        if (ruid != (uint32_t)-1)
        {
            // If the process isn't privileged, and new new real UID is not the same as either
            // the old real UID or effective UID, fail with EPERM
            if (current_task->process->obj.uid != 0 && ruid != current_task->process->obj.uid &&
                ruid != current_task->process->obj.euid)
            {
                return -EPERM;
            }

            current_task->process->obj.uid = ruid;
        }

        if (euid != (uint32_t)-1)
        {
            // If the process isn't privileged, and new effective UID is not the same as one of:
            // - The old real UID
            // - The effective UID
            // - The saved set-user ID
            // Then fail with EPERM
            if (current_task->process->obj.uid != 0 && euid != current_task->process->obj.uid &&
                euid != current_task->process->obj.euid && euid != current_task->process->obj.suid)
            {
                return -EPERM;
            }
            current_task->process->obj.euid = euid;
        }

        return 0;
    }

    int32_t sys_setregid(uint32_t rgid, uint32_t egid)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        if (rgid != (uint32_t)-1)
        {
            // If the process isn't privileged, and new real GID is not the same as either
            // the old real GID or effective GID, fail with EPERM
            if (current_task->process->obj.gid != 0 && rgid != current_task->process->obj.gid &&
                rgid != current_task->process->obj.egid)
            {
                return -EPERM;
            }

            current_task->process->obj.gid = rgid;
        }

        if (egid != (uint32_t)-1)
        {
            // If the process isn't privileged, and new effective GID is not the same as one of:
            // - The old real GID
            // - The effective GID
            // - The saved set-group ID
            // Then fail with EPERM
            if (current_task->process->obj.gid != 0 && egid != current_task->process->obj.gid &&
                egid != current_task->process->obj.egid && egid != current_task->process->obj.sgid)
            {
                return -EPERM;
            }
            current_task->process->obj.egid = egid;
        }

        return 0;
    }

    int32_t sys_setresuid(uint32_t ruid, uint32_t euid, uint32_t suid)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        // Unprivileged processes can only set the real, effective, and saved user IDs to one of
        // - The old real user ID
        // - The old effective user ID
        // - The old saved user ID

        if (ruid != (uint32_t)-1)
        {
            if (current_task->process->obj.uid != 0 && ruid != current_task->process->obj.uid &&
                ruid != current_task->process->obj.euid && ruid != current_task->process->obj.suid)
            {
                return -EPERM;
            }
            current_task->process->obj.uid = ruid;
        }

        if (euid != (uint32_t)-1)
        {
            if (current_task->process->obj.uid != 0 && euid != current_task->process->obj.uid &&
                euid != current_task->process->obj.euid && euid != current_task->process->obj.suid)
            {
                return -EPERM;
            }
            current_task->process->obj.euid = euid;
        }

        if (suid != (uint32_t)-1)
        {
            if (current_task->process->obj.uid != 0 && suid != current_task->process->obj.uid &&
                suid != current_task->process->obj.euid && suid != current_task->process->obj.suid)
            {
                return -EPERM;
            }
            current_task->process->obj.suid = suid;
        }

        return 0;
    }

    int32_t sys_setresgid(uint32_t rgid, uint32_t egid, uint32_t sgid)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        // same thing as setresuid, but for groups

        if (rgid != (uint32_t)-1)
        {
            if (current_task->process->obj.gid != 0 && rgid != current_task->process->obj.gid &&
                rgid != current_task->process->obj.egid && rgid != current_task->process->obj.sgid)
            {
                return -EPERM;
            }
            current_task->process->obj.gid = rgid;
        }

        if (egid != (uint32_t)-1)
        {
            if (current_task->process->obj.gid != 0 && egid != current_task->process->obj.gid &&
                egid != current_task->process->obj.egid && egid != current_task->process->obj.sgid)
            {
                return -EPERM;
            }
            current_task->process->obj.egid = egid;
        }

        if (sgid != (uint32_t)-1)
        {
            if (current_task->process->obj.gid != 0 && sgid != current_task->process->obj.gid &&
                sgid != current_task->process->obj.egid && sgid != current_task->process->obj.sgid)
            {
                return -EPERM;
            }
            current_task->process->obj.sgid = sgid;
        }

        return 0;
    }

    int32_t sys_getgroups(uint32_t size, uint32_t list_loc)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        auto &groups = current_task->process->obj.supplementary_gids;

        if (size == 0)
        {
            // If size is 0, just return the number of groups
            return groups.size();
        }

        if (list_loc == 0)
        {
            return -EFAULT;
        }

        if (size < groups.size())
        {
            // Buffer too small
            return -EINVAL;
        }

        // Copy the group IDs to the user space
        if (current_task->memory->obj.memory.memcpy(list_loc, groups.data(), groups.size() * sizeof(uint32_t)) < 0)
        {
            return -EFAULT;
        }

        return groups.size();
    }

    int32_t sys_setgroups(uint32_t size, uint32_t list_loc)
    {
        Task *current_task = scheduler.get_current_task();
        assert(current_task != nullptr);

        auto &groups = current_task->process->obj.supplementary_gids;

        if (current_task->process->obj.euid != 0)
        {
            // Only root can set groups
            return -EPERM;
        }

        if (size == 0)
        {
            // If size is 0, just clear the groups
            groups.clear();
            return 0;
        }

        if (list_loc == 0)
        {
            return -EFAULT;
        }

        groups.resize(size);

        if (current_task->memory->obj.memory.memcpy(groups.data(), list_loc, size * sizeof(uint32_t)) < 0)
        {
            return -EFAULT;
        }

        return 0;
    }
} // namespace Hamster
