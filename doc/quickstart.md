# Quickstart Guide

This guide will help you get started with Hamster, and boot a simple user space

## Prerequisites
Make sure you have:
- `g++` in your PATH (clang++ on macOS should be fine)
- POSIX-compliant host system (e.g. Linux, macOS)
- Bash (or equivalent) shell

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
    nano test.c # Edit a file using nano
    gcc test.c -o test # Compile a C program
    ./test # Run the compiled program
    ```
4. To exit Hamster, simply exit the shell as you normally would:
    ```sh
    exit
    ```
    or press `Ctrl+D`.

## Additional Resources

### Compiling your own user-space programs
It is not recommended to use the provided GCC for compiling big programs, since it is very slow, and there is no build system installed. Please use a cross-compiler on your host system instead. You can find more information on how to set up a cross-compiler in the [Cross-Compiling Guide](/doc/cross_compile.md).
