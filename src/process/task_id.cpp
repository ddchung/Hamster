// Hamster task pid/tid/pgid/sid/uid/gid/groups functions

#include <process/task.hpp>

namespace Hamster
{
    void Task::get_uid(int *uid, int *euid, int *suid) const
    {
        process->get_uid(uid, euid, suid);
    }

    void Task::get_gid(int *gid, int *egid, int *sgid) const
    {
        process->get_gid(gid, egid, sgid);
    }

    void Task::set_uid(int uid, int euid, int suid)
    {
        process->set_uid(uid, euid, suid);
    }

    void Task::set_gid(int gid, int egid, int sgid)
    {
        process->set_gid(gid, egid, sgid);
    }

    int Task::set_pgid(uint32_t pgid)
    {
        return process->set_pgid(pgid);
    }

    uint32_t Task::get_pid() const
    {
        return process->get_pid();
    }

    uint32_t Task::get_ppid() const
    {
        return process->get_ppid();
    }

    uint32_t Task::get_pgid() const
    {
        return process->get_process_group()->get_pgid();
    }

    uint32_t Task::get_sid() const
    {
        return process->get_process_group()->get_session()->get_sid();
    }

    int Task::set_groups(const Vector<int> &groups)
    {
        return process->set_groups(groups);
    }

    int Task::setsid()
    {
        return process->setsid();
    }

    int Process::set_pgid(uint32_t pgid)
    {
        if (pgid == 0)
            pgid = pid;

        const auto &pgroups = pgroup->get_session()->get_process_groups();

        if (pid == pgid)
        {
            // Make a new process group for ourselves

            if (pgroup->get_pgid() == pgid)
                return 0; // Already in our own pgroup

            pgroup->remove_process(this);
            pgroup.construct(pid, pgroup->get_session());
            pgroup->add_process(this);
            return 0;
        }

        for (ProcessGroup *pg : pgroups)
        {
            if (pg->get_pgid() == pgid)
            {
                // Found existing process group
                pgroup->remove_process(this);
                pgroup.assign(pg->get_shared_ptr(), SharedPtrCopyType::SHALLOW);
                pgroup->add_process(this);
                return 0;
            }
        }

        // not found
        error = H_EPERM;
        return -1;
    }

    int Process::setsid()
    {
        if (pgroup->get_pgid() == pid)
        {
            // already a pgroup leader
            error = H_EPERM;
            return -1;
        }

        // make new process group with new session
        pgroup->remove_process(this);
        pgroup.construct(pid);
        pgroup->add_process(this);

        return 0;
    }

    void Process::get_uid(int *uid, int *euid, int *suid) const
    {
        if (uid)
            *uid = this->uid;
        if (euid)
            *euid = this->euid;
        if (suid)
            *suid = this->suid;
    }

    void Process::get_gid(int *gid, int *egid, int *sgid) const
    {
        if (gid)
            *gid = this->gid;
        if (egid)
            *egid = this->egid;
        if (sgid)
            *sgid = this->sgid;
    }

    void Process::set_uid(int uid, int euid, int suid)
    {
        if (uid != -1)
            this->uid = uid;
        if (euid != -1)
            this->euid = euid;
        if (suid != -1)
            this->suid = suid;
    }

    void Process::set_gid(int gid, int egid, int sgid)
    {
        if (gid != -1)
            this->gid = gid;
        if (egid != -1)
            this->egid = egid;
        if (sgid != -1)
            this->sgid = sgid;
    }

    int Process::set_groups(const Vector<int> &groups)
    {
        this->groups = groups;
        return 0;
    }
} // namespace Hamster

