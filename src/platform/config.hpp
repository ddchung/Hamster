// Hamster config

#pragma once


// The size of each page
#define HAMSTER_PAGE_SIZE 4096
static_assert((HAMSTER_PAGE_SIZE & (HAMSTER_PAGE_SIZE - 1)) == 0, "Page size must be a power of 2");

// The absolute maximum number of pages
// RAM pages + swapped pages
#define HAMSTER_MAX_PAGES 16384

// The stack top, leave some space above for reserved data
#define HAMSTER_STACK_TOP 0xFFFF0000

// The length of each thread's time slice, in # of instructions
#define HAMSTER_THREAD_TIME_SLICE 1024

// The maximum overrun time in the kernel scheduler, before the task is considered
// overdue and is removed from the scheduler, in milliseconds
#define HAMSTER_KSCHED_OVERDUE_TIME 1000

// The target amount of free RAM for the page manager, in bytes
// This controls the "swappiness" of the system
// Ensure it's not too low or too high:
// - too low: The system will keep too many pages in RAM, possibly causing an out-of-memory situation
// - too high: The system will swap out too many pages, possibly causing performance issues
#define HAMSTER_TARGET_FREE_RAM (sizeof(void *) * HAMSTER_PAGE_SIZE)
