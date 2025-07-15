// Hamster getdents64 system call

#include <syscall/syscall.hpp>
#include <abi/syscall_id.hpp>
#include <abi/structs.hpp>
#include <process/process.hpp>
#include <filesystem/vfs.hpp>
#include <memory/allocator.hpp>
#include <cassert>
#include <errno/errno.h>
#include <cstring>

namespace Hamster
{
    // utility for printing an array of strings
    void print_string_array(const char * const *array)
    {
        if (!array)
            return;

        for (const char * const *entry = array; *entry != nullptr; ++entry)
        {
            printf("%s\n", *entry);
        }
    }

    int sys_getdents64(Thread &thread)
    {
        int fd = deref_fd(thread, get_arg(thread, 0));
        uint32_t dirp_addr = get_arg(thread, 1);
        uint32_t count = get_arg(thread, 2);

        if (fd < 0)
        {
            error = EBADF;
            return transfer_error(thread);
        }

        if (!dirp_addr)
        {
            error = EFAULT;
            return transfer_error(thread);
        }

        // counter for return value (number of bytes put in the buffer)
        uint32_t bytes_read = 0;

        char * const *list = vfs.list(fd, count);

        if (!list)
        {
            return transfer_error(thread);
        }

        bool ok = true;

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
            if (vfs.lstatat(fd, name, &st) < 0)
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
            if (thread.get_process()->memory_space.memcpy(dirp_addr + bytes_read, dirent, sizeof(sys_dirent) + name_len) < 0)
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
        // do this in a seperate loop because the first one may break early
        for (const char * const *entry = list; *entry != nullptr; ++entry)
        {
            dealloc(*entry);
        }
        dealloc(list);

        if (!ok)
        {
            return transfer_error(thread);
        }

        // Set the return value to the number of bytes read
        return set_return(thread, bytes_read);
    }
} // namespace Hamster

