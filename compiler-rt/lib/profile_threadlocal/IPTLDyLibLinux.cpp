#if defined(__linux__) || defined(__FreeBSD__) || defined(__Fuchsia__) ||      \
    (defined(__sun__) && defined(__svr4__)) || defined(__NetBSD__) ||          \
    defined(_AIX)

#include <elf.h>
#include <link.h>
#endif
#include <stdlib.h>
#include <string.h>

extern "C" {

#include "profile/InstrProfiling.h"
/*
#include "profile/InstrProfilingInternal.h"
*/
#include "IPTL.h"
}

#include "sanitizer_common/sanitizer_thread_registry.h"

using namespace __sanitizer;

#include "IPTLInternal.h"
#include "interception/interception.h"

static bool find_by_dtv(ThreadContextBase *tctx, void *arg);
static ThreadContextBase *find_tctx_by_dtv(uptr dtv);

static bool find_by_dtv_cb(ThreadContextBase *tctx, void *arg) {
    return tctx->user_id == *(uptr *) arg;
}

static ThreadContextBase *find_tctx_by_dtv(uptr dtv) {
    ThreadRegistryLock l(&ctx.thread_registry);
    return ctx.thread_registry.FindThreadContextLocked(find_by_dtv_cb, &dtv);
}

extern "C" {

void ***get_self_dtv(void) {
    void *dtv_addr = NULL;
    // Here's what's weird about this.  On x86-64, a struct pthread_t
    // is at %fs:0.  At an offset of 0 within the pthread_t is a pointer
    // to the TCB.  That also happens to be wha %fs:0 is pointing to,
    // so *(void **) x = (void *) x where x is the contents of %fs. 
    // I.e., %fs:0 contains a pointer to its own address.
    //
    // This is fortunate because on x86_64, we can't actually read
    // the address contained in %fs directly.
    //
    // The DTV pointer is the next member of pthread_t, at an offset
    // of 8.
    asm("movq %%fs:0,%0\n"
        "addq $8,%0"
        : "=r"(dtv_addr));
    return (void ***) dtv_addr;
}

struct pthread_wrapper_arg {
  void *(*fn)(void *);
  void *arg;
  u32 parent_tid;
  uint32_t arg_keepalive;
};


void *pthread_fn_wrapper(void *arg_ptr) {
  struct pthread_wrapper_arg *wrapper_arg =
      (struct pthread_wrapper_arg *)arg_ptr;

  u32 parent_tid = __atomic_load_n(&wrapper_arg->parent_tid, __ATOMIC_RELAXED);
  // Use the dtv address as our user_id.
  void ***dtv = get_self_dtv();
  u32 tid = ctx.thread_registry.CreateThread((uptr) dtv, false, parent_tid, NULL);

  void *(*fn)(void *) = __atomic_load_n(&wrapper_arg->fn, __ATOMIC_RELAXED);
  void *arg = __atomic_load_n(&wrapper_arg->arg, __ATOMIC_RELAXED);
  __atomic_store_n(&wrapper_arg->arg_keepalive, 0, __ATOMIC_RELEASE);

  // TODO if needed: Get os tid for this.  I don't believe this is necessary for now
  ctx.thread_registry.StartThread(tid, 0, ThreadType::Regular, (void *) dtv); 

  // startup
  // Do nothing (TLS is automatically loaded and zeroed)
  void *retval = fn(arg);
  // cleanup
  // run_thread_exit_handlers();

  // TODO (MUST DO): runonce thread exit handlers in OnFinished
  ctx.thread_registry.FinishThread(tid);
  // Combine counters with main counters
  return retval;
}

void __llvm_register_profile_intercepts(void) { register_profile_intercepts(); }

} // end extern "C"

ThreadContextBase *thread_factory(u32 tid) {
    static LowLevelAllocator thread_factory_allocator;
    static Mutex thread_factory_mutex;

    
    Lock l(&thread_factory_mutex);
    // TODO: Use better alloc for performance.  We're using the low level allocator here.
    return new (thread_factory_allocator) ThreadContext(tid);
}

INTERCEPTOR(int, pthread_create, void *thread, void *attr,
            void *(*start_routine)(void *), void *arg) {
  int res = -1;
  struct pthread_wrapper_arg wrapper_arg = {(void *(*)(void *))start_routine,
                                            arg, 0, 1};

  // do pthread
  res = REAL(pthread_create)(thread, attr, pthread_fn_wrapper, &wrapper_arg);
  // Spin wait for child thread to copy arguments
  while (__atomic_load_n(&wrapper_arg.arg_keepalive, __ATOMIC_ACQUIRE) == 1)
    ;
  return res;
}

void register_profile_intercepts(void) { INTERCEPT_FUNCTION(pthread_create); }
