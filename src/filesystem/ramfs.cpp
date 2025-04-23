// Ram filesystem

#include <filesystem/ramfs.hpp>
#include <memory/tree.hpp>
#include <memory/stl_sequential.hpp>
#include <memory/stl_map.hpp>
#include <memory/stl_set.hpp>
#include <memory/allocator.hpp>
#include <errno/errno.h>
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

            FileType type() const override { return FileType::Directory; }
        };

        class RamFsFileNode : public RamFsTreeNode
        {
        public:
            FileType type() const override { return FileType::Regular; }

            String data;
        };

        class RamFsFifoNode : public RamFsTreeNode
        {
        public:
            FileType type() const override { return FileType::FIFO; }

            Deque<char> data;
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

                int fd = ++fd_counter;
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

        private:
            // pointers in this map are weak and do not own the object
            UnorderedMap<int, RamFsTreeNode *> fd_map;
            UnorderedMap<int, unsigned int> fd_ref_count;
            UnorderedSet<RamFsTreeNode *> fd_set;

            int fd_counter;
        };
    } // namespace

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
                buf->st_dev = (int)node;
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
                    buf->st_size = ((RamFsFileNode *)node)->data.size();
                    buf->st_blksize = 4096;
                    buf->st_blocks = (buf->st_size + 4095) / 4096;
                    break;
                case FileType::FIFO:
                    buf->st_size = ((RamFsFifoNode *)node)->data.size();
                    buf->st_blksize = 4096;
                    buf->st_blocks = (buf->st_size + 4095) / 4096;
                    break;
                case FileType::Directory:
                    buf->st_size = 0;
                    buf->st_blksize = 4096;
                    buf->st_blocks = 0;
                    break;
                default:
                    error = EIO;
                    return -1;
                };

                return 0;
            }

            int mode() const
            {
                RamFsTreeNode *node = fs_data.fd_manager.get_node(fd);
                if (node == nullptr)
                {
                    error = EBADF;
                    return -1;
                }

                return node->mode;
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

        protected:
            int fd;
            RamFsData &fs_data;

            int open_flags;
        };

        class RamFsFifoFile : public RamFsFile, public BaseRegularFile
        {
        public:
            // takes ownership of the fd
            RamFsFifoFile(int fd, RamFsData &fs_data, int flags)
                : RamFsFile(fd, fs_data, flags)
            {
            }
            
            ~RamFsFifoFile() override = default;

            int rename(const char *newname) override { return RamFsFile::rename(newname); }
            int remove() override { return RamFsFile::remove(); }
            int stat(struct ::stat *buf) override { return RamFsFile::stat(buf); }
            int mode() const override { return RamFsFile::mode(); }
            int chmod(int mode) override { return RamFsFile::chmod(mode); }
            int chown(int uid, int gid) override { return RamFsFile::chown(uid, gid); }
            const char *name() const override { return RamFsFile::name(); }

            ssize_t read(uint8_t *buf, size_t size) override
            {
                if ((open_flags & O_ACCMODE) == O_WRONLY)
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
                if (node->type() != FileType::FIFO)
                {
                    error = EINVAL;
                    return -1;
                }
                RamFsFifoNode *fifo_node = (RamFsFifoNode *)node;
                if (fifo_node->data.size() == 0)
                {
                    error = EAGAIN;
                    return -1;
                }
                size_t read_size = std::min(size, fifo_node->data.size());
                for (size_t i = 0; i < read_size; ++i)
                {
                    buf[i] = fifo_node->data.front();
                }
                fifo_node->data.erase(fifo_node->data.begin(), fifo_node->data.begin() + read_size);
                return read_size;
            }

            ssize_t write(const uint8_t *buf, size_t size) override
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
                if (node->type() != FileType::FIFO)
                {
                    error = EINVAL;
                    return -1;
                }
                RamFsFifoNode *fifo_node = (RamFsFifoNode *)node;
                fifo_node->data.insert(fifo_node->data.end(), buf, buf + size);
                return size;
            }
        };

        class RamFsRegularFile : public RamFsFile, public BaseRegularFile
        {
        public:
            // takes ownership of the fd
            RamFsRegularFile(int fd, RamFsData &fs_data, int flags)
                : RamFsFile(fd, fs_data, flags), pos(0)
            {
            }
            
            ~RamFsRegularFile() override = default;

            int rename(const char *newname) override { return RamFsFile::rename(newname); }
            int remove() override { return RamFsFile::remove(); }
            int stat(struct ::stat *buf) override { return RamFsFile::stat(buf); }
            int mode() const override { return RamFsFile::mode(); }
            int chmod(int mode) override { return RamFsFile::chmod(mode); }
            int chown(int uid, int gid) override { return RamFsFile::chown(uid, gid); }
            const char *name() const override { return RamFsFile::name(); }

            ssize_t read(uint8_t *buf, size_t size) override
            {
                if ((open_flags & O_ACCMODE) == O_WRONLY)
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
                if (node->type() != FileType::Regular)
                {
                    error = EINVAL;
                    return -1;
                }
                RamFsFileNode *file_node = (RamFsFileNode *)node;
                if (pos >= file_node->data.size())
                {
                    // EOF
                    return 0;
                }
                size_t read_size = std::min((int64_t)size, (int64_t)file_node->data.size() - pos);
                memcpy(buf, file_node->data.c_str() + pos, read_size);
                pos += read_size;
                return read_size;
            }

            ssize_t write(const uint8_t *buf, size_t size) override
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
                if (node->type() != FileType::Regular)
                {
                    error = EINVAL;
                    return -1;
                }
                if (open_flags & O_APPEND)
                {
                    pos = ((RamFsFileNode *)node)->data.size();
                }
                RamFsFileNode *file_node = (RamFsFileNode *)node;
                if (pos + size > file_node->data.size())
                {
                    file_node->data.resize(pos + size);
                }
                memcpy(file_node->data.data() + pos, buf, size);
                pos += size;
                return size;
            }

            int64_t seek(int64_t offset, int whence) override
            {
                RamFsTreeNode *node = fs_data.fd_manager.get_node(fd);
                if (node == nullptr)
                {
                    error = EBADF;
                    return -1;
                }
                if (node->type() != FileType::Regular)
                {
                    error = EINVAL;
                    return -1;
                }
                switch (whence)
                {
                case SEEK_SET:
                    pos = offset;
                    break;
                case SEEK_CUR:
                    pos += offset;
                    break;
                case SEEK_END:
                    pos = ((RamFsFileNode *)node)->data.size() + offset;
                    break;
                default:
                    error = EINVAL;
                    return -1;
                }
                return pos;
            }

            int64_t tell() const override
            {
                RamFsTreeNode *node = fs_data.fd_manager.get_node(fd);
                if (node == nullptr)
                {
                    error = EBADF;
                    return -1;
                }
                if (node->type() != FileType::Regular)
                {
                    error = EINVAL;
                    return -1;
                }
                return pos;
            }

            int truncate(int64_t size) override
            {
                RamFsTreeNode *node = fs_data.fd_manager.get_node(fd);
                if (node == nullptr)
                {
                    error = EBADF;
                    return -1;
                }
                if (node->type() != FileType::Regular)
                {
                    error = EINVAL;
                    return -1;
                }
                RamFsFileNode *file_node = (RamFsFileNode *)node;
                file_node->data.resize(size);
                if (pos > size)
                    pos = size;
                return 0;
            }

        protected:
            int64_t pos;
        };
    } // namespace
    

    struct RamFsData
    {
        RamFsData()
            : tree("/", 0777)
        {
        }

        RamFsFdManager fd_manager;
        Tree<RamFsTreeNode *> tree;
    };

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


    BaseFile *RamFs::open(const char *path, int flags, ...) 
    {
        va_list args;
        va_start(args, flags);
        BaseFile *file = open(path, flags, args);
        va_end(args);
        return file;
    }

    BaseFile *RamFs::open(const char *path, int flags, va_list args) { return nullptr; }
    int RamFs::rename(const char *old_path, const char *new_path) { return -1; }
    int RamFs::unlink(const char *path) { return -1; }
    int RamFs::link(const char *old_path, const char *new_path) { return -1; }
    int RamFs::stat(const char *path, struct ::stat *buf) { return -1; }
    BaseRegularFile *RamFs::mkfile(const char *name, int flags, int mode) { return nullptr; }
    BaseDirectory *RamFs::mkdir(const char *name, int flags, int mode) { return nullptr; }
    BaseFifoFile *RamFs::mkfifo(const char *name, int flags, int mode) { return nullptr; }
} // namespace Hamster
