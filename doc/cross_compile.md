# Cross compiling your own user-space programs

## Prerequisites
- A cross-compiler that targets `riscv32-linux-musl` (e.g. `riscv32-linux-musl-gcc`). For the rest of the guide, we assume your cross compiler GCC is called `riscv32-linux-musl-gcc` and is in your PATH
- A mounted root filesystem image on your host system, so you can edit it. See ... on how to do that.
TODO: link

## Compiling a simple C program
1. Create a simple C program, e.g. `hello.c`:
    ```c
    #include <stdio.h>

    int main() {
        printf("Hello, Hamster!\n");
        return 0;
    }
    ```
2. Compile the program using your cross-compiler:
    ```sh
    riscv32-linux-musl-gcc -o hello hello.c
    ```
3. Copy the compiled binary to your mounted root filesystem image:
    ```sh
    cp hello rootfs/ # Copy into the root of the filesystem
    ```

## Running your program in Hamster
1. Boot Hamster as you normally would.
2. Login with your user credentials.
3. Navigate to the directory where you copied your program:
    ```sh 
    cd /
    ```
4. Run your program:
    ```sh
    ./hello
    # Prints: Hello, Hamster!
    ```

