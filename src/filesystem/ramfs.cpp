// Ram filesystem

#include <filesystem/ramfs.hpp>
#include <filesystem/mounts.hpp>
#include <memory/tree.hpp>
#include <memory/stl_sequential.hpp>
#include <memory/stl_map.hpp>
#include <memory/stl_set.hpp>
#include <memory/allocator.hpp>
#include <memory/memory_space.hpp>
#include <errno/errno.h>
#include <cstring>
#include <algorithm>
#include <fcntl.h>

namespace Hamster
{
    // Tree
    namespace
    {
        class RamFsTreeNode
        {
        public:
            RamFsTreeNode(const String &name, int mode)
                : name(name), mode(mode), uid(0), gid(0)
            {}

            RamFsTreeNode(const RamFsTreeNode &other) = delete;
            RamFsTreeNode &operator=(const RamFsTreeNode &other) = delete;

            virtual ~RamFsTreeNode() = default;

            virtual FileType type() const = 0;

            String name;
            int mode;

            int uid;
            int gid;
        };

        class RamFsDirNode : public RamFsTreeNode
        {
        public:
            // empty because Tree takes care of everyting
            RamFsDirNode(const String &name, int mode)
                : RamFsTreeNode(name, mode)
            {
            }

            ~RamFsDirNode() override = default;

            FileType type() const override { return FileType::Directory; }
        };

        class RamFsFileNode : public RamFsTreeNode
        {
        public:
            RamFsFileNode(const String &name, int mode)
                : RamFsTreeNode(name, mode), length(0)
            {
            }

            ~RamFsFileNode() override = default;

            FileType type() const override { return FileType::Regular; }

            MemorySpace data;
            int64_t length;
        };

        class RamFsSymlinkNode : public RamFsTreeNode
        {
        public:
            RamFsSymlinkNode(const String &name, int mode, const String &target)
                : RamFsTreeNode(name, mode), target(target)
            {
            }

            ~RamFsSymlinkNode() override = default;

            FileType type() const override { return FileType::SymbolicLink; }

            String target;
        };

        class RamFsSymbolicNode : public RamFsTreeNode
        {
        public:
            RamFsSymbolicNode(const String &name, int mode, FileType symbolic_type)
                : RamFsTreeNode(name, mode), symbolic_type(symbolic_type)
            {
            }

            ~RamFsSymbolicNode() override = default;

            FileType type() const override { return FileType::SymbolicFile; }

            FileType symbolic_type;
        };
    } // namespace

    // File descriptor manager
    namespace
    {
        class RamFsFdManager
        {
        public:
            RamFsFdManager() : fd_counter(0) 
            {
            }

            ~RamFsFdManager() = default;

            int add_fd(RamFsTreeNode *node)
            {
                if (node == nullptr)
                    return -1;
                if (fd_set.find(node) != fd_set.end())
                {
                    // already in use, get the fd
                    for (auto &[fd, n] : fd_map)
                    {
                        if (n == node)
                        {
                            ++fd_ref_count[fd];
                            return fd;
                        }
                    }
                }

                if (node->type() == FileType::Regular)
                {
                    ((RamFsFileNode *)node)->data.swap_in_pages();
                }

                int fd = ++fd_counter;
                while (fd_ref_count[fd] > 0)
                {
                    // if fd counter overflows, and it matches an existing fd
                    ++fd;
                }

                fd_map[fd] = node;
                fd_ref_count[fd] = 1;
                fd_set.insert(node);
                return fd;
            }

            void remove_fd(int fd)
            {
                auto ref_it = fd_ref_count.find(fd);
                if (ref_it == fd_ref_count.end())
                    return;
                if (--ref_it->second > 0)
                    return;
                auto it = fd_map.find(fd);
                if (it != fd_map.end())
                {
                    if (it->second->type() == FileType::Regular)
                    {
                        ((RamFsFileNode *)it->second)->data.swap_out_pages();
                    }
                    fd_set.erase(it->second);
                    fd_ref_count.erase(ref_it);
                    fd_map.erase(it);
                }
            }

            // please do not keep this pointer for long, as it
            // defeats the point of this manager
            RamFsTreeNode *get_node(int fd)
            {
                auto it = fd_map.find(fd);
                if (it != fd_map.end())
                    return it->second;
                return nullptr;
            }

            unsigned int get_ref_count(int fd)
            {
                auto it = fd_ref_count.find(fd);
                if (it != fd_ref_count.end())
                    return it->second;
                return 0;
            }

            bool is_busy()
            {
                return !fd_set.empty();
            }

        private:
            // pointers in this map are weak and do not own the object
            UnorderedMap<int, RamFsTreeNode *> fd_map;
            UnorderedMap<int, unsigned int> fd_ref_count;
            UnorderedSet<RamFsTreeNode *> fd_set;

            int fd_counter;
        };
    } // namespace

    struct RamFsData
    {
        RamFsData()
            : tree(alloc<RamFsDirNode>(1, "/", 0777))
        {
        }

        RamFsFdManager fd_manager;
        Tree<RamFsTreeNode *> tree;
    };

    // user file and directory and fifo
    namespace
    {
        class RamFsFile // don't inherit from BaseFile to prevent diamond problem
        {
        public:
            // takes ownership of the fd
            RamFsFile(int fd, RamFsData &fs_data, int flags)
                : fd(fd), fs_data(fs_data), open_flags(flags)
            {
            }

            ~RamFsFile()
            {
                fs_data.fd_manager.remove_fd(fd);
            }

            int rename(const char *newname)
            {
                if ((open_flags & O_ACCMODE) == O_RDONLY)
                {
                    error = EACCES;
                    return -1;
                }

                RamFsTreeNode *node = fs_data.fd_manager.get_node(fd);
                if (node == nullptr)
                {
                    error = EBADF;
                    return -1;
                }

                node->name = newname;
                return 0;
            }

            int remove()
            {
                if ((open_flags & O_ACCMODE) == O_RDONLY)
                {
                    error = EACCES;
                    return -1;
                }
                
                RamFsTreeNode *node = fs_data.fd_manager.get_node(fd);
                if (node == nullptr)
                {
                    error = EBADF;
                    return -1;
                }
                
                for (auto it = fs_data.tree.begin(); it != fs_data.tree.end(); ++it)
                {
                    if (*it == node)
                    {
                        it.remove();
                        fs_data.fd_manager.remove_fd(fd);
                        return 0;
                    }
                }
                
                // should not happen
                error = EIO;
                return -1;
            }

            int stat(struct ::stat *buf)
            {
                RamFsTreeNode *node = fs_data.fd_manager.get_node(fd);
                if (node == nullptr)
                {
                    error = EBADF;
                    return -1;
                }

                // see: https://pubs.opengroup.org/onlinepubs/007904875/basedefs/sys/stat.h.html
                buf->st_dev = (int)(uintptr_t)node;
                buf->st_ino = 0;
                buf->st_mode = node->mode;
                buf->st_nlink = fs_data.fd_manager.get_ref_count(fd);
                if (buf->st_nlink == 0) buf->st_nlink = 1;
                buf->st_uid = node->uid;
                buf->st_gid = node->gid;
                buf->st_rdev = 0;
                buf->st_atime = 0;
                buf->st_mtime = 0;
                buf->st_ctime = 0;

                switch (node->type())
                {
                case FileType::Regular:
                    buf->st_size = ((RamFsFileNode *)node)->length;
                    buf->st_blksize = 4096;
                    buf->st_blocks = (buf->st_size + 4095) / 4096;
                    buf->st_mode |= S_IFREG;
                    break;
                case FileType::Directory:
                    buf->st_size = 0;
                    buf->st_blksize = 4096;
                    buf->st_blocks = 0;
                    buf->st_mode |= S_IFDIR;
                    break;
                case FileType::FIFO:
                    buf->st_size = 0;
                    buf->st_blksize = 4096;
                    buf->st_blocks = 0;
                    buf->st_mode |= S_IFIFO;
                    break;
                case FileType::CharDevice:
                    buf->st_size = 0;
                    buf->st_blksize = 4096;
                    buf->st_blocks = 0;
                    buf->st_mode |= S_IFCHR;
                    break;
                case FileType::BlockDevice:
                    buf->st_size = 0;
                    buf->st_blksize = 4096;
                    buf->st_blocks = 0;
                    buf->st_mode |= S_IFBLK;
                    break;
                case FileType::SymbolicLink:
                    buf->st_size = ((RamFsSymlinkNode *)node)->target.length();
                    buf->st_blksize = 4096;
                    buf->st_blocks = 0;
                    buf->st_mode |= S_IFLNK;
                    break;
                default:
                    error = EIO;
                    return -1;
                };

                return 0;
            }

            int get_mode() const
            {
                RamFsTreeNode *node = fs_data.fd_manager.get_node(fd);
                if (node == nullptr)
                {
                    error = EBADF;
                    return -1;
                }

                int mode = node->mode;

                switch (node->type())
                {
                case FileType::Regular:
                    mode |= S_IFREG;
                    break;
                case FileType::Directory:
                    mode |= S_IFDIR;
                    break;
                case FileType::FIFO:
                    mode |= S_IFIFO;
                    break;
                case FileType::CharDevice:
                    mode |= S_IFCHR;
                    break;
                case FileType::BlockDevice:
                    mode |= S_IFBLK;
                    break;
                case FileType::SymbolicLink:
                    mode |= S_IFLNK;
                    break;
                default:
                    error = EIO;
                    return -1;
                }
                return mode;
            }

            int get_uid() const
            {
                RamFsTreeNode *node = fs_data.fd_manager.get_node(fd);
                if (node == nullptr)
                {
                    error = EBADF;
                    return -1;
                }

                return node->uid;
            }

            int get_gid() const
            {
                RamFsTreeNode *node = fs_data.fd_manager.get_node(fd);
                if (node == nullptr)
                {
                    error = EBADF;
                    return -1;
                }

                return node->gid;
            }

            int chmod(int mode)
            {
                RamFsTreeNode *node = fs_data.fd_manager.get_node(fd);
                if (node == nullptr)
                {
                    error = EBADF;
                    return -1;
                }

                node->mode = mode;
                return 0;
            }

            int chown(int uid, int gid)
            {
                RamFsTreeNode *node = fs_data.fd_manager.get_node(fd);
                if (node == nullptr)
                    return -1;
                node->uid = uid;
                node->gid = gid;
                return 0;
            }

            const char *name() const
            {
                RamFsTreeNode *node = fs_data.fd_manager.get_node(fd);
                if (node == nullptr)
                {
                    error = EBADF;
                    return nullptr;
                }

                return node->name.c_str();
            }

            int get_fd() const
            {
                return fd;
            }

        protected:
            int fd;
            RamFsData &fs_data;

            int open_flags;
        };

        class RamFsRegularFile : private RamFsFile, public BaseRegularFile
        {
        public:
            RamFsRegularFile(int fd, RamFsData &fs_data, int flags)
                : RamFsFile(fd, fs_data, flags), position(0)
            {
            }

            ~RamFsRegularFile() override = default;

            int rename(const char *newname) override { return RamFsFile::rename(newname); }
            int remove() override { return RamFsFile::remove(); }
            int stat(struct ::stat *buf) override { return RamFsFile::stat(buf); }
            int get_mode() override { return RamFsFile::get_mode(); }
            int get_uid() override { return RamFsFile::get_uid(); }
            int get_gid() override { return RamFsFile::get_gid(); }
            int chmod(int mode) override { return RamFsFile::chmod(mode); }
            int chown(int uid, int gid) override { return RamFsFile::chown(uid, gid); }
            const char *name() override { return RamFsFile::name(); }
            int get_fd() override { return RamFsFile::get_fd(); }

            ssize_t read(uint8_t *buf, size_t size) override
            {
                if ((open_flags & O_ACCMODE) == O_WRONLY)
                {
                    error = EACCES;
                    return -1;
                }
                RamFsFileNode *file_node = get_file_node();
                if (file_node == nullptr)
                    return -1;
                if (position >= file_node->length)
                {
                    // read past end
                    error = EINVAL;
                    return 0;
                }

                size_t read_size = std::min((int64_t)size, file_node->length - position);
                for (size_t i = 0; i < read_size; ++i)
                {
                    buf[i] = file_node->data[position + i];
                }
                position += read_size;
                return read_size;
            }

            ssize_t write(const uint8_t *buf, size_t size) override
            {
                if ((open_flags & O_ACCMODE) == O_RDONLY)
                {
                    error = EACCES;
                    return -1;
                }
                RamFsFileNode *file_node = get_file_node();
                if (file_node == nullptr)
                    return -1;
                for (size_t i = 0; i < size; ++i)
                {
                    uint8_t &byte = file_node->data[position + i];
                    if (&byte != &Page::get_dummy())
                    {
                        // ok
                        byte = buf[i];
                    }
                    else if (file_node->data.allocate_page(position + i) < 0)
                    {
                        // failed allocation
                        if (i == 0)
                        {
                            error = EIO;
                            return -1;
                        }
                        return i;
                    }
                    else
                    {
                        // allocated page
                        file_node->data[position + i] = buf[i];
                    }
                }
                file_node->length = std::max(file_node->length, position + (int64_t)size);
                position += size;
                return size;
            }

            int64_t seek(int64_t offset, int whence) override
            {
                RamFsFileNode *file_node = get_file_node();
                if (file_node == nullptr)
                    return -1;
                switch (whence)
                {
                case SEEK_SET:
                    if (offset < 0)
                    {
                        error = EINVAL;
                        return -1;
                    }
                    position = offset;
                    break;
                case SEEK_CUR:
                    if (position + offset < 0)
                    {
                        error = EINVAL;
                        return -1;
                    }
                    position += offset;
                    break;
                case SEEK_END:
                    if (file_node->length + offset < 0)
                    {
                        error = EINVAL;
                        return -1;
                    }
                    position = file_node->length + offset;
                    break;
                default:
                    error = EINVAL;
                    return -1;
                }
                return position;
            }

            int64_t tell() override
            {   
                if (!get_file_node())
                    return -1;
                return position;
            }

            int truncate(int64_t size) override
            {
                if ((open_flags & O_ACCMODE) == O_RDONLY)
                {
                    error = EACCES;
                    return -1;
                }
                RamFsFileNode *file_node = get_file_node();
                if (file_node == nullptr)
                    return -1;
                if (size < 0)
                {
                    error = EINVAL;
                    return -1;
                }

                // free whole pages past the end
                if (size < file_node->length)
                {
                    for (int64_t i = MemorySpace::get_page_start(size); i <= (int64_t)MemorySpace::get_page_start(file_node->length); i += HAMSTER_PAGE_SIZE)
                    {
                        file_node->data.deallocate_page(i);
                    }
                }

                file_node->length = size;
                return 0;
            }

            int64_t size() override
            {
                RamFsFileNode *node = get_file_node();
                if (!node)
                    return -1;
                return node->length;
            }

        private:
            int64_t position;

            RamFsFileNode *get_file_node() const
            {
                RamFsTreeNode *node = fs_data.fd_manager.get_node(fd);
                if (node == nullptr)
                {
                    error = EBADF;
                    return nullptr;
                }
                if (node->type() != FileType::Regular)
                {
                    error = EINVAL;
                    return nullptr;
                }
                return (RamFsFileNode *)node;
            }
        };

        class RamFsSymbolicFile : private RamFsFile, public BaseSymbolicFile
        {
        public:
            RamFsSymbolicFile(int fd, RamFsData &fs_data, int flags)
                : RamFsFile(fd, fs_data, flags)
            {
            }

            ~RamFsSymbolicFile() override = default;

            int rename(const char *newname) override { return RamFsFile::rename(newname); }
            int remove() override { return RamFsFile::remove(); }
            int stat(struct ::stat *buf) override { return RamFsFile::stat(buf); }
            int get_mode() override { return RamFsFile::get_mode(); }
            int get_uid() override { return RamFsFile::get_uid(); }
            int get_gid() override { return RamFsFile::get_gid(); }
            int chmod(int mode) override { return RamFsFile::chmod(mode); }
            int chown(int uid, int gid) override { return RamFsFile::chown(uid, gid); }
            const char *name() override { return RamFsFile::name(); }
            int get_fd() override { return RamFsFile::get_fd(); }

            const FileType type() const override
            {
                RamFsTreeNode *node = fs_data.fd_manager.get_node(fd);
                if (node == nullptr)
                {
                    error = EBADF;
                    return FileType::SymbolicLink;
                }
                if (node->type() == FileType::SymbolicFile)
                {
                    return ((RamFsSymbolicNode *)node)->symbolic_type;
                }
                return FileType::Invalid;
            }
        };

        class RamFsDirectory : private RamFsFile, public BaseDirectory
        {
        public:
            RamFsDirectory(int fd, RamFsData &data, int flags)
                : RamFsFile(fd, data, flags)
            {
            }

            ~RamFsDirectory() override = default;

            int rename(const char *newname) override { return RamFsFile::rename(newname); }
            int remove() override { return RamFsFile::remove(); }
            int stat(struct ::stat *buf) override { return RamFsFile::stat(buf); }
            int get_mode() override { return RamFsFile::get_mode(); }
            int chmod(int mode) override { return RamFsFile::chmod(mode); }
            int chown(int uid, int gid) override { return RamFsFile::chown(uid, gid); }
            const char *name() override { return RamFsFile::name(); }
            int get_fd() override { return RamFsFile::get_fd(); }

            char * const * list() override
            {
                if ((open_flags & O_ACCMODE) == O_WRONLY)
                {
                    error = EACCES;
                    return nullptr;
                }

                RamFsDirNode *node = get_node();
                Tree<RamFsTreeNode *>::Iterator it = fs_data.tree.begin();
                for (; it != fs_data.tree.end(); ++it)
                    if (*it == node)
                        break;
                if (!it)
                {
                    error = EBADF;
                    return nullptr;
                }

                char **strings = alloc<char*>(it.children().size() + 1);

                for (size_t i = 0; i < it.children().size(); ++i)
                {
                    strings[i] = alloc<char>(it.children()[i].data->name.length() + 1);
                    strcpy(strings[i], it.children()[i].data->name.c_str());
                }

                strings[it.children().size()] = nullptr;

                return strings;
            }

            BaseFile *mkfile(const char *name, int flags, int mode) override
            {
                if ((open_flags & O_ACCMODE) == O_RDONLY)
                {
                    error = EACCES;
                    return nullptr;
                }
                if (!name)
                {
                    error = EINVAL;
                    return nullptr;
                }
                if (strchr(name, '/') != nullptr)
                {
                    error = EINVAL;
                    return nullptr;
                }
                RamFsDirNode *node = get_node();
                if (node == nullptr)
                    return nullptr;

                // get an iterator to the current node
                Tree<RamFsTreeNode *>::Iterator it = fs_data.tree.begin();
                for (; it != fs_data.tree.end(); ++it)
                    if (*it == node)
                        break;
                if (!it)
                {
                    // shouldn't happen
                    error = EBADF;
                    return nullptr;
                }
                // check if the file already exists
                for (auto &child : it.children())
                {
                    if (child.data->name == name)
                    {
                        error = EEXIST;
                        return nullptr;
                    }
                }
                // create the file
                RamFsFileNode *file_node = alloc<RamFsFileNode>(1, name, mode);
                file_node->uid = node->uid;
                file_node->gid = node->gid;

                it.insert(file_node);

                int fd = fs_data.fd_manager.add_fd(file_node);
                if (fd < 0)
                {
                    error = EIO;
                    return nullptr;
                }

                return alloc<RamFsRegularFile>(1, fd, fs_data, flags);
            }

            BaseFile *mkdir(const char *name, int flags, int mode) override
            {
                if ((open_flags & O_ACCMODE) == O_RDONLY)
                {
                    error = EACCES;
                    return nullptr;
                }
                if (!name)
                {
                    error = EINVAL;
                    return nullptr;
                }
                if (strchr(name, '/') != nullptr)
                {
                    error = EINVAL;
                    return nullptr;
                }
                RamFsDirNode *node = get_node();
                if (node == nullptr)
                    return nullptr;

                // get an iterator to the current node
                Tree<RamFsTreeNode *>::Iterator it = fs_data.tree.begin();
                for (; it != fs_data.tree.end(); ++it)
                    if (*it == node)
                        break;
                if (!it)
                {
                    // shouldn't happen
                    error = EBADF;
                    return nullptr;
                }
                
                // check if the directory already exists
                for (auto &child : it.children())
                {
                    if (child.data->name == name)
                    {
                        error = EEXIST;
                        return nullptr;
                    }
                }

                // create the directory
                RamFsDirNode *dir_node = alloc<RamFsDirNode>(1, name, mode);
                dir_node->uid = node->uid;
                dir_node->gid = node->gid;

                it.insert(dir_node);

                int fd = fs_data.fd_manager.add_fd(dir_node);
                if (fd < 0)
                {
                    error = EIO;
                    return nullptr;
                }

                return alloc<RamFsDirectory>(1, fd, fs_data, flags);
            }

            BaseFile *mksfile(const char *name, int flags, FileType type, int mode) override
            {
                if ((open_flags & O_ACCMODE) == O_RDONLY)
                {
                    error = EACCES;
                    return nullptr;
                }
                if (!name)
                {
                    error = EINVAL;
                    return nullptr;
                }
                if (strchr(name, '/') != nullptr)
                {
                    error = EINVAL;
                    return nullptr;
                }
                RamFsDirNode *node = get_node();
                if (node == nullptr)
                    return nullptr;

                // get an iterator to the current node
                Tree<RamFsTreeNode *>::Iterator it = fs_data.tree.begin();
                for (; it != fs_data.tree.end(); ++it)
                    if (*it == node)
                        break;
                if (!it)
                {
                    // shouldn't happen
                    error = EBADF;
                    return nullptr;
                }

                // check if the file already exists
                for (auto &child : it.children())
                {
                    if (child.data->name == name)
                    {
                        error = EEXIST;
                        return nullptr;
                    }
                }

                // create the file
                RamFsSymbolicNode *sfile_node = alloc<RamFsSymbolicNode>(1, name, mode, type);
                sfile_node->uid = node->uid;
                sfile_node->gid = node->gid;

                it.insert(sfile_node);

                int fd = fs_data.fd_manager.add_fd(sfile_node);
                if (fd < 0)
                {
                    error = EIO;
                    return nullptr;
                }

                return alloc<RamFsSymbolicFile>(1, fd, fs_data, flags);
            }

            BaseFile *open(const char *name, int flags, va_list args) override
            {
                if ((open_flags & O_ACCMODE) == O_WRONLY)
                {
                    error = EACCES;
                    return nullptr;
                }
                if (!name)
                {
                    error = EINVAL;
                    return nullptr;
                }
                
                while (name[0] == '/')
                ++name;

                const char *next = strchr(name, '/');
                
                if (name[0] == '\0' || strcmp(name, ".") == 0 || strncmp(name, "./", 2) == 0 ||
                    (next && (strcmp(next + 1, "..") == 0 || strncmp(next + 1, "../", 3) == 0)))
                {
                    // trailing slash, empty name, current directory, or parent directory of next element
                    // return current directory
                    //
                    // FIXME: for parent directory of next element, we should check for validity 
                    //        of the next element, right now we skip and just return this directory
                    int fd = fs_data.fd_manager.add_fd(get_node());
                    if (fd < 0)
                    {
                        error = EIO;
                        return nullptr;
                    }
                    return alloc<RamFsDirectory>(1, fd, fs_data, flags);
                }
                else if (strcmp(name, "..") == 0 || strncmp(name, "../", 3) == 0)
                {
                    // Parent directory
                    // return parent directory
                    RamFsTreeNode *node = fs_data.fd_manager.get_node(fd);
                    if (node == nullptr)
                    {
                        error = EBADF;
                        return nullptr;
                    }
                    if (node->type() != FileType::Directory)
                    {
                        error = ENOTDIR;
                        return nullptr;
                    }
                    Tree<RamFsTreeNode *>::Iterator it = fs_data.tree.begin();
                    for (; it != fs_data.tree.end(); ++it)
                        if (*it == node)
                            break;
                    if (!it)
                    {
                        error = EBADF;
                        return nullptr;
                    }
                    if (it.node()->parent == nullptr)
                    {
                        error = EINVAL;
                        return nullptr;
                    }
                    RamFsTreeNode *parent = it.node()->parent->data;
                    if (parent->type() != FileType::Directory)
                    {
                        error = ENOTDIR;
                        return nullptr;
                    }
                    int fd = fs_data.fd_manager.add_fd(parent);
                    if (fd < 0)
                    {
                        error = EIO;
                        return nullptr;
                    }
                    return alloc<RamFsDirectory>(1, fd, fs_data, flags);
                }
                

                if (next == nullptr)
                {
                    return get_by_name(name, flags, args);
                }
                else
                {
                    String next_name(name, next - name);
                    BaseDirectory *dir = (BaseDirectory*)get_by_name(next_name.c_str(), (flags & ~O_CREAT & ~O_EXCL) | O_DIRECTORY, args);
                    if (dir == nullptr)
                    {
                        error = ENOTDIR;
                        return nullptr;
                    }
                    BaseFile *file = dir->open(next + 1, flags, args);
                    dealloc(dir);
                    return file;
                }
            }

            int remove(const char *name) override
            {
                if ((open_flags & O_ACCMODE) == O_RDONLY)
                {
                    error = EACCES;
                    return -1;
                }
                if (!name)
                {
                    error = EINVAL;
                    return -1;
                }
                if (strchr(name, '/') != nullptr)
                {
                    error = EINVAL;
                    return -1;
                }
                RamFsDirNode *node = get_node();
                if (node == nullptr)
                    return -1;

                // get an iterator to the current node
                Tree<RamFsTreeNode *>::Iterator it = fs_data.tree.begin();
                for (; it != fs_data.tree.end(); ++it)
                    if (*it == node)
                        break;
                if (!it)
                {
                    // shouldn't happen
                    error = EBADF;
                    return -1;
                }

                for (size_t i = 0; i < it.children().size(); ++i)
                {
                    if (it.children()[i].data->name == name)
                    {
                        // found the file
                        if (it.children()[i].data->type() == FileType::Directory)
                        {
                            // check if empty
                            if (it.children()[i].children.size() > 0)
                            {
                                error = ENOTEMPTY;
                                return -1;
                            }
                        }
                        it.remove(i);
                        fs_data.fd_manager.remove_fd(fd);
                        return 0;
                    }
                }

                // not found
                error = ENOENT;
                return -1;
            }

            int get_uid() override
            {
                RamFsDirNode *node = get_node();
                if (node == nullptr)
                {
                    error = EBADF;
                    return -1;
                }
                return node->uid;
            }

            int get_gid() override
            {
                RamFsDirNode *node = get_node();
                if (node == nullptr)
                {
                    error = EBADF;
                    return -1;
                }
                return node->gid;
            }

        private:
            RamFsDirNode *get_node() const
            {
                RamFsTreeNode *node = fs_data.fd_manager.get_node(fd);
                if (!node)
                {
                    error = EBADF;
                    return nullptr;
                }
                return (RamFsDirNode *)node;
            }

            // gets a child by name
            BaseFile *get_by_name(const char *name, int flags, va_list args)
            {
                if (!name)
                {
                    error = EINVAL;
                    return nullptr;
                }
                if (strchr(name, '/') != nullptr)
                {
                    error = EINVAL;
                    return nullptr;
                }
                RamFsDirNode *node = get_node();
                if (node == nullptr)
                    return nullptr;

                if ((node->mode & O_ACCMODE) == O_WRONLY)
                {
                    error = EACCES;
                    return nullptr;
                }

                Tree<RamFsTreeNode *>::Iterator it = fs_data.tree.begin();
                for (; it != fs_data.tree.end(); ++it)
                    if (*it == node)
                        break;
                if (!it)
                {
                    error = EBADF;
                    return nullptr;
                }

                RamFsTreeNode *child = nullptr;

                for (auto &node : it.children())
                {
                    RamFsTreeNode *cur = node.data;
                    if (cur->name == name)
                    {
                        child = cur;
                        break;
                    }
                }

                if (strncmp(name, "..", 2) == 0)
                {
                    // parent directory
                    if (it.node()->parent == nullptr)
                    {
                        error = EINVAL;
                        return nullptr;
                    }
                    child = it.node()->parent->data;
                }
                else if (strncmp(name, ".", 1) == 0)
                {
                    // current directory
                    child = it.node()->data;
                }

                if (child)
                {
                    if (flags & O_EXCL)
                    {
                        error = EEXIST;
                        return nullptr;
                    }
                    if (!(child->mode & 0b100000000) && (flags & O_ACCMODE) != O_WRONLY)
                    {
                        // Read not permitted
                        error = EACCES;
                        return nullptr;
                    }
                    if (!(child->mode & 0b010000000) && (flags & O_ACCMODE) != O_RDONLY)
                    {
                        // Write not permitted
                        error = EACCES;
                        return nullptr;
                    }
                    // match
                    int fd = fs_data.fd_manager.add_fd(child);
                    if (fd < 0)
                    {
                        error = EIO;
                        return nullptr;
                    }

                    const char *target;
                    switch (child->type())
                    {
                    case FileType::Regular:
                        if (flags & O_DIRECTORY)
                        {
                            error = ENOTDIR;
                            return nullptr;
                        }
                        if (flags & O_TRUNC)
                            ((RamFsFileNode*)child)->length = 0; 
                        return alloc<RamFsRegularFile>(1, fd, fs_data, flags);
                    case FileType::SymbolicLink:
                        if (flags & O_NOFOLLOW)
                        {
                            error = ELOOP;
                            return nullptr;
                        }
                        target = ((RamFsSymlinkNode *)child)->target.c_str();
                        fs_data.fd_manager.remove_fd(fd);
                        return mounts.open(target, flags, args);
                    case FileType::Directory:
                        return alloc<RamFsDirectory>(1, fd, fs_data, flags);
                    case FileType::SymbolicFile:
                        return alloc<RamFsSymbolicFile>(1, fd, fs_data, flags);
                    default:
                        error = EPERM;
                        return nullptr;
                    }
                }
                else
                {
                    // not found
                    if (!(flags & O_CREAT))
                    {
                        error = ENOENT;
                        return nullptr;
                    }

                    // create
                    int mode = va_arg(args, int);

                    if (flags & O_DIRECTORY)
                    {
                        return mkdir(name, flags, mode);
                    }
                    else
                    {
                        return mkfile(name, flags, mode);
                    }
                }
            }
        };
    } // namespace
    

    

    // placeholders
    RamFs::RamFs()
    {
        data = alloc<RamFsData>();
    }

    RamFs::~RamFs() 
    {
        dealloc(data);
        data = nullptr;
    }

    RamFs::RamFs(RamFs &&other) 
    {
        if (this != &other)
        {
            std::swap(data, other.data);
        }
    }

    RamFs &RamFs::operator=(RamFs &&other) 
    {
        if (this != &other)
        {
            std::swap(data, other.data);
        }
        return *this;
    }

    BaseFile *RamFs::open(const char *path, int flags, va_list args) 
    {
        if (!path || !*path)
        {
            error = EINVAL;
            return nullptr;
        }

        int root_fd = data->fd_manager.add_fd(*data->tree.root());
        if (root_fd < 0)
        {
            error = EIO;
            return nullptr;
        }
        BaseDirectory *root = alloc<RamFsDirectory>(1, root_fd, *data, flags);
        BaseFile *file = root->open(path, flags, args);
        dealloc(root);

        // If file is nullptr, open will set the error
        return file;
    }

    int RamFs::rename(const char *old_path, const char *new_path)
    {
        if (!old_path || !new_path || !*old_path || !*new_path)
        {
            error = EINVAL;
            return -1;
        }

        BaseFile *file = open(old_path, O_RDONLY);
        if (!file)
            return -1;

        const char *new_path_last = strrchr(new_path, '/');
        size_t new_dir_len = 0;
        if (new_path_last)
            new_dir_len = new_path_last - new_path;

        String new_dir_path{new_path, new_dir_len};

        BaseFile *dir = open(new_dir_path.c_str(), O_RDONLY);
        if (!dir)
        {
            dealloc(file);
            return -1;
        }

        int file_fd, dir_fd;
        file_fd = file->get_fd();
        dir_fd = dir->get_fd();
        RamFsTreeNode *file_node = data->fd_manager.get_node(file_fd);
        RamFsTreeNode *dir_node = data->fd_manager.get_node(dir_fd);

        if (!file_node || !dir_node)
        {
            error = EBADF;
            dealloc(file);
            dealloc(dir);
            return -1;
        }

        auto file_it = data->tree.begin();
        auto dir_it = data->tree.begin();

        for (; file_it != data->tree.end(); ++file_it)
            if (*file_it == file_node)
                break;
        for (; dir_it != data->tree.end(); ++dir_it)
            if (*dir_it == dir_node)
                break;
        if (!file_it || !dir_it)
        {
            dealloc(file);
            dealloc(dir);
            return -1;
        }

        for (auto &child : dir_it.children())
            if (child.data->name == new_path_last)
            {
                error = EEXIST;
                dealloc(file);
                dealloc(dir);
                return -1;
            }
        file_it.node()->parent->children.erase(file_it.node()->parent->children.begin() + (file_it.node() - file_it.node()->parent->children.data()));
        dir_it.insert(file_node);
        file_node->name = new_path_last;

        dealloc(file);
        dealloc(dir);
        return 0;
    }

    int RamFs::unlink(const char *path) 
    {
        // don't use open and then remove, because it will follow symlinks

        if (!path || !*path)
        {
            error = EINVAL;
            return -1;
        }
        const char *last_slash = strrchr(path, '/');
        String parent_path(path, last_slash - path);

        BaseDirectory *parent = (BaseDirectory*)open(parent_path.c_str(), O_RDWR | O_DIRECTORY);
        if (!parent)
            return -1;
        int parent_fd = parent->get_fd();
        RamFsTreeNode *parent_node = data->fd_manager.get_node(parent_fd);
        if (!parent_node)
        {
            dealloc(parent);
            return -1;
        }
        auto it = data->tree.begin();
        for (; it != data->tree.end(); ++it)
            if (*it == parent_node)
                break;
        if (!it)
        {
            // parent does not exist, shouldn't happen
            dealloc(parent);
            return -1;
        }

        for (size_t i = 0; i < it.children().size(); ++i)
        {
            if (it.children()[i].data->name == last_slash + 1)
            {
                // found the file
                if (it.children()[i].data->type() == FileType::Directory)
                {
                    // check if empty
                    if (it.children()[i].children.size() > 0)
                    {
                        error = ENOTEMPTY;
                        dealloc(parent);
                        return -1;
                    }
                }
                it.remove(i);
                dealloc(parent);
                return 0;
            }
        }

        // not found
        error = ENOENT;
        dealloc(parent);
        return -1;
    }

    int RamFs::link(const char *old_path, const char *new_path) 
    {
        // Hard links are not supported
        error = ENOTSUP;
        return -1;
    }

    int RamFs::stat(const char *path, struct ::stat *buf) 
    {
        BaseFile *file = open(path, O_RDONLY);
        if (!file)
            return -1;
        int ret = file->stat(buf);
        dealloc(file);
        return ret;
    }

    int RamFs::lstat(const char *path, struct ::stat *buf) 
    {
        BaseFile *file = open(path, O_RDONLY | O_NOFOLLOW);
        if (file)
        {
            // file is not a symlink
            dealloc(file);
            error = EINVAL;
            return -1;
        }
        // fill with placeholder data
        memset(buf, 0, sizeof(struct ::stat));
        buf->st_mode = S_IFLNK | 0777;
        buf->st_nlink = 1;
        buf->st_size = 1;
        buf->st_blksize = 4096;
        buf->st_blocks = 1;
        buf->st_atime = 0;
        buf->st_mtime = 0;
        buf->st_ctime = 0;
        buf->st_uid = 0;
        buf->st_gid = 0;
        buf->st_dev = 0;
        buf->st_ino = 0;
        return 0;
    }

    BaseRegularFile *RamFs::mkfile(const char *name, int flags, int mode) 
    {
        BaseFile *f = open(name, (flags & ~O_DIRECTORY)|O_CREAT|O_TRUNC, mode);
        if (!f)
            return nullptr;
        assert(f->type() == FileType::Regular);
        return (BaseRegularFile *)f;
    }

    BaseDirectory *RamFs::mkdir(const char *name, int flags, int mode) 
    {
        BaseFile *f = open(name, flags|O_CREAT|O_DIRECTORY, mode);
        if (!f)
            return nullptr;
        assert(f->type() == FileType::Directory);
        return (BaseDirectory *)f;
    }

    BaseSymbolicFile *RamFs::mksfile(const char *name, int flags, FileType type, int mode) 
    {
        if (!name || !*name)
        {
            error = EINVAL;
            return nullptr;
        }

        const char *last_slash = strrchr(name, '/');
        String parent_path(name, last_slash - name);

        BaseDirectory *parent = (BaseDirectory*)open(parent_path.c_str(), O_RDWR | O_DIRECTORY);
        if (!parent)
            return nullptr;
        BaseFile *f = parent->mksfile(last_slash + 1, flags|O_CREAT, type, mode);
        if (!f)
        {
            dealloc(parent);
            return nullptr;
        }
        assert(f->type() == FileType::SymbolicFile);
        dealloc(parent);
        return (BaseSymbolicFile *)f;
    }

    int RamFs::symlink(const char *old_path, const char *new_path) 
    {
        if (!old_path || !new_path || !*old_path || !*new_path)
        {
            error = EINVAL;
            return -1;
        }

        const char *last_slash = strrchr(old_path, '/');
        String parent_path(old_path, last_slash - old_path);

        BaseDirectory *parent = (BaseDirectory*)open(parent_path.c_str(), O_RDWR | O_DIRECTORY);

        if (!parent)
            return -1;
        
        int fd = parent->get_fd();

        RamFsTreeNode *parent_node = data->fd_manager.get_node(fd);

        if (!parent_node)
        {
            dealloc(parent);
            return -1;
        }

        auto it = data->tree.begin();
        for (; it != data->tree.end(); ++it)
            if (*it == parent_node)
                break;
        if (!it)
        {
            // parent does not exist, shouldn't happen
            dealloc(parent);
            return -1;
        }

        for (size_t i = 0; i < it.children().size(); ++i)
        {
            if (it.children()[i].data->name == last_slash + 1)
            {
                // found the file
                error = EEXIST;
                dealloc(parent);
                return -1;
            }
        }

        // create the symlink
        RamFsSymlinkNode *sfile_node = alloc<RamFsSymlinkNode>(1, last_slash + 1, 0777, new_path);
        sfile_node->uid = parent_node->uid;
        sfile_node->gid = parent_node->gid;
        it.insert(sfile_node);

        dealloc(parent);
        return 0;
    }

    int RamFs::readlink(const char *path, char *buf, size_t buf_size) 
    {
        if (!path || !*path)
        {
            error = EINVAL;
            return -1;
        }

        const char *last_slash = strrchr(path, '/');
        String parent_path(path, last_slash - path);

        BaseDirectory *parent = (BaseDirectory*)open(parent_path.c_str(), O_RDWR | O_DIRECTORY);
        if (!parent)
            return -1;
        
        int fd = parent->get_fd();
        RamFsTreeNode *parent_node = data->fd_manager.get_node(fd);
        if (!parent_node)
        {
            dealloc(parent);
            return -1;
        }

        auto it = data->tree.begin();
        for (; it != data->tree.end(); ++it)
            if (*it == parent_node)
                break;
        if (!it)
        {
            // parent does not exist, shouldn't happen
            dealloc(parent);
            return -1;
        }
        for (size_t i = 0; i < it.children().size(); ++i)
        {
            if (it.children()[i].data->name == last_slash + 1)
            {
                // found the file
                if (it.children()[i].data->type() != FileType::SymbolicLink)
                {
                    error = EINVAL;
                    dealloc(parent);
                    return -1;
                }
                RamFsSymlinkNode *sfile_node = (RamFsSymlinkNode *)it.children()[i].data;
                strncpy(buf, sfile_node->target.c_str(), buf_size);
                dealloc(parent);
                return 0;
            }
        }

        // not found
        error = ENOENT;
        dealloc(parent);
        return -1;
    }

    char *RamFs::readlink(const char *path) 
    {
        if (!path || !*path)
        {
            error = EINVAL;
            return nullptr;
        }

        const char *last_slash = strrchr(path, '/');
        String parent_path(path, last_slash - path);

        BaseDirectory *parent = (BaseDirectory*)open(parent_path.c_str(), O_RDWR | O_DIRECTORY);
        if (!parent)
            return nullptr;

        int fd = parent->get_fd();
        RamFsTreeNode *parent_node = data->fd_manager.get_node(fd);
        if (!parent_node)
        {
            dealloc(parent);
            return nullptr;
        }

        auto it = data->tree.begin();
        for (; it != data->tree.end(); ++it)
            if (*it == parent_node)
                break;
        if (!it)
        {
            // parent does not exist, shouldn't happen
            dealloc(parent);
            return nullptr;
        }
        for (size_t i = 0; i < it.children().size(); ++i)
        {
            if (it.children()[i].data->name == last_slash + 1)
            {
                // found the file
                if (it.children()[i].data->type() != FileType::SymbolicLink)
                {
                    error = EINVAL;
                    dealloc(parent);
                    return nullptr;
                }
                RamFsSymlinkNode *sfile_node = (RamFsSymlinkNode *)it.children()[i].data;
                char *buf = alloc<char>(sfile_node->target.length() + 1);
                strcpy(buf, sfile_node->target.c_str());
                dealloc(parent);
                return buf;
            }
        }

        // not found
        error = ENOENT;
        dealloc(parent);
        return nullptr;
    }

    FileType RamFs::type(const char *path) 
    {
        if (!path || !*path)
        {
            error = EINVAL;
            return FileType::Invalid;
        }

        struct ::stat buf;
        if (lstat(path, &buf) < 0)
            return FileType::Invalid;
        if (S_ISREG(buf.st_mode))
            return FileType::Regular;
        if (S_ISDIR(buf.st_mode))
            return FileType::Directory;
        if (S_ISLNK(buf.st_mode))
            return FileType::SymbolicLink;
        if (S_ISFIFO(buf.st_mode))
            return FileType::FIFO;
        if (S_ISCHR(buf.st_mode))
            return FileType::CharDevice;
        if (S_ISBLK(buf.st_mode))
            return FileType::BlockDevice;
        return FileType::Invalid;
    }

    bool RamFs::is_busy()
    {
        return data->fd_manager.is_busy();
    }
} // namespace Hamster
