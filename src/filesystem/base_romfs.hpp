// Hamster romfs driver

#pragma once

#include <filesystem/romfs_structs.hpp>
#include <filesystem/base_file.hpp>
#include <memory/allocator.hpp>
#include <memory/stl_sequential.hpp>
#include <errno/errno.h>
#include <cassert>
#include <cstring>

namespace Hamster
{
    /*
    This romfs driver uses a "backend" to abstract actual device I/O

    The backend must implement these, with at least a loosely fitting signature
    that can be called "as if" it were as specified:

    struct
    {
        ssize_t read(uint32_t pos, void *buf, size_t size);
    };
    */

    // simple stub that errors with H_EROFS
    template <typename T = int, T return_val = -1>
    inline T error_rofs()
    {
        error = H_EROFS;
        return return_val;
    }

    template <class Backend>
    class BaseRomFs : public Backend, public BaseFilesystem
    {
        // this implements BaseFile, without actually inheriting it
        // to prevent a diamond inheritance problem
        class RomFsFile
        {
        public:
            RomFsFile(BaseRomFs *fs, uint32_t loc, int oflag);
            RomFsFile(const RomFsFile &) = default;
            RomFsFile(RomFsFile &&) = default;
            RomFsFile &operator=(const RomFsFile &) = default;
            RomFsFile &operator=(RomFsFile &&) = default;
            ~RomFsFile() = default;

            BaseFilesystem *get_filesystem()
            {
                return filesystem;
            }

            int get_id() const
            {
                return file_loc;
            }

            int stat(sys_stat *buf);

            int get_mode();

            int get_flags()
            {
                return oflag;
            }

            int get_uid()
            {
                return 0;
            }

            int get_gid()
            {
                return 0;
            }

            int chmod(int mode)
            {
                return error_rofs();
            }

            int chown(int uid, int gid)
            {
                return error_rofs();
            }

            int set_flags(int flags)
            {
                oflag = flags;
                return 0;
            }

        protected:
            romfs_struct_file file_struct;
            String filename;
            BaseRomFs *filesystem;
            uint32_t file_loc;
            uint32_t filedata_loc;
            int oflag;
        };

        class RomFsRegularFile : public BaseRegularFile, public RomFsFile
        {
        public:
            using RomFsFile::RomFsFile;

            BaseFile *clone() override { return alloc<RomFsRegularFile>(1, *this); }
            BaseFilesystem *get_filesystem() override { return RomFsFile::get_filesystem(); }
            int get_id() const override { return RomFsFile::get_id(); }
            int stat(sys_stat *buf) override { return RomFsFile::stat(buf); }
            int get_mode() override { return RomFsFile::get_mode(); }
            int get_flags() override { return RomFsFile::get_flags(); }
            int get_uid() override { return RomFsFile::get_uid(); }
            int get_gid() override { return RomFsFile::get_gid(); }
            int chmod(int mode) override { return RomFsFile::chmod(mode); }
            int chown(int uid, int gid) override { return RomFsFile::chown(uid, gid); }
            int set_flags(int flags) override { return RomFsFile::set_flags(flags); }

            ssize_t pread(uint8_t *buf, size_t size, int64_t pos) override;
            ssize_t pwrite(const uint8_t *buf, size_t size, int64_t pos) override
            {
                return error_rofs<ssize_t>();
            }

            int truncate(int64_t length) override
            {
                return error_rofs();
            }

            int64_t size() override;
        };

        class RomFsSpecialFile : public BaseSpecialFile, public RomFsFile
        {
        public:
            using RomFsFile::RomFsFile;

            BaseFile *clone() override { return alloc<RomFsSpecialFile>(1, *this); }
            BaseFilesystem *get_filesystem() override { return RomFsFile::get_filesystem(); }
            int get_id() const override { return RomFsFile::get_id(); }
            int stat(sys_stat *buf) override { return RomFsFile::stat(buf); }
            int get_mode() override { return RomFsFile::get_mode(); }
            int get_flags() override { return RomFsFile::get_flags(); }
            int get_uid() override { return RomFsFile::get_uid(); }
            int get_gid() override { return RomFsFile::get_gid(); }
            int chmod(int mode) override { return RomFsFile::chmod(mode); }
            int chown(int uid, int gid) override { return RomFsFile::chown(uid, gid); }
            int set_flags(int flags) override { return RomFsFile::set_flags(flags); }

            DeviceID get_device_id() override;
        };

        class RomFsSymlink : public BaseSymlink, public RomFsFile
        {
        public:
            using RomFsFile::RomFsFile;

            BaseFile *clone() override { return alloc<RomFsSymlink>(1, *this); }
            BaseFilesystem *get_filesystem() override { return RomFsFile::get_filesystem(); }
            int get_id() const override { return RomFsFile::get_id(); }
            int stat(sys_stat *buf) override { return RomFsFile::stat(buf); }
            int get_mode() override { return RomFsFile::get_mode(); }
            int get_flags() override { return RomFsFile::get_flags(); }
            int get_uid() override { return RomFsFile::get_uid(); }
            int get_gid() override { return RomFsFile::get_gid(); }
            int chmod(int mode) override { return RomFsFile::chmod(mode); }
            int chown(int uid, int gid) override { return RomFsFile::chown(uid, gid); }
            int set_flags(int flags) override { return RomFsFile::set_flags(flags); }

            char *get_target() override;
            int set_target(const char *target) override
            {
                return error_rofs();
            }
        };

        class RomFsDirectory : public BaseDirectory, public RomFsFile
        {
        public:
            using RomFsFile::RomFsFile;

            BaseFile *clone() override { return alloc<RomFsDirectory>(1, *this); }
            BaseFilesystem *get_filesystem() override { return RomFsFile::get_filesystem(); }
            int get_id() const override { return RomFsFile::get_id(); }
            int stat(sys_stat *buf) override { return RomFsFile::stat(buf); }
            int get_mode() override { return RomFsFile::get_mode(); }
            int get_flags() override { return RomFsFile::get_flags(); }
            int get_uid() override { return RomFsFile::get_uid(); }
            int get_gid() override { return RomFsFile::get_gid(); }
            int chmod(int mode) override { return RomFsFile::chmod(mode); }
            int chown(int uid, int gid) override { return RomFsFile::chown(uid, gid); }
            int set_flags(int flags) override { return RomFsFile::set_flags(flags); }

            char *const *list(size_t count) override;
            BaseFile *get(const char *name, int flags, int mode = 0) override;

            BaseRegularFile *mkfile(const char *name, int flags, int mode) override
            {
                return error_rofs<BaseRegularFile *, nullptr>();
            }

            BaseDirectory *mkdir(const char *name, int flags, int mode) override
            {
                return error_rofs<BaseDirectory *, nullptr>();
            }

            BaseSymlink *mksym(const char *name, const char *target) override
            {
                return error_rofs<BaseSymlink *, nullptr>();
            }

            BaseSpecialFile *mksfile(const char *name, int flags, DeviceID id, int mode) override
            {
                return error_rofs<BaseSpecialFile *, nullptr>();
            }

            int link(BaseFile *file, const char *name) override
            {
                return error_rofs();
            }

            int remove(const char *name) override
            {
                return error_rofs();
            }
        };

    public:
        BaseRomFs();
        BaseDirectory *open_root(int flags) override;
        void read_name_to_string(uint32_t name_loc, String &out);
        BaseFile *get_file(uint32_t loc, int flags);
        void read_file(uint32_t loc, romfs_struct_file *out = nullptr, String *name = nullptr);

        ssize_t read(uint32_t pos, void *buf, size_t size)
        {
            return Backend::read(pos, buf, size);
        }

    private:
        romfs_struct_fs_header fs_header;
        size_t fs_start;
    };

    template <class Backend>
    BaseRomFs<Backend>::RomFsFile::RomFsFile(BaseRomFs<Backend> *fs, uint32_t loc, int oflag)
        : filesystem(fs), file_loc(loc), oflag(oflag)
    {
        filesystem->read_file(loc, &file_struct, &filename);

        filedata_loc = loc + sizeof(romfs_struct_file) + filename.size() + 1;
        filedata_loc = round_up_16(filedata_loc);
    }

    template <class Backend>
    int BaseRomFs<Backend>::RomFsFile::stat(sys_stat *buf)
    {
        if (!buf)
        {
            error = -H_EINVAL;
            return -1;
        }

        buf->size = file_struct.size;
        buf->mode = get_mode();
        buf->ino = file_loc;

        return 0;
    }

    template <class Backend>
    int BaseRomFs<Backend>::RomFsFile::get_mode()
    {
        int mode = file_struct.is_executable ? 0555 : 0444;
        
        switch (file_struct.file_type)
        {
        case romfs_type_directory:
            return mode | STAT_IFDIR;
        case romfs_type_regular:
            return mode | STAT_IFREG;
        case romfs_type_symlink:
            return mode | STAT_IFLNK;
        default:
            // Note: no special/hardlink handling here, as VFS handles special files
            // while hardlinks aren't encountered here
            return mode;
        }
    }

    template <class Backend>
    ssize_t BaseRomFs<Backend>::RomFsRegularFile::pread(uint8_t *buf, size_t size, int64_t pos)
    {
        if (pos >= this->file_struct.size)
            return 0;

        if (size + pos > this->file_struct.size)
            size = this->file_struct.size - pos;
        return this->filesystem->read(this->filedata_loc + pos, buf, size);
    }

    template <class Backend>
    int64_t BaseRomFs<Backend>::RomFsRegularFile::size()
    {
        return this->file_struct.size;
    }

    template <class Backend>
    DeviceID BaseRomFs<Backend>::RomFsSpecialFile::get_device_id()
    {
        DeviceID device_id;
        device_id.major = (this->file_struct.info >> 16) & 0xFFFF;
        device_id.minor = this->file_struct.info & 0xFFFF;
        return device_id;
    }

    template <class Backend>
    char *BaseRomFs<Backend>::RomFsSymlink::get_target()
    {
        char *str = alloc<char>(this->file_struct.size + 1);

        if (this->filesystem->read(this->filedata_loc, str, this->file_struct.size) < 0)
        {
            dealloc(str);
            return nullptr;
        }

        str[this->file_struct.size] = '\0';
        return str;
    }

    template <class Backend>
    char *const *BaseRomFs<Backend>::RomFsDirectory::list(size_t count)
    {
        Vector<char *> names;
        
        romfs_struct_file entry;
        String name;
        uint32_t next_loc = this->file_struct.info;
        while (next_loc != 0 && count != 0)
        {
            this->filesystem->read_file(next_loc, &entry, &name);
            next_loc = entry.next_file();

            count--;
            char *name_str = alloc<char>(name.size() + 1);
            strcpy(name_str, name.c_str());
            names.push_back(name_str);
        }

        char **result = alloc<char *>(names.size() + 1);
        result[names.size()] = nullptr;
        memcpy(result, names.data(), names.size() * sizeof(char *));

        return result;
    }

    template <class Backend>
    BaseFile *BaseRomFs<Backend>::RomFsDirectory::get(const char *name, int flags, int mode)
    {
        String entry_name;
        romfs_struct_file entry;
        uint32_t next_loc = this->file_struct.info;
        while (next_loc != 0)
        {
            this->filesystem->read_file(next_loc, &entry, &entry_name);

            if (entry_name == name)
            {
                return this->filesystem->get_file(next_loc, flags);
            }

            next_loc = entry.next_file();
        }

        error = flags & OPEN_CREAT ? H_EROFS : H_ENOENT;
        return nullptr;
    }

    template <class Backend>
    BaseRomFs<Backend>::BaseRomFs()
    {
        read(0, &fs_header, sizeof(fs_header));
        fs_header.init();

        _trace("Mounted romfs with volume name '");

        uint32_t pos = sizeof(fs_header);
        char c;
        do
        {
            read(pos, &c, sizeof(c));
            pos += sizeof(c);
            _trace("%c", c);
        } while (c != 0);
        fs_start = round_up_16(pos);
        _trace("'\n");
    }

    template <class Backend>
    BaseDirectory *BaseRomFs<Backend>::open_root(int flags)
    {
        return (BaseDirectory *)get_file(fs_start, flags | OPEN_DIRECTORY);
    }

    template <class Backend>
    void BaseRomFs<Backend>::read_name_to_string(uint32_t name_loc, String &out)
    {
        char c;
        out.clear();
        while (true)
        {
            read(name_loc++, &c, sizeof(c));
            if (c == 0)
                break;
            out.push_back(c);
        }
    }

    template <class Backend>
    BaseFile *BaseRomFs<Backend>::get_file(uint32_t loc, int flags)
    {
        romfs_struct_file file;
        read_file(loc, &file);

        if ((flags & OPEN_DIRECTORY) && file.file_type != romfs_type_directory
            && file.file_type != romfs_type_hardlink)
        {
            error = H_ENOTDIR;
            return nullptr;
        }

        switch (file.file_type)
        {
        case romfs_type_hardlink:
            if (file.info == loc)
            {
                error = H_ELOOP;
                return nullptr;
            }
            return get_file(file.info, flags);
        case romfs_type_directory:
            return alloc<RomFsDirectory>(1, this, loc, flags);
        case romfs_type_regular:
            return alloc<RomFsRegularFile>(1, this, loc, flags);
        case romfs_type_symlink:
            return alloc<RomFsSymlink>(1, this, loc, flags);
        case romfs_type_block:
        case romfs_type_char:
        case romfs_type_sock:
        case romfs_type_fifo:
            return alloc<RomFsSpecialFile>(1, this, loc, flags);
        }

        __builtin_unreachable();
    }

    template <class Backend>
    void BaseRomFs<Backend>::read_file(uint32_t loc, romfs_struct_file *out, String *name)
    {
        romfs_struct_file file;
        read(loc, &file, sizeof(file));
        file.init();

        if (out)
            *out = file;

        if (name)
            read_name_to_string(loc + sizeof(file), *name);
    }
} // namespace Hamster
