// Hamster file

#pragma once

#include <sys/stat.h>
#include <cstdint>
#include <cstddef>
#include <cstdarg>
#include <sys/types.h>

// expose some macros
#include <fcntl.h>

namespace Hamster
{
    enum class FileType
    {
        // Normal files that are on the filesystem

        Regular,
        Directory,

        // Special files that have an entry on the filesystem,
        // but are not actually on the filesystem, but are instead
        // special files that are managed by the kernel
        //
        // All of these are represented by BaseSymbolicFile, that only serves
        // to be a placeholder on the disk with the type of the special file

        BlockDevice,
        CharDevice,
        FIFO,

        // Not actually used, and shouldn't be returned from the bases below
        // but they are here to help with possible internal management
        SymbolicLink,
        SymbolicFile,

        // Generic invalid file type
        Invalid,
    };

    class BaseFile
    {
    public:
        virtual ~BaseFile() = default;

        // Get the file type, from the ones above
        virtual const FileType type() const = 0;

        // Rename this file to the given name
        // Note that this will not move it to a new directory
        // but will instead just change the name in place
        // Name cannot contain slashes
        virtual int rename(const char *) = 0;

        // Remove this file
        virtual int remove() = 0;

        // stat the file
        virtual int stat(struct ::stat *buf) = 0;

        // change the file mode
        // This should be the same as chmod in POSIX
        virtual int chmod(int mode) = 0;

        // change the file owner
        // This should be the same as chown in POSIX
        virtual int chown(int uid, int gid) = 0;


        // Get the owner and mode

        virtual int get_uid() = 0;
        virtual int get_gid() = 0;
        virtual int get_mode() = 0;

        // returns a weak pointer, so don't modify or delete
        // data should be valid until the file is closed or renamed
        virtual const char *name() = 0;

        // optional
        virtual int get_fd() { return -1; }
    };

    class BaseRWFile : public BaseFile
    {
    public:
        virtual ssize_t read(uint8_t *buf, size_t size) = 0;
        virtual ssize_t write(const uint8_t *buf, size_t size) = 0;
    };

    class BaseRegularFile : public BaseRWFile
    {
    public:
        virtual const FileType type() const override { return FileType::Regular; }

        // Seek to the given offset
        // Equivalent to lseek in POSIX
        virtual int64_t seek(int64_t offset, int whence) = 0;

        // Get the current offset
        // Equivalent to tell in POSIX
        virtual int64_t tell() = 0;

        // Truncate the file to the given size
        // Equivalent to ftruncate in POSIX
        virtual int truncate(int64_t size) = 0;

        // Get the size of the file
        virtual int64_t size() = 0;
    };

    class BaseDirectory : public BaseFile
    {
    public:
        virtual const FileType type() const override { return FileType::Directory; }

        // List all the contents of the directory, excluding "." and ".."
        // Returns a newly allocated array of newly allocated strings
        // 
        // BOTH DIMENSIONS MUST BE DEALLOCATED LATER
        virtual char * const *list() = 0;

        // Open a path relative to this directory
        //
        // Note that unlike POSIX open, this allows for opening of directories, with
        // O_RDONLY, O_WRONLY, or O_RDWR. Enabled write would allow for the functions here
        // to modify the directory, such as creating files or directories
        // Reading would be considered opening or listing the directory or its contents
        BaseFile *open(const char *name, int flags, ...)
        {
            va_list args;
            va_start(args, flags);
            BaseFile *file = open(name, flags, args);
            va_end(args);
            return file;
        }

        // Open a path relative to this directory
        //
        // Note that unlike POSIX open, this allows for opening of directories, with
        // O_RDONLY, O_WRONLY, or O_RDWR. Enabled write would allow for the functions here
        // to modify the directory, such as creating files or directories
        // Reading would be considered opening or listing the directory or its contents
        //
        // Also unlike POSIX open, this allows the creation of directories through
        // O_CREAT | O_DIRECTORY
        virtual BaseFile *open(const char *name, int flags, va_list args) = 0;

        // Make a file in this directory
        // Name is a name, not a path, so it cannot contain slashes
        // Equivalant to open(name, (flags&~O_DIRECTORY)|O_CREAT, mode)
        //
        // One of two things could happen if the child already exists:
        // 1. error = EEXIST, and nullptr is returned
        // 2. The file is opened with the given flags, and the mode is ignored
        virtual BaseFile *mkfile(const char *name, int flags, int mode) = 0;

        // Make a directory in this directory
        // Name is a name, not a path, so it cannot contain slashes
        // Equivalant to open(name, flags|O_CREAT|O_DIRECTORY, mode)
        //
        // One of two things could happen if the child already exists:
        // 1. error = EEXIST, and nullptr is returned
        // 2. The directory is opened with the given flags, and the mode is ignored
        virtual BaseFile *mkdir(const char *name, int flags, int mode) = 0;
        
        // Create a symbolic file of the given type
        // Returns a pointer to the newly created file
        // or nullptr on error. Note that this is only required to work with
        // FIFO, block and character devices. Also note that this is not like POSIX mknod
        //
        // Name should not contain slashes, as it is not a path
        //
        // One of two things could happen if the child already exists:
        // 1. error = EEXIST, and nullptr is returned
        // 2. The file is opened with the given flags, and the mode is ignored
        virtual BaseFile *mksfile(const char *name, int flags, FileType type, int mode) = 0;

        using BaseFile::remove;

        // Remove a filesystem entry
        // Returns 0 on success, negative value on error
        // Equivelant to POSIX remove
        //
        // Note that this must not follow soft links, but instead remove them
        virtual int remove(const char *name) = 0;
    };

    class BaseSymbolicFile : public BaseFile
    {
        // empty class that is used to represent special files
    };

    class BaseFilesystem
    {
    public:
        virtual ~BaseFilesystem() = default;

        // For the differences between this and the POSIX open, see the comment on BaseDirectory::open
        BaseFile *open(const char *path, int flags, ...)
        {
            va_list args;
            va_start(args, flags);
            BaseFile *file = open(path, flags, args);
            va_end(args);
            return file;
        }

        // For the differences between this and the POSIX open, see the comment on BaseDirectory::open
        virtual BaseFile *open(const char *path, int flags, va_list args) = 0;
        virtual int rename(const char *oldpath, const char *newpath) = 0;
        virtual int link(const char *oldpath, const char *newpath) = 0;
        virtual int unlink(const char *path) = 0;
        
        virtual BaseDirectory *mkdir(const char *path, int flags, int mode) = 0;
        virtual BaseRegularFile *mkfile(const char *path, int flags, int mode) = 0;
        virtual BaseSymbolicFile *mksfile(const char *path, int flags, FileType type, int mode) = 0;
        
        // do not follow soft links
        virtual FileType type(const char *path) = 0;

        virtual int symlink(const char *oldpath, const char *newpath) = 0;
        virtual int readlink(const char *path, char *buf, size_t bufsiz) = 0;

        // Returns a newly allocated string with the target of the symlink
        // MUST use Hamster::alloc to allocate the string
        virtual char *readlink(const char *path) = 0;

        virtual int stat(const char *path, struct ::stat *buf) = 0;
        virtual int lstat(const char *path, struct ::stat *buf) = 0;

        virtual bool is_busy() { return false; }
    };
} // namespace Hamster

