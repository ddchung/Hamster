// libcthread x86_64 SysV glue

#if defined(__x86_64__) && !defined(_WIN32)

#include "arch.h"
#include <stdlib.h>

asm (
    ".weak cthread_run_new_stack\n"
    ".type cthread_run_new_stack,@function\n"
    "cthread_run_new_stack:\n"
    // rdi = func
    // rsi = stack_top

    // Align stack_top down to 16 bytes
    "    mov %rsi, %rax\n"
    "    and $-16, %rax\n"

    // SysV ABI: before call, %rsp must be 16-byte aligned.
    // Because CALL pushes an 8-byte return address,
    // we subtract 8 so that inside the called function
    // %rsp is 16-byte aligned.
    "    sub $8, %rax\n"

    // Switch to new stack
    "    mov %rax, %rsp\n"

    // Call function pointer
    "    call *%rdi\n"

    // If function returns, spin forever
    "1:\n"
    "    pause\n"
    "    jmp 1b\n"
);

__attribute__((weak))
void *cthread_malloc(size_t size) {
    return malloc(size);
}

__attribute__((weak))
void cthread_free(void *ptr) {
    free(ptr);
}

#endif // __x86_64__
