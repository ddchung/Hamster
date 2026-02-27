// libcthread aarch64 glue

#ifdef __aarch64__

#include "arch.h"
#include <stdlib.h>

asm (
    ".weak cthread_run_new_stack\n"
    "cthread_run_new_stack:\n" // void cthread_run_new_stack(void (*func)(void), void *stack_top)
    "    ldr x3, =0xFFFFFFFFFFFFFFF0\n" // ~0xF
    "    and x1, x1, x3\n" // Align stack_top to 16 bytes
    "    mov sp, x1\n" // Set stack pointer to stack_top
    "    blr x0\n"      // Call the function pointed to by x0
    // If the function returns, spin indefinitely
    "1:\n"
    "    wfe\n"        // Wait for event (low power)
    "    b 1b\n"       // Loop back to wait for event
);

__attribute__((weak))
void *cthread_malloc(size_t size) {
    return malloc(size);
}

__attribute__((weak))
void cthread_free(void *ptr) {
    free(ptr);
}

#endif // __aarch64__
