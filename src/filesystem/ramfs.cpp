// Ram FS implementaiton

#include <filesystem/base_file.hpp>
#include <filesystem/ramfs.hpp>
#include <memory/allocator.hpp>
#include <memory/memory_space.hpp>
#include <memory/stl_sequential.hpp>
#include <memory/stl_map.hpp>
#include <errno/errno.h>
#include <cstring>
#include <cassert>

namespace Hamster
{
    /* Ram FS Tree */
    namespace
    {
        class RamFsDirectoryNode;

        class RamFsNode
        {
        public:
            virtual ~RamFsNode() = default;

            virtual FileType type() const = 0;

            RamFsNode(RamFsNode &&) = delete;
            RamFsNode(const RamFsNode &) = delete;
            RamFsNode &operator=(RamFsNode &&) = delete;
            RamFsNode &operator=(const RamFsNode &) = delete;

            RamFsNode(int mode, int uid, int gid)
                : mode(mode), uid(uid), gid(gid), refcount(1), filesystem(nullptr)
            {
            }

            int mode;
            int uid, gid;
            uint32_t refcount;
            RamFs *filesystem;
        };

        class RamFsRegularNode : public RamFsNode
        {
        public:
            FileType type() const override { return FileType::Regular; }

            using RamFsNode::RamFsNode;

            MemorySpace data;
            int64_t size = 0;
        };

        class RamFsSpecialNode : public RamFsNode
        {
        public:
            FileType type() const override { return FileType::Special; }

            using RamFsNode::RamFsNode;

            DeviceID device_id;
        };

        class RamFsSymlinkNode : public RamFsNode
        {
        public:
            FileType type() const override { return FileType::Symlink; }

            using RamFsNode::RamFsNode;

            String target;
        };

        class RamFsDirectoryNode : public RamFsNode
        {
        public:
            FileType type() const override { return FileType::Directory; }

            using RamFsNode::RamFsNode;

            // Removes . and ..
            void remove_dir_hardlinks()
            {
                auto it = children.find(".");
                if (it != children.end())
                {
                    it->second->refcount--;
                    children.erase(it);
                }
                it = children.find("..");
                if (it != children.end())
                {
                    it->second->refcount--;
                    children.erase(it);
                }

                // Recurse into children
                for (auto &[name, node] : children)
                {
                    if (node->type() == FileType::Directory)
                    {
                        auto *dir_node = static_cast<RamFsDirectoryNode *>(node);
                        dir_node->remove_dir_hardlinks();
                    }
                }
            }

            ~RamFsDirectoryNode()
            {
                remove_dir_hardlinks();
                for (auto &[name, node] : children)
                {
                    if (name == "." || name == "..")
                        continue; // Skip self and parent references
                    if (node)
                    {
                        node->refcount--;
                        if (node->refcount == 0)
                        {
                            dealloc(node);
                        }
                    }
                }
                children.clear();
            }

            Map<String, RamFsNode *> children;
        };
    } // namespace 

    /* File Handles */
    namespace
    {
        class RamFsNodeHandle
        {
        public:
            RamFsNodeHandle(RamFsNode *node, int flags)
                : node(node), flags(flags)
            {
            }

            ~RamFsNodeHandle() = default;

            BaseFilesystem *get_filesystem()
            {
                if (!node)
                {
                    error = EBADF;
                    return nullptr;
                }

                return node->filesystem;
            }

            int get_id() const
            {
                if (!node)
                {
                    error = EBADF;
                    return -1;
                }

                // this will be different for every file, so we can use the address as an ID
                return (uintptr_t)(node);
            }

            int stat(sys_stat *buf)
            {
                if (!node)
                {
                    error = EBADF;
                    return -1;
                }

                memset(buf, 0, sizeof(sys_stat));

                buf->ino = get_id();
                buf->mode = node->mode & 0777;
                buf->uid = node->uid;
                buf->gid = node->gid;
                buf->size = 0;
                buf->nlink = node->refcount;

                if (node->type() == FileType::Regular)
                {
                    auto *regular_node = static_cast<RamFsRegularNode *>(node);
                    buf->size = regular_node->size;
                    buf->blocks = (regular_node->size + HAMSTER_PAGE_SIZE - 1) / HAMSTER_PAGE_SIZE;
                    buf->blksize = HAMSTER_PAGE_SIZE;
                    buf->mode |= STAT_IFREG;
                }
                else if (node->type() == FileType::Directory)
                {
                    buf->mode |= STAT_IFDIR;
                }
                else if (node->type() == FileType::Symlink)
                {
                    buf->mode |= STAT_IFLNK;
                }
                else if (node->type() == FileType::Special)
                {
                    auto *special_node = static_cast<RamFsSpecialNode *>(node);
                    buf->rdev = special_node->device_id.major << 20 | (special_node->device_id.minor & 0xFFFFF);
                }
                else
                {
                    error = ENOTSUP;
                    return -1;
                }

                return 0;
            }

            int get_mode()
            {
                if (!node)
                {
                    error = EBADF;
                    return -1;
                }

                return node->mode;
            }

            int get_uid()
            {
                if (!node)
                {
                    error = EBADF;
                    return -1;
                }

                return node->uid;
            }

            int get_gid()
            {
                if (!node)
                {
                    error = EBADF;
                    return -1;
                }

                return node->gid;
            }

            int get_flags()
            {
                if (!node)
                {
                    error = EBADF;
                    return -1;
                }

                return flags;
            }

            int chmod(int mode)
            {
                if (!node)
                {
                    error = EBADF;
                    return -1;
                }

                node->mode = mode;
                return 0;
            }

            int chown(int uid, int gid)
            {
                if (!node)
                {
                    error = EBADF;
                    return -1;
                }

                node->uid = uid;
                node->gid = gid;
                return 0;
            }
            int set_flags(int flags)
            {
                if (!node)
                {
                    error = EBADF;
                    return -1;
                }

                this->flags = flags;
                return 0;
            }

            RamFsNode *get_node()
            {
                return node;
            }

        protected:
            RamFsNode *node;
            int flags;
        };

        class RamFsRegularHandle : public BaseRegularFile, public RamFsNodeHandle
        {
        public:
            RamFsRegularHandle(RamFsRegularNode *node, int flags)
                : RamFsNodeHandle(node, flags), offset(0)
            {
            }

            ~RamFsRegularHandle() override = default;

            BaseFilesystem *get_filesystem() override { return RamFsNodeHandle::get_filesystem(); }
            int get_id() const override { return RamFsNodeHandle::get_id(); }
            int stat(sys_stat *buf) override { return RamFsNodeHandle::stat(buf); }
            int get_mode() override { return RamFsNodeHandle::get_mode(); }
            int get_uid() override { return RamFsNodeHandle::get_uid(); }
            int get_gid() override { return RamFsNodeHandle::get_gid(); }
            int get_flags() override { return RamFsNodeHandle::get_flags(); }
            int chmod(int mode) override { return RamFsNodeHandle::chmod(mode); }
            int chown(int uid, int gid) override { return RamFsNodeHandle::chown(uid, gid); }
            int set_flags(int flags) override { return RamFsNodeHandle::set_flags(flags); }

            RamFsRegularHandle *clone() override
            {
                auto *reg_node = get_node();
                if (!reg_node)
                    return nullptr;
                
                auto *new_handle = alloc<RamFsRegularHandle>(1, reg_node, flags);
                new_handle->offset = offset;
                return new_handle;
            }

            ssize_t read(uint8_t *buf, size_t size) override
            {
                auto *reg_node = get_node();
                if (!reg_node)
                    return -1;
                if (offset >= reg_node->size)
                {
                    error = EINVAL;
                    return 0;
                }
                if (offset + (int64_t)size > reg_node->size)
                    size = reg_node->size - offset;
                if (size == 0)
                    return 0;
                
                for (uint64_t addr = offset; addr < (uint64_t)offset + size; ++addr)
                {
                    if (reg_node->data.read_byte(addr, buf[addr - offset]) < 0)
                    {
                        error = EIO;
                        return -1;
                    }
                }
                offset += size;
                return size;
            }

            ssize_t write(const uint8_t *buf, size_t size) override
            {
                auto *reg_node = get_node();
                if (!reg_node)
                    return -1;
                
                /*
                As defined by POSIX:
                    If the O_APPEND flag of the file status flags is set, the file offset 
                    shall be set to the end of the file prior to each write and no intervening 
                    file modification operation shall occur between changing the file offset and 
                    the write operation.
                */
                if (flags & OPEN_APPEND)
                    seek(0, H_SEEK_END);
                
                // See: the comment on the seek function
                if (offset > reg_node->size && reg_node->data.memset(reg_node->size, 0, offset - reg_node->size) < 0)
                {
                    error = EIO;
                    return -1;
                }
                
                for (uint64_t addr = offset; addr < (uint64_t)offset + size; ++addr)
                {
                    if (reg_node->data.write_byte(addr, buf[addr - offset]) < 0)
                    {
                        error = EIO;
                        return -1;
                    }
                }
                offset += size;
                reg_node->size = std::max(reg_node->size, offset);
                return size;
            }

            int64_t seek(int64_t offset, int whence) override
            {
                auto *reg_node = get_node();
                if (!reg_node)
                    return -1;
                
                /*
                Note that this should not check for seeking past the end of the file, as POSIX
                defines:
                    The lseek() function shall allow the file offset to be set beyond the end 
                    of the existing data in the file. If data is later written at this point, 
                    subsequent reads of data in the gap shall return bytes with the value 0 
                    until data is actually written into the gap.
                */

                switch (whence)
                {
                case H_SEEK_SET:
                    if (offset < 0)
                    {
                        error = EINVAL;
                        return -1;
                    }
                    this->offset = offset;
                    break;
                case H_SEEK_CUR:
                    if (this->offset + offset < 0)
                    {
                        error = EINVAL;
                        return -1;
                    }
                    this->offset += offset;
                    break;
                case H_SEEK_END:
                    if (reg_node->size + offset < 0)
                    {
                        error = EINVAL;
                        return -1;
                    }
                    this->offset = reg_node->size + offset;
                    break;
                default:
                    error = EINVAL;
                    return -1;
                }

                return this->offset;
            }

            int64_t tell() override
            {
                return seek(0, H_SEEK_CUR);
            }

            int truncate(int64_t size) override
            {
                auto *reg_node = get_node();
                if (!reg_node)
                    return -1;

                if (size < 0)
                {
                    error = EINVAL;
                    return -1;
                }

                if (size > reg_node->size)
                {
                    // Extend with zeros
                    reg_node->data.memset(reg_node->size, 0, size - reg_node->size);
                }
                else
                {
                    // Deallocate all unused pages starting from `size`
                    for (uint64_t addr = ((size & ~HAMSTER_PAGE_SIZE) + HAMSTER_PAGE_SIZE) & ~HAMSTER_PAGE_SIZE;
                         addr <= ((uint64_t)reg_node->size & ~HAMSTER_PAGE_SIZE);
                         addr += HAMSTER_PAGE_SIZE)
                    {
                        if (reg_node->data.deallocate_page(addr))
                            return -1;
                    }
                }

                reg_node->size = size;
                return 0;
            }

            int64_t size() override
            {
                auto *reg_node = get_node();
                if (!reg_node)
                    return -1;

                return reg_node->size;
            }

        private:
            int64_t offset;

            RamFsRegularNode *get_node()
            {
                if (!node)
                {
                    error = EBADF;
                    return nullptr;
                }
                if (node->type() != FileType::Regular)
                {
                    error = EINVAL;
                    return nullptr;
                }
                return static_cast<RamFsRegularNode *>(node);
            }
        };

        class RamFsSpecialHandle : public BaseSpecialFile, public RamFsNodeHandle
        {
        public:
            RamFsSpecialHandle(RamFsSpecialNode *node, int flags)
                : RamFsNodeHandle(node, flags)
            {
            }

            ~RamFsSpecialHandle() override = default;

            BaseFilesystem *get_filesystem() override { return RamFsNodeHandle::get_filesystem(); }
            int get_id() const override { return RamFsNodeHandle::get_id(); }
            int stat(sys_stat *buf) override { return RamFsNodeHandle::stat(buf); }
            int get_mode() override { return RamFsNodeHandle::get_mode(); }
            int get_uid() override { return RamFsNodeHandle::get_uid(); }
            int get_gid() override { return RamFsNodeHandle::get_gid(); }
            int get_flags() override { return RamFsNodeHandle::get_flags(); }
            int chmod(int mode) override { return RamFsNodeHandle::chmod(mode); }
            int chown(int uid, int gid) override { return RamFsNodeHandle::chown(uid, gid); }
            int set_flags(int flags) override { return RamFsNodeHandle::set_flags(flags); }

            RamFsSpecialHandle *clone() override
            {
                auto *special_node = get_node();
                if (!special_node)
                    return nullptr;
                
                return alloc<RamFsSpecialHandle>(1, special_node, flags);
            }

            DeviceID get_device_id() override
            {
                auto *special_node = get_node();
                if (!special_node)
                    return {0, 0};

                return special_node->device_id;
            }

        private:
            RamFsSpecialNode *get_node()
            {
                if (!node)
                {
                    error = EBADF;
                    return nullptr;
                }
                if (node->type() != FileType::Special)
                {
                    error = EINVAL;
                    return nullptr;
                }
                return static_cast<RamFsSpecialNode *>(node);
            }
        };

        class RamFsSymlinkHandle : public BaseSymlink, public RamFsNodeHandle
        {
        public:
            RamFsSymlinkHandle(RamFsSymlinkNode *node, int flags)
                : RamFsNodeHandle(node, flags)
            {
            }

            ~RamFsSymlinkHandle() override = default;

            BaseFilesystem *get_filesystem() override { return RamFsNodeHandle::get_filesystem(); }
            int get_id() const override { return RamFsNodeHandle::get_id(); }
            int stat(sys_stat *buf) override { return RamFsNodeHandle::stat(buf); }
            int get_mode() override { return RamFsNodeHandle::get_mode(); }
            int get_uid() override { return RamFsNodeHandle::get_uid(); }
            int get_gid() override { return RamFsNodeHandle::get_gid(); }
            int get_flags() override { return RamFsNodeHandle::get_flags(); }
            int chmod(int mode) override { return RamFsNodeHandle::chmod(mode); }
            int chown(int uid, int gid) override { return RamFsNodeHandle::chown(uid, gid); }
            int set_flags(int flags) override { return RamFsNodeHandle::set_flags(flags); }

            RamFsSymlinkHandle *clone() override
            {
                auto *symlink_node = get_node();
                if (!symlink_node)
                    return nullptr;
                
                return alloc<RamFsSymlinkHandle>(1, symlink_node, flags);
            }

            char *get_target() override
            {
                auto *symlink_node = get_node();
                if (!symlink_node)
                    return nullptr;

                const char *target = symlink_node->target.c_str();
                char *result = alloc<char>(strlen(target) + 1);
                strcpy(result, target);
                return result;
            }

            int set_target(const char *target) override
            {
                auto *symlink_node = get_node();
                if (!symlink_node)
                    return -1;

                symlink_node->target = target;
                return 0;
            }
        private:
            RamFsSymlinkNode *get_node()
            {
                if (!node)
                {
                    error = EBADF;
                    return nullptr;
                }
                if (node->type() != FileType::Symlink)
                {
                    error = EINVAL;
                    return nullptr;
                }
                return static_cast<RamFsSymlinkNode *>(node);
            }
        };

        class RamFsDirectoryHandle : public BaseDirectory, public RamFsNodeHandle
        {
        public:
            RamFsDirectoryHandle(RamFsDirectoryNode *node, int flags)
                : RamFsNodeHandle(node, flags), offset(0)
            {
            }

            ~RamFsDirectoryHandle() override = default;

            BaseFilesystem *get_filesystem() override { return RamFsNodeHandle::get_filesystem(); }
            int get_id() const override { return RamFsNodeHandle::get_id(); }
            int stat(sys_stat *buf) override { return RamFsNodeHandle::stat(buf); }
            int get_mode() override { return RamFsNodeHandle::get_mode(); }
            int get_uid() override { return RamFsNodeHandle::get_uid(); }
            int get_gid() override { return RamFsNodeHandle::get_gid(); }
            int get_flags() override { return RamFsNodeHandle::get_flags(); }
            int chmod(int mode) override { return RamFsNodeHandle::chmod(mode); }
            int chown(int uid, int gid) override { return RamFsNodeHandle::chown(uid, gid); }
            int set_flags(int flags) override { return RamFsNodeHandle::set_flags(flags); }

            RamFsDirectoryHandle *clone() override
            {
                auto *dir_node = get_node();
                if (!dir_node)
                    return nullptr;
                
                RamFsDirectoryHandle *new_handle = alloc<RamFsDirectoryHandle>(1, dir_node, flags);
                new_handle->offset = offset;
                return new_handle;
            }

            char * const *list(size_t count /* = SIZE_MAX */) override
            {
                auto *dir_node = get_node();
                if (!dir_node)
                    return nullptr;
                
                if (offset < 0)
                {
                    error = EINVAL;
                    return nullptr;
                }

                if (offset >= (int64_t)dir_node->children.size())
                {
                    char **empty_list = alloc<char *>(1);
                    empty_list[0] = nullptr; // Null-terminate the array
                    return empty_list;
                }

                count = std::min((uint64_t)count, (uint64_t)dir_node->children.size() - offset);

                auto it = dir_node->children.begin();
                std::advance(it, offset);

                char **strings = alloc<char *>(count + 1);
                strings[count] = nullptr; // Null-terminate the array

                for (size_t i = 0; i < count; ++i)
                {
                    strings[i] = alloc<char>(it->first.size() + 1);
                    strcpy(strings[i], it->first.c_str());
                    ++it;
                }

                offset += count;

                return strings;
            }
            
            int64_t seek(int64_t offset, int whence) override
            {
                auto *node = get_node();
                if (!node)
                    return -1;

                switch (whence)
                {
                case H_SEEK_SET:
                    if (offset < 0)
                    {
                        error = EINVAL;
                        return -1;
                    }
                    this->offset = offset;
                    break;
                case H_SEEK_CUR:
                    if (this->offset + offset < 0)
                    {
                        error = EINVAL;
                        return -1;
                    }
                    this->offset += offset;
                    break;
                case H_SEEK_END:
                    if ((int64_t)node->children.size() + offset < 0)
                    {
                        error = EINVAL;
                        return -1;
                    }
                    this->offset = node->children.size() + offset;
                    break;
                default:
                    error = EINVAL;
                    return -1;
                }

                return this->offset;
            }

            int64_t tell() override
            {
                return this->offset;
            }

            BaseFile *get(const char *name, int flags, int mode) override
            {
                auto *dir_node = get_node();
                if (!dir_node)
                    return nullptr;

                if (strchr(name, '/'))
                {
                    error = EINVAL;
                    return nullptr;
                }

                auto it = dir_node->children.find(name);
                if (it != dir_node->children.end())
                {
                    if (flags & OPEN_EXCL)
                    {
                        error = EEXIST;
                        return nullptr;
                    }

                    RamFsNode *node = it->second;

                    if (flags & OPEN_DIRECTORY && node->type() != FileType::Directory)
                    {
                        error = ENOTDIR;
                        return nullptr;
                    }

                    switch (node->type())
                    {
                    case FileType::Regular:
                        return alloc<RamFsRegularHandle>(1, static_cast<RamFsRegularNode *>(node), flags);
                    case FileType::Special:
                        return alloc<RamFsSpecialHandle>(1, static_cast<RamFsSpecialNode *>(node), flags);
                    case FileType::Symlink:
                        return alloc<RamFsSymlinkHandle>(1, static_cast<RamFsSymlinkNode *>(node), flags);
                    case FileType::Directory:
                        return alloc<RamFsDirectoryHandle>(1, static_cast<RamFsDirectoryNode *>(node), flags);
                    default:
                        error = EIO;
                        return nullptr;
                    }
                }
                else
                {
                    if (!(flags & OPEN_CREAT))
                    {
                        error = ENOENT;
                        return nullptr;
                    }

                    if (flags & OPEN_DIRECTORY)
                    {
                        return mkdir(name, flags, mode);
                    }
                    else
                    {
                        return mkfile(name, flags, mode);
                    }
                }
            }

            BaseRegularFile *mkfile(const char *name, int flags, int mode) override
            {
                auto *dir_node = get_node();
                if (!dir_node)
                    return nullptr;

                if (strchr(name, '/'))
                {
                    error = EINVAL;
                    return nullptr;
                }

                auto it = dir_node->children.find(name);
                if (it != dir_node->children.end())
                {
                    error = EEXIST;
                    return nullptr;
                }

                auto *new_node = alloc<RamFsRegularNode>(1, mode, 0, 0);
                new_node->filesystem = dir_node->filesystem;
                dir_node->children[name] = new_node;

                return alloc<RamFsRegularHandle>(1, new_node, flags);
            }

            BaseDirectory *mkdir(const char *name, int flags, int mode) override
            {
                auto *dir_node = get_node();
                if (!dir_node)
                    return nullptr;

                if (strchr(name, '/'))
                {
                    error = EINVAL;
                    return nullptr;
                }

                auto it = dir_node->children.find(name);
                if (it != dir_node->children.end())
                {
                    error = EEXIST;
                    return nullptr;
                }

                auto *new_node = alloc<RamFsDirectoryNode>(1, mode, 0, 0);
                dir_node->children[name] = new_node;
                new_node->filesystem = dir_node->filesystem;

                new_node->children["."] = new_node; // Self-reference
                new_node->children[".."] = dir_node; // Parent reference
                dir_node->refcount++; // Increment parent directory's refcount
                new_node->refcount++; // Increment new directory's refcount

                return alloc<RamFsDirectoryHandle>(1, new_node, flags);
            }

            BaseSymlink *mksym(const char *name, const char *target) override
            {
                auto *dir_node = get_node();
                if (!dir_node)
                    return nullptr;

                if (strchr(name, '/'))
                {
                    error = EINVAL;
                    return nullptr;
                }

                auto it = dir_node->children.find(name);
                if (it != dir_node->children.end())
                {
                    error = EEXIST;
                    return nullptr;
                }

                auto *new_node = alloc<RamFsSymlinkNode>(1, 0777, 0, 0);
                new_node->filesystem = dir_node->filesystem;
                new_node->target = target;
                dir_node->children[name] = new_node;

                return alloc<RamFsSymlinkHandle>(1, new_node, flags);
            }

            BaseSpecialFile *mksfile(const char *name, int flags, DeviceID devid, int mode) override
            {
                auto *dir_node = get_node();
                if (!dir_node)
                    return nullptr;

                if (strchr(name, '/'))
                {
                    error = EINVAL;
                    return nullptr;
                }

                auto it = dir_node->children.find(name);
                if (it != dir_node->children.end())
                {
                    error = EEXIST;
                    return nullptr;
                }

                auto *new_node = alloc<RamFsSpecialNode>(1, mode, 0, 0);
                new_node->filesystem = dir_node->filesystem;
                new_node->device_id = devid;
                dir_node->children[name] = new_node;

                return alloc<RamFsSpecialHandle>(1, new_node, flags);
            }

            int link(BaseFile *file, const char *name) override
            {
                if (!file)
                {
                    error = EINVAL;
                    return -1;
                }

                if (!name || strchr(name, '/') != nullptr)
                {
                    error = EINVAL;
                    return -1;
                }

                if (file->get_filesystem() != this->get_filesystem())
                {
                    error = EXDEV;
                    return -1;
                }

                RamFsNodeHandle *handle = nullptr;
                switch(file->type())
                {
                case FileType::Regular:
                    handle = (RamFsRegularHandle*)file;
                    break;
                case FileType::Directory:
                    error = EISDIR;
                    return -1;
                case FileType::Special:
                    handle = (RamFsSpecialHandle*)file;
                    break;
                case FileType::Symlink:
                    handle = (RamFsSymlinkHandle*)file;
                    break;
                default:
                    // shouldn't get here
                    error = EBADF;
                    return -1;
                }

                assert(handle);

                auto dir_node = get_node();
                if (!dir_node)
                    return -1;
                
                RamFsNode *target = handle->get_node();


                if (!target)
                {
                    error = EBADF;
                    return -1;
                }

                if (dir_node->children.find(name) != dir_node->children.end())
                {
                    error = EEXIST;
                    return -1;
                }

                dir_node->children[name] = target;
                target->refcount += 1;

                return 0;
            }

            int remove(const char *name) override
            {
                auto *dir_node = get_node();
                if (!dir_node)
                    return -1;

                if (strchr(name, '/'))
                {
                    error = EINVAL;
                    return -1;
                }

                auto it = dir_node->children.find(name);
                if (it == dir_node->children.end())
                {
                    error = ENOENT;
                    return -1;
                }

                RamFsNode *node = it->second;

                if (node->type() == FileType::Directory)
                {
                    if (((RamFsDirectoryNode *)node)->children.size() > 2)
                    {
                        error = ENOTEMPTY;
                        return -1;
                    }
                    ((RamFsDirectoryNode *)node)->remove_dir_hardlinks();
                }

                dir_node->children.erase(it);
                node->refcount -= 1;
                if (node->refcount == 0)
                {
                    dealloc(node);
                }
                return 0;
            }
        private:
            int64_t offset;    

            RamFsDirectoryNode *get_node()
            {
                if (!node)
                {
                    error = EBADF;
                    return nullptr;
                }
                if (node->type() != FileType::Directory)
                {
                    error = EINVAL;
                    return nullptr;
                }
                return static_cast<RamFsDirectoryNode *>(node);
            }
        };
    } // namespace

    class RamFsData
    {
    public:
        RamFsData()
            : root(alloc<RamFsDirectoryNode>(1, 0777, 0, 0))
        {
            root->children["."] = root; // Self-reference
            root->refcount += 1;
        }

        ~RamFsData()
        {
            dealloc(root);
        }

        RamFsDirectoryNode *root;
    };

    RamFs::RamFs()
        : data(alloc<RamFsData>(1))
    {
        data->root->filesystem = this;
    }

    RamFs::~RamFs()
    {
        dealloc(data);
    }

    RamFs::RamFs(RamFs &&other)
        : data(other.data)
    {
        other.data = nullptr;
    }

    RamFs &RamFs::operator=(RamFs &&other)
    {
        if (this != &other)
        {
            dealloc(data);
            data = other.data;
            other.data = nullptr;
        }
        return *this;
    }

    BaseDirectory *RamFs::open_root(int flags)
    {
        if (!data)
        {
            error = EINVAL;
            return nullptr;
        }

        return alloc<RamFsDirectoryHandle>(1, data->root, flags);
    }
} // namespace Hamster
