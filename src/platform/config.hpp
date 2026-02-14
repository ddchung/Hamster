// Hamster config

#pragma once

// Note: If you want to modify some of these, create a 'config.local.hpp'
//       file in either the src/ dir or the port/{platform}/ dir
#if __has_include ("config.local.hpp")
# include "config.local.hpp"
#endif

#if __has_include ("config.platform.hpp")
# include "config.platform.hpp"
#endif

// Define to 1 if you want to disable colors in the logger
#ifndef HAMSTER_LOGGER_NO_COLORS
# define HAMSTER_LOGGER_NO_COLORS 0
#endif

// The stack top, leave some space above for reserved data
#ifndef HAMSTER_STACK_TOP
# define HAMSTER_STACK_TOP (120 * 1024 * 1024)
#endif

// Stack size
#ifndef HAMSTER_STACK_SIZE
# define HAMSTER_STACK_SIZE (8 * 1024 * 1024)
#endif

// ioctl pointer max structure size. Must be less than HAMSTER_PAGE_SIZE
#ifndef HAMSTER_MAX_IOCTL_SIZE
# define HAMSTER_MAX_IOCTL_SIZE 512
#endif

// The length of each thread's time slice, in # of instructions
#ifndef HAMSTER_THREAD_TIME_SLICE
# define HAMSTER_THREAD_TIME_SLICE 65536
#endif

// The maximum number of cached instructions in the trace cache
#ifndef HAMSTER_TRACE_SIZE
# define HAMSTER_TRACE_SIZE 128
#endif

// The target amount of free RAM for the page manager, in bytes
// This controls the "swappiness" of the system
// Ensure it's not too low or too high:
// - too low: The system will keep too many pages in RAM, possibly causing an out-of-memory situation
// - too high: The system will swap out too many pages, possibly causing performance issues
#ifndef HAMSTER_TARGET_FREE_RAM
# define HAMSTER_TARGET_FREE_RAM (512 * 1024)
#endif

// Maximum memory pressure for disk caching
#ifndef HAMSTER_DISK_FREE_RAM
# define HAMSTER_DISK_FREE_RAM (1 * 1024 * 1024)
#endif

// Maximum pipe buffer size, in bytes
#ifndef HAMSTER_MAX_PIPE_BUFFERED
# define HAMSTER_MAX_PIPE_BUFFERED 512
#endif

// Maximum number of pages per process
//
// The addressable range of each process is:
// 0x00000000...(PAGE_SIZE * PAGES_PER_PROC)
//
// For example, if PAGES_PER_PROC=32768 and PAGE_SIZE=4096:
// 0x00000000...0x08000000 (128MiB)
#ifndef HAMSTER_PAGES_PER_PROC
# define HAMSTER_PAGES_PER_PROC 32768
#endif

// Page table page ID type
// uint16_t or uint32_t. This defines the maximmum pages in the whole system
#ifndef HAMSTER_PAGE_ID_TYPE
# define HAMSTER_PAGE_ID_TYPE uint16_t
#endif

// Maximum global total number of futexes
#ifndef HAMSTER_MAX_FUTEXES
# define HAMSTER_MAX_FUTEXES 1024
#endif

// Maximum number of file descriptors per process
// (actually the max per TaskFDTable, but there is usually one per process)
#ifndef HAMSTER_MAX_FD_TABLE_SIZE
# define HAMSTER_MAX_FD_TABLE_SIZE 128
#endif

// warning: Changing this won't adversely affect the kernel, but RISC-V linux
//          userspace programs expect a 4096-byte page size, and so changing this
//          will probably break all the programs
#ifndef HAMSTER_PAGE_SIZE_BITS
# define HAMSTER_PAGE_SIZE_BITS 12
#endif

// can't modify this
#define HAMSTER_PAGE_SIZE (1 << HAMSTER_PAGE_SIZE_BITS)

// Likely/unlikely

#ifndef HAMSTER_LIKELY
# define HAMSTER_LIKELY(x) (__builtin_expect(!!(x), 1))
#endif
#ifndef HAMSTER_UNLIKELY
# define HAMSTER_UNLIKELY(x) (__builtin_expect(!!(x), 0))
#endif

// uname

#ifndef HAMSTER_SYSNAME
# define HAMSTER_SYSNAME "Hamster"
#endif
#ifndef HAMSTER_NODENAME
# define HAMSTER_NODENAME "localhost"
#endif
#ifndef HAMSTER_RELEASE
# define HAMSTER_RELEASE "dev"
#endif
#ifndef HAMSTER_VERSION
# define HAMSTER_VERSION "0.0"
#endif
#ifndef HAMSTER_MACHINE
# define HAMSTER_MACHINE "riscv32"
#endif
#ifndef HAMSTER_DOMAINNAME
# define HAMSTER_DOMAINNAME "localdomain"
#endif

