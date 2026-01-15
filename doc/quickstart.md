# Quickstart Guide

This guide will help you get started with Hamster, and boot a simple user space

## Prerequisites
Make sure you have:
- `g++` in your PATH (clang++ on macOS should be fine)
- POSIX-compliant host system (e.g. Linux, macOS)
- Bash (or equivalent) shell
- `xz` utility for decompressing files
- `git` for cloning the repository

## Building Hamster
1. Clone the repository:
    ```sh
    git clone https://github.com/tindao64/Hamster.git
    cd Hamster
    ```
2. Run the build script:
    ```sh
    # This will compile all the necessary source files
    ./build.sh build
    ```
3. Extract the root filesystem image:
    ```sh
    unxz rootfs.img.xz
    ```

## Running Hamster
1. After building, you can run Hamster with:
    ```sh
    ./hamster
    ```
2. You will see a login prompt. Login with the default user:
    - Username: `root`
    - Password: `1234`
3. You can now run user-space programs! For example:
    ```sh
    ls # List files in the current directory
    echo "Hello, Hamster!" # Print a message
    nano file.txt # Edit a file using nano
    cat file.txt # Display the contents of a file
    ```
4. To exit Hamster, simply exit the shell as you normally would:
    ```sh
    exit
    ```
    or press `Ctrl+D`.

## Additional Resources

### Read-write root filesystem (Linux hosts only)
By default, Hamster uses a read-only root filesystem image. If you want a
read-write root filesystem that persists changes, you can enable native filesystem support
on Linux.

First, mount/extract the `rootfs.img` file to somewhere on your host system:
```sh
mkdir /tmp/hamster_rootfs
sudo mount -o loop rootfs.img /tmp/hamster_rootfs
```

Then, copy the contents to a new directory:
```sh
mkdir ~/hamster_rw_rootfs
cp -a /tmp/hamster_rootfs/* ~/hamster_rw_rootfs/
sudo umount /tmp/hamster_rootfs
rmdir /tmp/hamster_rootfs
```

Ensure you have the right permissions:
```sh
sudo chown -R $(whoami):$(whoami) ~/hamster_rw_rootfs
chmod -R u+rw ~/hamster_rw_rootfs
```

Then, edit `src/platform/native/native_fs.hpp` and change the `LINUX_NATIVE_FS` from `0` to `1`.

After that, make a symlink from your new root filesystem directory to `rootfs`:
```sh
ln -s ~/hamster_rw_rootfs rootfs
```

Finally, rebuild Hamster as above and run it. You should now have a read-write root filesystem!


### Cross-compiling your own programs
If you have a cross compiler targeting riscv32-linux-musl, you can compile
your own programs for Hamster. To avoid libc version mismatches, it's recommended
to statically link your custom programs. There are a variety of C programs in `root`'s home directory.

Example compilation command:
```sh
riscv32-unknown-linux-musl-gcc -static -o hello hello_world.c
```

