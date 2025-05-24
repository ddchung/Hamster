// Hamster process

#include <memory/memory_space.hpp>
#include <memory/stl_sequential.hpp>
#include <cstdint>

#pragma once

namespace Hamster
{
    enum class ThreadStatus : uint8_t
    {
        RUNNING,
        ENDED
    };

    class BaseThread;
    class ProcessData;

    /**
     * @brief Thread pause callback. 
     * @note This will be called by the scheduler to determine
     *     * whether the thread should be ticked or not. The thread will be ticked if
     *     * the callback is null after the call to this callback
     */
    using ThreadPauseCallback = void (*)(BaseThread *thread);

    /**
     * @brief Process pause callback.
     * @note This will be called by the scheduler to determine
     *     * whether the threads in the process should be ticked or not. The process will be ticked if
     *     * the callback is null after the call to this callback. Note that if the process is not ticked,
     *     * no pause callbacks in the threads will be called.
     */
    using ProcessPauseCallback = void (*)(ProcessData *proc);

    struct ProcFd
    {
        // Underlying "raw" file descriptor, for use with `Hamster::VFS`
        int fd;

        // File descriptor flags
        // Note that this is not the same as the open flags, see:
        // * https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/fcntl.h.html
        uint32_t fd_flags;
    };

    class BaseThread;
    class Scheduler;

    struct ProcessData
    {
        MemorySpace mem_sp;

        uint32_t pid;
        uint32_t ppid;

        Vector<ProcFd> fds;

        // Weak pointers to the threads
        List<BaseThread *> threads;

        // Pause callback, works the same way as in BaseThread, except that
        // it will pause all threads in the process
        ProcessPauseCallback pause_callback;

        // weak pointer to the scheduler that owns this process
        Scheduler *scheduler;
    };

    class BaseThread
    {
    public:
        BaseThread(ProcessData &proc_data)
            : proc_data(proc_data), status(ThreadStatus::RUNNING), pause_callback(nullptr)
        {
        }

        virtual ~BaseThread() = default;

        ProcessData &get_process_data() { return proc_data; }
        ThreadStatus get_status() const { return status; }
        void set_status(ThreadStatus status) { this->status = status; }

        /**
         * @brief Pause the thread
         * @param callback The pause callback
         * @note Set the callback to nullptr to unpause
         * @note This will overwrite any existing callback
         * @note Please see the comment on ThreadPauseCallback for more information
         */
        void pause(ThreadPauseCallback callback = [](BaseThread *) { /* This will pause until stopped */ })
        { pause_callback = callback; }

        /**
         * @brief Forcibly unpause the thread
         * @note This will not check the callback, as it will just set it to nullptr
         */
        void unpause() { pause_callback = nullptr; }

        /**
         * @brief Tick the thread once
         * @note This should NOT check the `pause_callback`, as it will be checked by the scheduler
         * @note Also ensure that this properly handles all system calls, and use `Hamster::scheduler` for new
         *     * threads and processes
         */
        virtual void tick() = 0;

    protected:
        ProcessData &proc_data;
        ThreadStatus status;
        ThreadPauseCallback pause_callback;
    };
} // namespace Hamster

