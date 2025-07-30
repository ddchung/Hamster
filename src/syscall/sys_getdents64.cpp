// Hamster getdents64 syscall implementation

#include <syscall/syscall.hpp>
#include <filesystem/vfs.hpp>
#include <abi/structs.hpp>
#include <errno/errno.h>
#include <memory/allocator.hpp>
#include <cstring>

namespace Hamster
{
    int sys_getdents64(int32_t fd, uint32_t dirent_loc, uint32_t count)
    {
        Task *task = scheduler.get_current_task();
        assert(task != nullptr);

        int vfs_fd = task->get_vfs_fd(fd);
        if (vfs_fd < 0)
        {
            error = EBADF;
            return cvt_error();
        }

        if (!dirent_loc)
        {
            error = EFAULT; // Bad address
            return cvt_error();
        }

        char *const *list = vfs.list(vfs_fd, count);
        if (!list)
        {
            return cvt_error();
        }

        bool ok = true;
        uint32_t bytes_read = 0;

        for (const char * const *entry = list; *entry != nullptr; ++entry)
        {
            const char *name = *entry;
            size_t name_len = strlen(name);

            if (bytes_read + sizeof(sys_dirent) + name_len > count)
            {
                // Not enough space in the buffer
                break;
            }

            sys_stat st;
            if (vfs.lstatat(vfs_fd, name, &st) < 0)
            {
                // If stat fails, we can skip this entry
                continue;
            }

            // sys_dirent's name field is an array of size 1, so we need to ensure enough space
            // here, we use the our _malloc and _free instead of alloc<T>/dealloc<T> because
            // we need more space than sizeof(sys_dirent)

            // sys_dirent's name[1] accounts for the null terminator, so we only add name_len
            sys_dirent *dirent = (sys_dirent *)_malloc(sizeof(sys_dirent) + name_len);
            assert(dirent != nullptr);

            dirent->ino = st.ino;
            dirent->offset = bytes_read;
            dirent->reclen = sizeof(sys_dirent) + name_len;
            dirent->type = st.mode & STAT_IFMT; // Use the file type from mode
            
            strcpy(dirent->name, name); // Copy the name into the dirent

            // Write the dirent to the user space buffer
            if (task->memory->obj.memory.memcpy(dirent_loc + bytes_read, dirent, sizeof(sys_dirent) + name_len) < 0)
            {
                _free(dirent);
                error = EFAULT;
                ok = false;
                break;
            }

            _free(dirent);
            bytes_read += sizeof(sys_dirent) + name_len;
        }

        // Free the list of entries
        for (const char * const *entry = list; *entry != nullptr; ++entry)
        {
            dealloc(*entry);
        }
        dealloc(list);

        if (!ok)
        {
            return cvt_error();
        }

        return bytes_read; // Return the number of bytes read
    }
} // namespace Hamster

