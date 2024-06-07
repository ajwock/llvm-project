#include "InstrProfilingTLS.h"

struct texit_func_handler texit_registry = {0};

static void lock_texit_registry(void) {
    int expected = 0;
    while (!__atomic_compare_exchange_n(&texit_registry.texit_mtx, &expected, 1, 0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
        expected = 0;
    }
}

static void unlock_texit_registry(void) {
    __atomic_store_n(&texit_registry.texit_mtx, 0, __ATOMIC_RELEASE);
}

static void wlock_texit_registry(void) {
    lock_texit_registry();
}

static void wunlock_texit_registry(void) {
    unlock_texit_registry();
}

static void rlock_texit_registry(void) {
    lock_texit_registry();
}

static void runlock_texit_registry(void) {
    unlock_texit_registry();
}

// TODO:  Was too lazy/scared to figure out if I could use an actual lock, and also this current
// implementation might be better served with memfence anyways rather than a shit ton of
// atomic load/stores
void register_thread_exit_handler(texit_fnc f) {
    wlock_texit_registry();
    unsigned int cnt = __atomic_load_n(&texit_registry.texit_fnc_count, __ATOMIC_RELAXED);
    if (cnt >= NUM_TEXIT_FNCS) {
        //TODO: diag or panic
        goto unlock;
    }
    __atomic_store_n(&texit_registry.texit_fncs[cnt], f, __ATOMIC_RELAXED);
    __atomic_store_n(&texit_registry.texit_fnc_count, cnt + 1, __ATOMIC_RELAXED);

unlock:
    wunlock_texit_registry();
}

void run_thread_exit_handlers() {
    rlock_texit_registry();
    unsigned int num_fns = __atomic_load_n(&texit_registry.texit_fnc_count, __ATOMIC_RELAXED);
    __atomic_thread_fence(__ATOMIC_ACQUIRE);
    for (unsigned int i = 0; i < num_fns; i++) {
        texit_registry.texit_fncs[i]();
    }
    runlock_texit_registry();
}
