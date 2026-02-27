
#include "cthread.h"
#include "arch.h"
#include <setjmp.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <assert.h>
#include <string.h>

struct cthread {
    jmp_buf context;
    void *stack_bottom;
    void (*func)(void *);
    void *arg;
    cthread_t id;
};

struct cthread_slot {
    struct cthread thread;
    int in_use;
};

// cthread global state

static int cthread_initialized = 0;
static size_t cthread_max_threads;
static struct cthread_slot *cthread_slots;
static struct cthread *cthread_current_thread = NULL;
static jmp_buf cthread_main_context;
static size_t cthread_num_threads = 0;

extern size_t CTHREAD_MAX_THREADS;
extern size_t CTHREAD_STACK_SIZE;

__attribute__((weak))
size_t CTHREAD_MAX_THREADS = 32;
__attribute__((weak))
size_t CTHREAD_STACK_SIZE = 32768;

enum {
    CTHREAD_SETJMP_STATUS_OK = 1,
    CTHREAD_SETJMP_STATUS_EXIT = 2,
};




// (size_t)-1 if not found
static size_t get_free_slot(void) {
    for (size_t i = 0; i < cthread_max_threads; ++i) {
        if (!cthread_slots[i].in_use) {
            return i;
        }
    }
    return (size_t)-1;
}

// For initial running of thread only
__attribute__((noreturn))
static void cthread_thread_trampoline(void) {
    assert(cthread_current_thread);
    void (*func)(void *) = cthread_current_thread->func;
    void *arg = cthread_current_thread->arg;

    func(arg);
    cthread_exit();

    // ?
    abort();
}

static void cthread_cleanup_thread(struct cthread *thread) {
    assert(thread->id >= 0 && (size_t)thread->id < cthread_max_threads);
    cthread_slots[thread->id].in_use = 0;
    cthread_free(thread->stack_bottom);
    memset(thread, 0, sizeof(struct cthread));
    assert(cthread_num_threads > 0);
    --cthread_num_threads;
}

static void cthread_run_thread_initial(struct cthread *thread) {
    assert(thread);
    assert(thread->func);
    assert(thread->id >= 0 && (size_t)thread->id < cthread_max_threads);
    assert(cthread_slots[thread->id].in_use);
    assert(!cthread_current_thread);

    int res = setjmp(cthread_main_context);
    if (res == 0) {
        cthread_current_thread = thread;
        cthread_run_new_stack(cthread_thread_trampoline, (char *)thread->stack_bottom + CTHREAD_STACK_SIZE);
    } else {
        // returned from thread
        memset(cthread_main_context, 0, sizeof(jmp_buf));
        cthread_current_thread = NULL;
        switch (res) {
        case CTHREAD_SETJMP_STATUS_OK:
            // thread yielded
            break;
        case CTHREAD_SETJMP_STATUS_EXIT:
            cthread_cleanup_thread(thread);
            break;
        }
    }
}

// Runs until yield
static void cthread_run_thread(struct cthread *thread) {
    assert(thread);
    assert(thread->func);
    assert(thread->id >= 0 && (size_t)thread->id < cthread_max_threads);
    assert(cthread_slots[thread->id].in_use);
    assert(!cthread_current_thread);

    cthread_current_thread = thread;

    int res = setjmp(cthread_main_context);
    if (res == 0) {
        // first time, run thread
        longjmp(thread->context, 1);
    } else {
        memset(cthread_main_context, 0, sizeof(jmp_buf));
        cthread_current_thread = NULL;
        switch (res) {
        case CTHREAD_SETJMP_STATUS_OK:
            break;
        case CTHREAD_SETJMP_STATUS_EXIT:
            cthread_cleanup_thread(thread);
            break;
        }
    }
}

void cthread_init(void) {
    if (cthread_initialized) {
        assert(!"cthread_init called multiple times");
        abort();
    }

    // save value of max threads, in case it's overridden and changed
    cthread_max_threads = CTHREAD_MAX_THREADS;

    cthread_slots = cthread_malloc(cthread_max_threads * sizeof(struct cthread_slot));
    if (!cthread_slots) {
        // can't do much, but at least crash early
        assert(!"Failed to allocate cthread_slots");
        abort();
    }

    memset(cthread_slots, 0, cthread_max_threads * sizeof(struct cthread_slot));

    cthread_initialized = 1;
}

int cthread_create(cthread_t *id, void (*func)(void *), void *arg) {
    if (!cthread_initialized) {
        assert(!"cthread_create called before cthread_init");
        abort();
    }

    size_t slot = get_free_slot();
    if (slot == (size_t)-1) {
        errno = EAGAIN;
        return -1;
    }

    void *stack = cthread_malloc(CTHREAD_STACK_SIZE);
    if (!stack) {
        errno = ENOMEM;
        return -1;
    }

    assert(slot < cthread_max_threads);
    assert(!cthread_slots[slot].in_use);

    struct cthread *thread = &cthread_slots[slot].thread;
    memset(thread, 0, sizeof(struct cthread));
    thread->stack_bottom = stack;
    thread->func = func;
    thread->arg = arg;
    thread->id = (cthread_t)slot;
    cthread_slots[slot].in_use = 1;
    ++cthread_num_threads;

    if (id) {
        *id = thread->id;
    }

    cthread_run_thread_initial(thread);

    return 0;
}

void cthread_yield(void) {
    if (!cthread_current_thread) {
        // yield from main thread, run random thread
        size_t to_run = rand() % cthread_num_threads;
        for (size_t i = 0; i < cthread_max_threads; ++i) {
            if (cthread_slots[i].in_use && to_run-- == 0) {
                cthread_run_thread(&cthread_slots[i].thread);
                break;
            }
        }
    } else {
        // yield from thread, return to main context
        int res = setjmp(cthread_current_thread->context);
        if (res == 0) {
            longjmp(cthread_main_context, CTHREAD_SETJMP_STATUS_OK);
        } else {
            // returned from main context, resume thread
            memset(cthread_current_thread->context, 0, sizeof(jmp_buf));
        }
    }
}

cthread_t cthread_get_current_thread(void) {
    if (!cthread_current_thread) {
        return -1;
    }
    return cthread_current_thread->id;
}

size_t cthread_get_num_threads(void) {
    return cthread_num_threads;
}

void *cthread_get_arg(cthread_t id) {
    if (id < 0 || (size_t)id >= cthread_max_threads || !cthread_slots[id].in_use) {
        errno = EINVAL;
        return NULL;
    }
    return cthread_slots[id].thread.arg;
}

void (*cthread_get_func(cthread_t id))(void *) {
    if (id < 0 || (size_t)id >= cthread_max_threads || !cthread_slots[id].in_use) {
        errno = EINVAL;
        return NULL;
    }
    return cthread_slots[id].thread.func;
}

void cthread_exit(void) {
    if (!cthread_current_thread) {
        assert(!"cthread_exit called from outside of thread");
        abort();
    }

    int res = setjmp(cthread_current_thread->context);
    if (res == 0) {
        longjmp(cthread_main_context, CTHREAD_SETJMP_STATUS_EXIT);
    }

    // Should never happen
    assert(!"Returned to thread after exit");
    abort();
}
