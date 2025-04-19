// Ram Filesystem

#include <filesystem/ramfs.hpp>
#include <memory/allocator.hpp>
#include <memory/stl_sequential.hpp>
#include <cstring>
#include <algorithm>
#include <fcntl.h>

// Set to 1 to create all parents while creating a file
// e.g. mkfile "a/b/c/d.txt" will create a, b, c, and d if they don't exist
// but it will only create d if CREATE_ALL_PARENTS is 0
#define CREATE_ALL_PARENTS 1

namespace Hamster
{
    class RamFsDirNode;

    namespace
    {
        class RamFsNode
        {
        public:
            RamFsNode() : name(), parent(nullptr) {}
            virtual ~RamFsNode() = default;

            virtual const FileType type() const = 0;

            String name;
            RamFsDirNode *parent;
        };

        class RamFsFileNode : public RamFsNode
        {
        public:
            RamFsFileNode() = default;
            ~RamFsFileNode() override = default;

            const FileType type() const override { return FileType::Regular; }
            String data;
        };
    }

    class RamFsDirNode : public RamFsNode
    {
    public:
        RamFsDirNode() = default;
        ~RamFsDirNode() override
        {
            for (auto &child : children)
            {
                dealloc(child);
            }
            children.clear();
        }

        const FileType type() const override { return FileType::Directory; }

        RamFsNode *get(const char *name)
        {
            if (!name)
                return nullptr;
            for (auto &child : children)
            {
                if (child->name == name)
                    return child;
            }
            return nullptr;
        }

        RamFsNode *add(const char *name, RamFsNode *node)
        {
            if (!name || !node)
                return nullptr;
            if (get(name))
                return nullptr;

            // remove from parent if needed
            // don't deallocate because we are moving it
            if (node->parent)
                node->parent->children.remove(node);
            node->name = name;
            node->parent = this;
            children.push_back(node);
            return node;
        }

        int remove(const char *name)
        {
            if (!name)
                return -1;

            RamFsNode *node = get(name);
            if (!node)
                return -2;

            children.remove(node);
            dealloc(node);
            return 0;
        }

        char *const *list()
        {
            if (children.empty())
                return nullptr;

            char **names = alloc<char *>(children.size() + 1);

            size_t i = 0;
            for (const auto &child : children)
            {
                names[i] = alloc<char>(child->name.length() + 1);
                strcpy(names[i], child->name.c_str());
                ++i;
            }
            names[i] = nullptr;
            return names;
        }

        List<RamFsNode *> children;
    };

    namespace
    {
        class RamFsFifoNode : public RamFsNode
        {
        public:
            RamFsFifoNode() = default;
            ~RamFsFifoNode() override = default;
            const FileType type() const override { return FileType::FIFO; }

            int read(uint8_t *buf, size_t size)
            {
                size = std::min(size, fifo.size());

                for (size_t i = 0; i < size; ++i)
                {
                    buf[i] = fifo.front();
                    fifo.pop_front();
                }
                return size;
            }

            int write(const uint8_t *buf, size_t size)
            {
                if (fifo.size() + size > fifo.max_size())
                    return -1;

                fifo.insert(fifo.end(), buf, buf + size);
                return size;
            }
            Deque<uint8_t> fifo;
        };

        // file descriptors

        class RamFsRegularFd : public BaseRegularFile
        {
        public:
            RamFsRegularFd(RamFsFileNode *node, int flags) : node(node), pos(0), flags(flags) {}

            // do not deallocate node because we don't own it
            ~RamFsRegularFd() override = default;

            int rename(const char *name) override
            {
                if (!name || !name[0])
                    return -1;
                if (!node)
                    return -2;

                node->name = name;
                return 0;
            }

            int remove() override
            {
                if (!node)
                    return -1;
                if (!node->parent)
                    return -2;

                int ret = node->parent->remove(node->name.c_str());
                if (ret == 0)
                    node = nullptr;
                return ret;
            }

            int read(uint8_t *buf, size_t size) override
            {
                if (!node)
                    return -1;
                
                if (flags & (O_RDWR | O_RDONLY) == 0)
                    return -1;

                size_t data_len = node->data.length();
                if (pos >= data_len)
                    return 0;
                if (pos + size > data_len)
                    size = data_len - pos;

                if (size == 0)
                    return 0;

                memcpy(buf, node->data.c_str() + pos, size);
                pos += size;
                return size;
            }

            int write(const uint8_t *buf, size_t size) override
            {
                if (!node)
                    return -1;

                if (pos + size > node->data.max_size())
                    size = node->data.max_size() - pos;
                
                if (flags & (O_RDWR | O_WRONLY) == 0)
                    return -1;
                
                if (flags & O_APPEND)
                {
                    seek(0, SEEK_END);
                    node->data.append((const char *)buf, size);
                    return size;
                }

                // pad with trailing zeros if pos > data.length()
                if (pos > node->data.length())
                {
                    size_t pad = pos - node->data.length();
                    node->data.append(pad, '\0');
                }

                node->data.insert(pos, (const char *)buf, size);
                pos += size;
                return size;
            }

            int64_t seek(int64_t offset, int whence) override
            {
                if (!node)
                    return -1;

                auto len = node->data.length();

                switch (whence)
                {
                case SEEK_SET:
                    if (offset < 0)
                        return -1;
                    pos = offset;
                    break;
                case SEEK_CUR:
                    if (pos + offset < 0)
                        return -1;
                    pos += offset;
                    break;
                case SEEK_END:
                    if (len + offset < 0)
                        return -1;
                    pos = len + offset;
                    break;
                default:
                    return -1;
                }
                return pos;
            }

            RamFsFileNode *node;
            uint64_t pos;
            int flags;
        };

        class RamFsFifoFd : public BaseFifoFile
        {
        public:
            RamFsFifoFd(RamFsFifoNode *node) : node(node) {}
            ~RamFsFifoFd() override = default;

            int rename(const char *name) override
            {
                if (!name || !name[0])
                    return -1;
                if (!node)
                    return -2;

                node->name = name;
                return 0;
            }

            int remove() override
            {
                if (!node)
                    return -1;
                if (!node->parent)
                    return -2;

                int ret = node->parent->remove(node->name.c_str());
                if (ret == 0)
                    node = nullptr;
                return ret;
            }

            int read(uint8_t *buf, size_t size) override
            {
                if (!node)
                    return -1;
                return node->read(buf, size);
            }

            int write(const uint8_t *buf, size_t size) override
            {
                if (!node)
                    return -1;
                return node->write(buf, size);
            }

            RamFsFifoNode *node;
        };

        class RamFsDirFd : public BaseDirectory
        {
        public:
            RamFsDirFd(RamFsDirNode *node) : node(node) {}
            ~RamFsDirFd() override = default;

            int rename(const char *name) override
            {
                if (!name || !name[0])
                    return -1;
                if (!node)
                    return -2;

                node->name = name;
                return 0;
            }

            int remove() override
            {
                if (!node)
                    return -1;
                if (!node->parent)
                    return -2;

                int ret = node->parent->remove(node->name.c_str());
                if (ret == 0)
                    node = nullptr;
                return ret;
            }

            char *const *list() override
            {
                if (!node)
                    return nullptr;
                return node->list();
            }

            BaseFile *get(const char *name, int flags, ...) override
            {
                if (!node)
                    return nullptr;
                if (!name || !name[0])
                    return nullptr;

                RamFsNode *child = node->get(name);
                if (!child)
                    return nullptr;

                switch (child->type())
                {
                case FileType::Regular:
                    return alloc<RamFsRegularFd>(1, static_cast<RamFsFileNode *>(child), flags);
                case FileType::Directory:
                    return alloc<RamFsDirFd>(1, static_cast<RamFsDirNode *>(child));
                case FileType::FIFO:
                    return alloc<RamFsFifoFd>(1, static_cast<RamFsFifoNode *>(child));
                default:
                    return nullptr;
                }
            }

            BaseFile *mkfile(const char *name, int flags, int mode) override
            {
                if (!node)
                    return nullptr;
                if (!name || !name[0])
                    return nullptr;

                RamFsFileNode *file = alloc<RamFsFileNode>();
                if (!file)
                    return nullptr;

                node->add(name, file);
                return alloc<RamFsRegularFd>(1, file, flags);
            }

            BaseFile *mkdir(const char *name, int flags, int mode) override
            {
                if (!node)
                    return nullptr;
                if (!name || !name[0])
                    return nullptr;

                RamFsDirNode *dir = alloc<RamFsDirNode>();
                if (!dir)
                    return nullptr;

                node->add(name, dir);
                return alloc<RamFsDirFd>(1, dir);
            }

            BaseFile *mkfifo(const char *name, int flags, int mode) override
            {
                if (!node)
                    return nullptr;
                if (!name || !name[0])
                    return nullptr;

                RamFsFifoNode *fifo = alloc<RamFsFifoNode>();
                if (!fifo)
                    return nullptr;

                node->add(name, fifo);
                return alloc<RamFsFifoFd>(1, fifo);
            }

            int remove(const char *name) override
            {
                if (!node)
                    return -1;
                if (!name || !name[0])
                    return -2;

                return node->remove(name);
            }

            RamFsDirNode *node;
        };
    }

    RamFs::RamFs() : root(alloc<RamFsDirNode>())
    {
        root->name = "/";
        root->parent = nullptr;
    }

    RamFs::~RamFs()
    {
        dealloc(root);
    }

    BaseFile *RamFs::open(const char *path, int flags, ...)
    {
        int mode = 0;
        if (flags & O_CREAT)
        {
            va_list args;
            va_start(args, flags);
            mode = va_arg(args, int);
            va_end(args);
        }
        return open_impl(path, root, flags, mode);
    }

    BaseFile *RamFs::open(const char *path, int flags, va_list args)
    {
        int mode = 0;
        if (flags & O_CREAT)
        {
            mode = va_arg(args, int);
        }
        return open_impl(path, root, flags, mode);
    }

    BaseFile *RamFs::open_impl(const char *path, RamFsDirNode *parent, int flags, int mode)
    {
        if (!path || !path[0] || !parent)
            return nullptr;

        while (path[0] == '/')
            ++path;

        if (!path[0])
        {
            if (flags & O_EXCL && flags & O_CREAT)
                return nullptr;
            return alloc<RamFsDirFd>(1, parent);
        }

        const char *next = strchr(path, '/');

        if (strncmp(path, "..", 2) == 0)
        {
            parent = parent->parent;
            if (!parent)
                parent = root;
            return open_impl(next, parent, flags, mode);
        }
        if (strncmp(path, ".", 1) == 0)
        {
            return open_impl(next, parent, flags, mode);
        }

        if (next)
        {
            size_t len = next - path;
            String name(path, len);
            RamFsNode *child = parent->get(name.c_str());
            if (!child)
            {
#if CREATE_ALL_PARENTS
                // make directory
                if (!(flags & (O_CREAT | O_TRUNC | O_APPEND | O_WRONLY | O_RDWR)))
                    return nullptr;
                RamFsDirNode *dir = alloc<RamFsDirNode>();
                parent->add(name.c_str(), dir);
                return open_impl(next, dir, flags, mode);
#else
                return nullptr;
#endif
            }
            if (child->type() != FileType::Directory)
                return nullptr;
            return open_impl(next, static_cast<RamFsDirNode *>(child), flags, mode);
        }
        else
        {
            RamFsNode *child = parent->get(path);
            if (!child)
            {
                // make file or directory
                if (!(flags & (O_CREAT | O_TRUNC | O_APPEND | O_WRONLY | O_RDWR)))
                    return nullptr;
                RamFsNode *new_node = nullptr;
                if (flags & O_DIRECTORY)
                    new_node = alloc<RamFsDirNode>();
                else
                    new_node = alloc<RamFsFileNode>();
                parent->add(path, new_node);
                if (flags & O_DIRECTORY)
                    return alloc<RamFsDirFd>(1, static_cast<RamFsDirNode *>(new_node));
                else
                    return alloc<RamFsRegularFd>(1, static_cast<RamFsFileNode *>(new_node), flags);
            }
            if (flags & O_EXCL && flags & O_CREAT)
                return nullptr;
            if (flags & O_DIRECTORY && child->type() != FileType::Directory)
                return nullptr;
            if (child->type() == FileType::Regular)
            {
                RamFsFileNode *file = static_cast<RamFsFileNode *>(child);
                if (flags & O_TRUNC)
                    file->data.clear();
                RamFsRegularFd *fd = alloc<RamFsRegularFd>(1, file, flags);
                return fd;
            }
            else if (child->type() == FileType::Directory)
                return alloc<RamFsDirFd>(1, static_cast<RamFsDirNode *>(child));
            else if (child->type() == FileType::FIFO)
                return alloc<RamFsFifoFd>(1, static_cast<RamFsFifoNode *>(child));
        }

        return nullptr;
    }

    int RamFs::rename(const char *old_path, const char *new_path)
    {
        if (!old_path || !new_path)
            return -1;
        if (!old_path[0] || !new_path[0])
            return -2;
        if (strcmp(old_path, new_path) == 0)
            return 0;

        const char *last_slash = strrchr(old_path, '/');
        if (!last_slash)
            last_slash = old_path;

        String new_parent_path(old_path, last_slash - old_path);

        BaseFile *file = open(old_path, O_RDONLY);
        BaseFile *new_parent = open(new_parent_path.c_str(), O_RDONLY);

        struct DeallocAtEnd
        {
            BaseFile *f1;
            BaseFile *f2;
            DeallocAtEnd(BaseFile *f1, BaseFile *f2) : f1(f1), f2(f2) {}
            ~DeallocAtEnd()
            {
                dealloc(f1);
                dealloc(f2);
            }
        } dealloc_at_end(file, new_parent);

        if (!file || !new_parent || new_parent->type() != FileType::Directory)
            return -3;

        if (last_slash != old_path)
            ++last_slash;

        RamFsDirNode *parent = static_cast<RamFsDirFd *>(new_parent)->node;

        RamFsNode *child;
        switch (file->type())
        {
        case FileType::Regular:
            child = static_cast<RamFsRegularFd *>(file)->node;
            break;
        case FileType::Directory:
            child = static_cast<RamFsDirFd *>(file)->node;
            break;
        case FileType::FIFO:
            child = static_cast<RamFsFifoFd *>(file)->node;
            break;
        default:
            return -4;
        }

        if (!child)
            return -5;

        parent->add(last_slash, child);

        return 0;
    }

    int RamFs::unlink(const char *path)
    {
        BaseFile *file = open(path, O_RDONLY);

        if (file)
        {
            int ret = file->remove();
            dealloc(file);
            return ret;
        }
        return -1;
    }

    int RamFs::link(const char *old_path, const char *new_path)
    {
        // TODO
        return -1;
    }

    int RamFs::stat(const char *path, struct ::stat *buf)
    {
        if (!path || !buf)
            return -1;
        if (!path[0])
            return -2;
        BaseFile *file = open(path, O_RDONLY);
        if (!file)
            return -3;

        buf->st_size = 0;
        buf->st_blocks = 0;
        buf->st_blksize = 4096;

        switch (file->type())
        {
        case FileType::Regular:
            buf->st_mode = S_IFREG | 0777;
            buf->st_size = static_cast<RamFsRegularFd *>(file)->node->data.length();
            break;
        case FileType::Directory:
            buf->st_mode = S_IFDIR | 0777;
            break;
        case FileType::FIFO:
            buf->st_mode = S_IFIFO | 0777;
            break;
        default:
            dealloc(file);
            return -4;
        }

        buf->st_nlink = 1;
        buf->st_uid = 0;
        buf->st_gid = 0;
        buf->st_atime = 0;
        buf->st_mtime = 0;
        buf->st_ctime = 0;
        buf->st_dev = 0;
        buf->st_ino = 0;
        buf->st_rdev = 0;

        dealloc(file);
        return 0;
    }

    BaseRegularFile *RamFs::mkfile(const char *name, int flags, int mode)
    {
        return (BaseRegularFile *)open_impl(name, root, flags | O_CREAT | O_EXCL, mode);
    }
    BaseDirectory *RamFs::mkdir(const char *name, int flags, int mode)
    {
        return (BaseDirectory *)open_impl(name, root, flags | O_CREAT | O_EXCL | O_DIRECTORY, mode);
    }
    BaseFifoFile *RamFs::mkfifo(const char *name, int flags, int mode)
    {
        // TODO
        return nullptr;
    }
} // namespace Hamster
