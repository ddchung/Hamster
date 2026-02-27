# libcthread - C Cooperative Threading Library

A simple and relatively portable cooperative threading library, written in C

## How to use in your project

Simply put the entire source tree where all the *.c files will get compiled, and include cthread.h.

## Supported platforms

GCC and similar compilers should be fine. These architectures will be supported:
- x86_64 SysV ABI
- aarch64 AAPCS64 ABI
- ARMv7-M

> [!WARNING]
> At least on x86_64 Linux, this does NOT work with address sanitizer!
> However, valgrind seems to work fine, although with some scary-looking warnings.
> 
> From my testing, it seems like asan plays fine on aarch64 Linux, but it might not be
> true on all machines.

## Porting

Port the things in arch.h if your platform isn't supported yet or the current support isn't OK.
The builtins use weak linkage, so you are free to override.

To set the maximum number of threads, define `size_t CTHREAD_MAX_THREADS` to your value somewhere,
and set it before calling `cthread_init()`

To set the stack size other than the default 32KiB, define `size_t CTHREAD_STACK_SIZE`, and set the value
before calling `cthread_create`. Threads created before will not be affected. This is changeable dynamically for new threads.
