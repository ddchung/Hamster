// Hamster config

#pragma once

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

// Maximum pipe buffer size, in bytes
#define HAMSTER_MAX_PIPE_BUFFERED 512


/* Page table layout config
    [      L1      ]    // L1 is root
    [L2][L2][L2][L2]    // (1 << L1_BITS) amount of L2 in L1
                /  \
  _____________/    |
 / [LEAF][LEAF][LEAF]   // (1 << L2_BITS) amount of LEAF in L2
               /    |
  ____________/     |
 / [PAGE][PAGE][PAGE]   // (1 << LEAF_BITS) amount of PAGE in LEAF
               /    |
  ____________/     |
 / 01 23 45 67 89 AB    // PAGE_SIZE bytes in a PAGE
   CD EF 01 23 45 67
          ...
*/

// Note: L1_BITS + L2_BITS + L3_BITS + PAGE_SIZE_BITS must equal 32
#define HAMSTER_PAGETABLE_L1_BITS 9
#define HAMSTER_PAGETABLE_L2_BITS 7
#define HAMSTER_PAGETABLE_LEAF_BITS 4
#define HAMSTER_PAGE_SIZE_BITS 12

// compatibility
#define HAMSTER_PAGE_SIZE (1 << HAMSTER_PAGE_SIZE_BITS)
static_assert((HAMSTER_PAGE_SIZE & (HAMSTER_PAGE_SIZE - 1)) == 0, "Page size must be a power of 2");
