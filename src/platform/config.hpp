// Hamster config

#pragma once

// The stack top, leave some space above for reserved data
#define HAMSTER_STACK_TOP (96 * 1024 * 1024)

// The length of each thread's time slice, in # of instructions
#ifdef ARDUINO
#define HAMSTER_THREAD_TIME_SLICE 2048
#else
#define HAMSTER_THREAD_TIME_SLICE 32768
#endif

// The maximum overrun time in the kernel scheduler, before the task is considered
// overdue and is removed from the scheduler, in milliseconds
#define HAMSTER_KSCHED_OVERDUE_TIME 1000

// The target amount of free RAM for the page manager, in bytes
// This controls the "swappiness" of the system
// Ensure it's not too low or too high:
// - too low: The system will keep too many pages in RAM, possibly causing an out-of-memory situation
// - too high: The system will swap out too many pages, possibly causing performance issues
#define HAMSTER_TARGET_FREE_RAM (512 * 1024)

// Maximum memory pressure for disk caching
#define HAMSTER_DISK_FREE_RAM (1 * 1024 * 1024)

// Maximum pipe buffer size, in bytes
#define HAMSTER_MAX_PIPE_BUFFERED 512

// Maximum number of pages per process
//
// The addressable range of each process is:
// 0x00000000...(PAGE_SIZE * PAGES_PER_PROC)
//
// For example, if PAGES_PER_PROC=32768 and PAGE_SIZE=4096:
// 0x00000000...0x08000000 (128MiB)
#define HAMSTER_PAGES_PER_PROC 32768

// warning: Changing this won't adversely affect the kernel, but RISC-V linux
//          userspace programs expect a 4096-byte page size, and so changing this
//          will probably break all the programs
#define HAMSTER_PAGE_SIZE_BITS 12

// compatibility
#define HAMSTER_PAGE_SIZE (1 << HAMSTER_PAGE_SIZE_BITS)
static_assert((HAMSTER_PAGE_SIZE & (HAMSTER_PAGE_SIZE - 1)) == 0, "Page size must be a power of 2");

// Likely/unlikely

#define HAMSTER_LIKELY(x) (__builtin_expect(!!(x), 1))
#define HAMSTER_UNLIKELY(x) (__builtin_expect(!!(x), 0))
