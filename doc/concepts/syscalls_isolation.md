# System Calls and Isolation

## Isolation

Isolation is a fundamental concept in modern operating systems that ensures that different programs have separate resources, and can opt into sharing some resources with others if needed. This is crucial for security, stability, and resource management. Isolation ensures that a misbehaving program cannot interfere with other programs or the operating system itself, and prevents malicious software from accessing sensitive data or system resources. This is achieved by providing each program with its own virtual address space, file descriptors, and other resources.

```
# Program A and Program B are not able to access each other's memory
# This is enforced by the kernel and hardware

----------------------                ----------------------
| Program A          |                | Program B          |
|                    |                |                    |
| ------------------ |      \ /       | ------------------ |
| |Virtual Memory A| | <=====X======> | |Virtual Memory B| |
| ------------------ |      / \       | ------------------ |
|                    |     Access     |                    |
---------------------- Others' Memory ----------------------
                        Not Allowed
```

## System Calls

However, if each program were fully isolated, they would not be able to do anything useful, as they can't interact with the outside world. Thus, kernels provide a controlled "system call" interface. System calls allow for a program to ask the kernel to do something on its behalf, such as reading from a file, requesting more memory, or printing something onto the screen. This allows the kernel to validate every operation and ensure that the program is only allowed to do what it is permitted to do.

```
# Programs must request the kernel to perform privileged actions

                 Validate Request
                /
================|===    Request action (system call)
Kernel      |   v  |      /
            |      |     /      --------------
-------     |      | <========= |  Program   |
Data  |     |System|            |            |
      |     |Calls |            |            |
    <=========>    |            |            |
------- |   |      | =========> |            |
        |   |      |  Result    --------------
        |   |      |
        |   |      |
========|===========
        |
 Privileged Access
```

There are almost 100 system calls in Hamster, which is a strict subset of Linux's system call interface (over 300 calls!). Each one does something different, for example, there are system calls to read from or write to a file, to make new threads or processes, to request more memory, etc. Anything that has a real action in userspace programs uses system calls.

```c
// This will print a message onto the terminal
// Internally, the C library will call the write() system call to print text onto the terminal, and possibly other system calls that it needs
puts("Hello, World!");

// This will allocate 20 bytes
// The C library usually pre-reserves memory,
// but if that runs out, it will call the
// brk() or mmap() system call to request more memory
void *p = malloc(20);

// Here, we make another process
// The C library will use the clone() system call with the correct arguments to request the kernel to make another process
fork();
```

## The C library

What might seem like a system call might not actually be! Almost all of the functions you use from the ISO C and POSIX standard libraries are actually the C library's wrapper functions. The C library defines many wrapper functions that call the system calls, but might do extra checks or handle portability issues internally, or might even provide completely different behavior. For example, the `mmap()` function provided in `sys/mman.h` is actually the C library's wrapper function that calls the necessary assembly. The `clone()` *function* is very different from the clone *system call*.

```c
// This is musl's mmap wrapper
// As you can see here, musl's mmap code actually does more than just calling the system call
// It first does some safety checks, before computing some different parameters to pass to the system call
void *__mmap(void *start, size_t len, int prot, int flags, int fd, off_t off)
{
	long ret;
	if (off & OFF_MASK) {
		errno = EINVAL;
		return MAP_FAILED;
	}
	if (len >= PTRDIFF_MAX) {
		errno = ENOMEM;
		return MAP_FAILED;
	}
	if (flags & MAP_FIXED) {
		__vm_wait();
	}
	ret = __syscall(SYS_mmap2, start, len, prot, flags, fd, off/UNIT);
	/* Fixup incorrect EPERM from kernel. */
	if (ret == -EPERM && !start && (flags&MAP_ANON) && !(flags&MAP_FIXED))
		ret = -ENOMEM;
	return (void *)__syscall_ret(ret);
}
```

However, some functions *do* just call the kernel, like getrandom():

```c
// This is musl's getrandom wrapper
// This one does nothing more than call the system call
// and return the result
ssize_t getrandom(void *buf, size_t buflen, unsigned flags)
{
	return syscall_cp(SYS_getrandom, buf, buflen, flags);
}
```

Thus, the C library is very important and is almost always the middleman while talking to the kernel (and in some operating systems, like OpenBSD, it is required as a security measure).

```
# The program talks to the C library, which in turn will request the appropriate system call from the kernel
-----------        -------------       -------------
| Kernel  |  <===> | C Library | <===> | C program |
-----------        -------------       -------------
       ^                                 ^
        \===============X===============/
      Never used (in vast majority of programs)
      program does not talk directly to kernel
```
