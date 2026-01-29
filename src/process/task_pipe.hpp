// Hamster unnamed pipes

#pragma once

#include <process/task_base_fd.hpp>
#include <memory/stl_sequential.hpp>
#include <memory/allocator.hpp>
#include <utility>

namespace Hamster
{
    class TaskPipe : public BaseTaskFD
    {
        class TaskPipeData
        {
        public:
            TaskPipeData() = default;
            TaskPipeData(TaskPipe &&) = delete;
            ~TaskPipeData() = default;

            Deque<char> data;
            uint16_t readers = 0;
            uint16_t writers = 0;
        };
        
        TaskPipe() = default;

    public:
        TaskPipe(TaskPipe&&) = delete;
        ~TaskPipe() override;

        // give access to default ctor above
        friend TaskPipe *alloc<TaskPipe>(size_t N);

        /**
         * @brief Make a new pair of pipes
         * @param flags 0, or `OPEN_NONBLOCK`
         * @return A pair of newly allocated pipes, containing a read end and a write end, in that order.
         */
        static std::pair<TaskPipe *, TaskPipe *> make_pair(int flags);

        TaskFDType type() const override { return TaskFDType::Pipe; }

        ssize_t read(void *buf, size_t size);
        ssize_t write(const void *buf, size_t size);
        int64_t seek(int64_t off, int whence);
        int64_t tell();
        int stat(sys_stat *buf);
        int truncate(int64_t size);
        int64_t size();
        int ioctl(int req, IoctlArg arg = IoctlArg());
        int set_flags(int flags);
        int get_flags();
        int poll(int op);
        int sync();
        int datasync();
        int get_vfs_fd();
    
    private:
        TaskPipeData *data;
        int flags;
    };
} // namespace Hamster

