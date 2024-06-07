#if defined(__linux__) || defined(__FreeBSD__) || defined(__Fuchsia__) || \
    (defined(__sun__) && defined(__svr4__)) || defined(__NetBSD__) || \
    defined(_AIX)

#include <elf.h>
#include <link.h>
#endif
#include <stdlib.h>
#include <string.h>

extern "C" {

#include "InstrProfiling.h"
#include "InstrProfilingInternal.h"
}

#include "interception/interception.h"

static void register_profile_intercepts();

extern "C" {

#define PROF_TLS_CNTS_START INSTR_PROF_SECT_START(INSTR_PROF_TLS_CNTS_COMMON)
#define PROF_TLS_CNTS_STOP INSTR_PROF_SECT_STOP(INSTR_PROF_TLS_CNTS_COMMON)

extern char PROF_TLS_CNTS_START COMPILER_RT_VISIBILITY COMPILER_RT_WEAK;
extern char PROF_TLS_CNTS_STOP COMPILER_RT_VISIBILITY COMPILER_RT_WEAK;


// Clarity:  This is where the tls counters SECTION begins.  This
// is not where the TLS section for this module begins on this thread-
// it's merely useful for calculating the offset of 
// __llvm_prf_tls_cnts in the PH_TLS program header.
COMPILER_RT_VISIBILITY char *__llvm_profile_begin_tls_counters(void) {
  return &PROF_TLS_CNTS_START;
}
COMPILER_RT_VISIBILITY char *__llvm_profile_end_tls_counters(void) {
  return &PROF_TLS_CNTS_STOP;
}

void __llvm_profile_tls_counters_finalize(void) {
    return;
}

struct pthread_wrapper_arg {
    void *(*fn)(void *);
    void *arg;
    uint32_t arg_keepalive;
};

void *pthread_fn_wrapper(void *arg_ptr) {
    struct pthread_wrapper_arg *wrapper_arg = (struct pthread_wrapper_arg *) arg_ptr;
    void *(*fn)(void *) = __atomic_load_n(&wrapper_arg->fn, __ATOMIC_RELAXED);
    void *arg = __atomic_load_n(&wrapper_arg->arg, __ATOMIC_RELAXED);
    __atomic_store_n(&wrapper_arg->arg_keepalive, 0, __ATOMIC_RELEASE);

    // startup
    // Do nothing (TLS is automatically loaded and zeroed)
    void *retval = fn(arg);
    // cleanup
    __llvm_profile_tls_counters_finalize();
    // Combine counters with main counters
    return retval;
}

void __llvm_register_profile_intercepts() {
    register_profile_intercepts();
}

} // end extern "C"

INTERCEPTOR(int, pthread_create, void *thread, void *attr, void *(*start_routine)(void *), void *arg) {
    int res = -1;
    struct pthread_wrapper_arg wrapper_arg = {
        (void *(*)(void *)) start_routine,
        arg,
        1
    };

    // do pthread
    res = REAL(pthread_create)(thread, attr, pthread_fn_wrapper, &wrapper_arg);
    // Spin wait for child thread to copy arguments
    while (__atomic_load_n(&wrapper_arg.arg_keepalive, __ATOMIC_ACQUIRE) == 1);
    return res;
}

static void register_profile_intercepts() {
    INTERCEPT_FUNCTION(pthread_create);
}
