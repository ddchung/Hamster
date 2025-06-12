# Filesystem Documentation

This document is not independent, but it goes with the comments in the relevant headers.

## The "Filesystem" Structure

Before we get into this file, it would help to clarify what exactly a theoretical "filesystem" is. A filesystem is a data structure that is a tree, where each node is identified by a name, and each one also has a distinct type. We call each node a "file"

These are the file types:
- Regular
- Directory
- Symbolic link
- Special

All file nodes contain some metadata:
- **mode**: The permissions
- **uid**, **gid**: The ownership of the file

Here are the distinct file operations for each type

### Regular File

The regular file is a very common type of file that contains some arbitrary data. These are commonly used to save persistent data across runs and restarts, as well as to save data too large to keep in memory.

These are the fundamental operations that each regular file has:
- **read**: This reads data from the file at a specific position
- **write**: This will write some data to the file at some position
- **truncate**: This will either extend the file with zeros, or shorten the file, both to the specified length

### Directory

A directory is a type of file that contains other files. They are the fundamental building blocks that allow the theoretical filesystem to have a tree structure.

Here are its operations:
- **list**: This will get the names of all its direct child nodes
- **get**: This will open a direct child, identified by name
- **create**: This will create a new file of specified name and type
- **remove**: This will remove a direct child, and possibly all of its children, identified by name

### Symbolic Link

A symbolic link is essentially a redirect file, which contains a path to another file to redirect to. At this level, it is essentially just a file that contains a string.

Operations:
- **read**: This will read *the entire* string that the file contains
- **write**: This will replace *the entire* string that the file contains

Note that the read and write operations are different than a regular file's, as it can only operate on the entirety of the data at once.

Also note that the string contained represents a path, but is not validated in any way in storage.

### Special File

A special file does some arbitrary thing when you try to perform an operation on it. Theoretically, not all special files share the same set of operations, but here, we are implementing it like this: they just contain a numerical ID to index something else.

Operations:
- **get_id**: This gets the device ID

Note that the ID is set on creation, and cannot be changed

## Abstract Structures

The classes in [base_file.hpp](base_file.hpp) are for low-level filesystem drivers. We use **handles** that operate on a specific "file" in the filesystem, so that each driver can abstract away file operations, into this common interface. There may be multiple handles for each "file."

Each handle also stores some independent state that is not shared in any way across other handles. When the handle is destroyed, this state is also gone.

- **flags**: The flags are what the handle was opened with, and it will determine what operations are allowed. For example, it can be `O_RDWR | O_APPEND`, which will allow both reading and writing, but if the file is a regular file, writes will go to the end
- **offset**: This is only for regular file handles. This is the current position in the file that read and write operations will go to

Each file handle has a specific type, which is the same as its node's type. Each type of file handle has a different set of operations, however there is a common set of operations that is shared between all types.

### Shared Operations

These are the operations that all file handles have

- **rename**: This will change the name that the underlying node is identified by
- **remove**: This will delete the underlying node
- **stat**, **get_mode**, **get_uid**, **get_gid**: These get the underlying node's metadata
- **get_flags**: These get this specific file handle's flags
- **chmod**, **chown**: These change some of the node's metadata
- **set_flags**: Change the handle's flags
- **basename**: This will get the name that the underlying node is identified by

Additionally, the VFS also stores some of its per-node metadata. Note that this metadata is not persistent. This metadata is changed through `set_vfs_flags` and `get_vfs_flags`

### Regular File Handle

This type of handle operates on a regular file node. Its operations are:

- `read`: Reads data from the file, at the current offset, and it will increment the offset by the number of bytes read afterwards
- `write`: Writes data to the file, at the current offset, or if `flags | O_APPEND`, it will first set the offset to the end of the file before writing. This will also increment the offset afterwards
- `seek`: This moves the current offset, in one of three ways:
    - `SEEK_SET`: This sets the offset to an absolute position
    - `SEEK_CUR`: This changes the offset relative to where it currently is
    - `SEEK_END`: This sets the offset to a position relative to the end of the file
- `tell`: Get the current offset
- `truncate`: This will either extend the file with zeros, or delete excess data.
- `size`: Get the size of the file

### Directory Handle

The directory handle operates on a directory.

- `list`: this gets the names of all the children of the underlying node
- `get`: This creates a new *file handle* that operates on the file specified by name. It also possibly creates it, based on `flags` and POSIX semantics
- `mkfile`, `mkdir`: These create new files or directories with the specified name, and mode. It will then create a new *file handle* that operates on the newly created file.
- `mksym`: This creates a new symlink with the specified name and target. It will then create a new handle that operates on the newly created symbolic link
- `mksfile`: This creates a new special file, with the specified name and mode, and it will also set its device ID. It will then create a new handle that operates on it.
- `remove`: This deletes an underlying node, identified by name

### Symbolic Link Handle

This operates on a symbolic link node

- `get_target`: This gets the string that the symbolic link contains
- `set_target`: This replaces the contained string with the new one

### Special File Handle

- `get_device_id`: This gets the device ID of the special file

### Base Filesystem

The `BaseFilesystem` class represents all filesystems, with exactly one virtual function, `open_root`. This opens a new directory handle that operates on the filesystem's root directory.

## The VFS

The VFS encapsulates `BaseFilesystem`s, and `BaseFileHandles`s, providing an interface that operates with numerical file descriptors. Filesystems can be mounted on any directory, and the VFS handles mount and symbolic link resolution.
