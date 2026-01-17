// Magic filesystem for VFS file descriptors

#pragma once

#include <filesystem/base_file.hpp>
#include <filesystem/vfs.hpp>
#include <memory/allocator.hpp>
#include <errno/errno.h>
#include <cctype>
#include <string>

namespace Hamster
{
    class VfsFdFs : public BaseFilesystem
    {
        class RootDir : public BaseDirectory
        {
        public:
            RootDir(BaseFilesystem *fs) : fs(fs) {}
            
            BaseFile *clone() { return alloc<RootDir>(1, fs); }
            BaseFilesystem *get_filesystem() { return fs; }
            int get_id() const { return 0; } // Only one directory: root
            int stat(sys_stat *buf)
            {
                memset(buf, 0, sizeof(sys_stat));
                buf->mode = 0777 | STAT_IFDIR;
                buf->nlink = 1;
                buf->uid = 0;
                buf->gid = 0;
                return 0;
            }
            int get_mode() { return 0777 | STAT_IFDIR; }
            int get_flags() { return OPEN_RDONLY; }
            int get_uid() { return 0; }
            int get_gid() { return 0; }
            int chmod(int mode) { error = H_EROFS; return -1; }
            int chown(int uid, int gid) { error = H_EROFS; return -1; }
            int set_flags(int flags) { error = H_EPERM; return -1; }


            char *const *list(size_t count) override { error = H_EPERM; return nullptr; }
            
            BaseFile *get(const char *name, int flags, int mode = 0) override
            {
                if (!name)
                {
                    error = H_EINVAL;
                }

                // Check if name is all digits
                for (const char *it = name; *it; ++it)
                    if (!isdigit(*it))
                    {
                        error = H_ENOENT;
                        return nullptr;
                    }
                
                int fd = atoi(name);

                // file is already duplicate
                BaseFile *file = vfs.get_file(fd);
                if (!file)
                    return nullptr;
                
                if (file->set_flags(flags) < 0)
                {
                    dealloc(file);
                    return nullptr;
                }

                return file;
            }

            BaseRegularFile *mkfile(const char *name, int flags, int mode) override { error = H_EROFS; return nullptr; }
            BaseDirectory *mkdir(const char *name, int flags, int mode) override { error = H_EROFS; return nullptr; }
            BaseSymlink *mksym(const char *name, const char *target) override { error = H_EROFS; return nullptr; }
            int link(BaseFile *file, const char *name) override { error = H_EROFS; return -1; }
            int remove(const char *name) override { error = H_EROFS; return -1; }
        private:
            BaseFilesystem *fs;
        };
    
    public:
        BaseDirectory *open_root(int)
        {
            return alloc<RootDir>(1, this);
        }
    };
} // namespace Hamster

