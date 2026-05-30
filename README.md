# Hamster: Linux-compatible OS
This is a toy operating system project called Hamster, designed to be compatible with Linux. This is my personal project for learning about operating systems and low-level programming. It is **NOT** intended for production use. You are free to do whatever with this code, but please remember it is just a learning project, and probably has huge security holes and some bugs.

## Features
- Linux-compatible system calls
    - Implements a subset of Linux system calls
    - Tested with: uClibc-ng (patched), musl (static, dynamic), glibc (static)
- Emulates user-space programs, able to run almost everywhere
    - Runs on microcontrollers, as a userspace program on Linux, etc.
    - Easily ported: just implement the functions in `src/platform/platform.hpp`
    - Emulation allows binaries to be used everywhere, regardless of host architecture
- Basic file system support
    - Ram filesystem
    - romfs

---
[Quickstart](/doc/quickstart.md)

---
This project is a work in progress. Feel free to fork your own and experiment

Made by: Tin Dao
