// Hamster config

#pragma once


// The size of each page
#define HAMSTER_PAGE_SIZE 4096
static_assert((HAMSTER_PAGE_SIZE & (HAMSTER_PAGE_SIZE - 1)) == 0, "Page size must be a power of 2");

// The number of pages loaded in RAM at once, per process
#define HAMSTER_CONCUR_PAGES 16

// The absolute maximum number of pages
// RAM pages + swapped pages
#define HAMSTER_MAX_PAGES 16384

// The stack top, leave some space above for reserved data
#define HAMSTER_STACK_TOP 0xFFFF0000

// The length of each thread's time slice, in # of instructions
#define HAMSTER_THREAD_TIME_SLICE 1024
