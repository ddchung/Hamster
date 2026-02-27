
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)

#include "arch.h"
#include <stdlib.h>

asm (
    ".weak cthread_run_new_stack\n"
    ".thumb_func\n"
    "cthread_run_new_stack:\n"
    // r0 = func
    // r1 = stack_top

    // Align stack_top down to 8 bytes
    "    mov r2, r1\n"
    "    bic r2, r2, #7\n"

    // Switch stack
    "    mov sp, r2\n"

    // Call function pointer
    "    bx r0\n"          // Branch to func

    // If function returns, spin forever
    "1:\n"
    "    wfi\n"            // Wait for interrupt (low-power)
    "    b 1b\n"
);

__attribute__((weak))
void *cthread_malloc(size_t size) {
    return malloc(size);
}

__attribute__((weak))
void cthread_free(void *ptr) {
    free(ptr);
}

#endif // armv7m
