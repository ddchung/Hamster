// Hamster task file descriptor table

#pragma once

#include <process/task_base_fd.hpp>
#include <memory/stl_sequential.hpp>
#include <memory/shared_ptr.hpp>

namespace Hamster
{
    class TaskFDTable
    {
        // Automatic destructor of pointer
        struct TaskFDHolder
        {
            TaskFDHolder(BaseTaskFD *fd)
                : fd(fd) {}
            TaskFDHolder(const TaskFDHolder &) = delete;
            TaskFDHolder &operator=(const TaskFDHolder &) = delete;
            TaskFDHolder(TaskFDHolder &&) = delete;
            TaskFDHolder &operator=(TaskFDHolder &&) = delete;
            ~TaskFDHolder();

            BaseTaskFD *fd;
        };
    public:
        /**
         * @brief Get a BaseTaskFD from a file descriptor
         * @param fd The file descriptor
         * @return The BaseTaskFD, or nullptr on error and set `error`
         * @warning The returned pointer does not own the object. Do not free
         */
        BaseTaskFD *get_fd(int fd) const;

        /**
         * @brief Close a file descriptor
         * @return 0 on success, -1 on error and set `error`
         */
        int close(int fd);

        /**
         * @brief Set a file descriptor
         * @param task_fd The BaseTaskFD object. Will take ownership
         * @param fd The FD to map to. -1 to allocate the next one
         * @return 0 on success, -1 on error
         * @warning This will close a BaseTaskFD if there already is one
         *        * in that slot
         */
        int set_fd(BaseTaskFD *task_fd, int fd = -1);

        /**
         * @brief Shallow-copy a file descriptor (dup)
         * @param fd The file descriptor to duplicate
         * @param new_fd The new slot. Existing files will be closed.
         * @return 0 on success, -1 on error
         */
        int dup(int fd, int new_fd);

        /**
         * @brief Allocate a file descriptor greater than start
         * @param start The starting slot to look
         * @return The file descriptor slot, or -1 on error and set `error`
         * @note This will not put anything in the slot. Use in conjuction with
         *       functions above to create desired effect
         * @note `start` must not be negative
         */
        int allocate_fd(int start = 0);

        /**
         * @brief Close all file descriptors that have the OPEN_CLOEXEC flag
         * This will go through all the file descriptors, call their `get_flags`, and
         * close them if the returned flag contains the OPEN_CLOEXEC bit. Errors are ignord.
         * @note Used as a part of the `exec` system call
         */
        void close_cloexec();

        /**
         * @brief Close all file descriptors
         */
        void clear() { fd_table.clear(); }

    private:
        // *Note*: the shared pointer contains a pointer, since the Hamster
        //         shared pointer cannot contain polymorphic types
        Vector<SharedPtr<TaskFDHolder, size_t, SharedPtrCopyType::SHALLOW>> fd_table;
    };
} // namespace Hamster

