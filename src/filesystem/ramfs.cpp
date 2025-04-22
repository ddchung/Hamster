// Ram filesystem

#include <filesystem/ramfs.hpp>
#include <memory/tree.hpp>
#include <memory/stl_sequential.hpp>
#include <memory/stl_map.hpp>
#include <memory/stl_set.hpp>
#include <memory/allocator.hpp>

namespace Hamster
{
    // Tree
    namespace
    {
        class RamFsTreeNode
        {
        public:
            RamFsTreeNode(const String &name, int flags, int mode)
                : name(name), flags(flags), mode(mode) 
            {}

            RamFsTreeNode(const RamFsTreeNode &other) = delete;
            RamFsTreeNode &operator=(const RamFsTreeNode &other) = delete;

            virtual ~RamFsTreeNode() = default;

            virtual FileType type() const = 0;

            String name;
            int flags;
            int mode;
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

            Deque<String> data;
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
                            return fd;
                    }
                }

                int fd = ++fd_counter;
                fd_map[fd] = node;
                fd_set.insert(node);
                return fd;
            }

            void remove_fd(int fd)
            {
                auto it = fd_map.find(fd);
                if (it != fd_map.end())
                {
                    fd_set.erase(it->second);
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

        private:
            // pointers in this map are weak and do not own the object
            UnorderedMap<int, RamFsTreeNode *> fd_map;
            UnorderedSet<RamFsTreeNode *> fd_set;

            int fd_counter;
        };
    } // namespace

    struct RamFsData
    {
        RamFsFdManager fd_manager;
        Tree<RamFsTreeNode *> tree;
    };

    // placeholders
    RamFs::RamFs()
    {
        data = alloc<RamFsData>();
        data->tree.root()->name = "/";
    }

    RamFs::~RamFs() {}


    BaseFile *RamFs::open(const char *path, int flags, ...) { return nullptr; }
    BaseFile *RamFs::open(const char *path, int flags, va_list args) { return nullptr; }
    int RamFs::rename(const char *old_path, const char *new_path) { return -1; }
    int RamFs::unlink(const char *path) { return -1; }
    int RamFs::link(const char *old_path, const char *new_path) { return -1; }
    int RamFs::stat(const char *path, struct ::stat *buf) { return -1; }
    BaseRegularFile *RamFs::mkfile(const char *name, int flags, int mode) { return nullptr; }
    BaseDirectory *RamFs::mkdir(const char *name, int flags, int mode) { return nullptr; }
    BaseFifoFile *RamFs::mkfifo(const char *name, int flags, int mode) { return nullptr; }
} // namespace Hamster
