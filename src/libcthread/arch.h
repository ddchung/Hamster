// libcthread platform-specifics

#ifndef LIBCTHREAD_ARCH_H
#define LIBCTHREAD_ARCH_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Note: None of these functions are required to be thread-safe

// Run a function on a new stack
// Aligns stack_top suitably
// `func` will not return, but please spin indefinitely as a last resort
__attribute__((noreturn, noinline))
void cthread_run_new_stack(void (*func)(void), void *stack_top);

// Allocation

void *cthread_malloc(size_t size);
void cthread_free(void *ptr);

#ifdef __cplusplus
}
#endif

#endif // LIBCTHREAD_ARCH_H
