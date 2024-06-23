#include "IPTL.h"
#include "IPTLInternal.h"
#include "IPTLThreadRegistry.h"
#include "sanitizer_common/sanitizer_common.h"
#include "sanitizer_common/sanitizer_mutex.h"

using namespace __sanitizer;

ThreadContext::ThreadContext(u32 tid) : ThreadContextBase(tid), needs_cleanup(false), dtv(nullptr) {}

// Responsible for merging this function's counters into the global ones
void ThreadContext::CleanupIfNeeded() {
    // TODO:  Does sanitizer common have a oncelock?
    bool expected = true;
    if (!__atomic_compare_exchange_n(&needs_cleanup, &expected, false, false, __ATOMIC_RELAXED, __ATOMIC_RELAXED)) {
        return;
    }

    // Do counter merge
    //
    // TODO: Use dtv.  We're dtvpilled.  Remember?
    run_thread_exit_handlers();
}

void ThreadContext::OnDead() {
    CleanupIfNeeded();
}
void ThreadContext::OnJoined(void *arg) {
    CleanupIfNeeded();
}
void ThreadContext::OnFinished() {
    CleanupIfNeeded();
}
void ThreadContext::OnStarted(void *arg) {
    __atomic_store_n(&needs_cleanup, true, __ATOMIC_RELEASE);
    dtv = (void ***) arg;
}
void ThreadContext::OnCreated(void *arg) {
}
void ThreadContext::OnReset() {
    CleanupIfNeeded();
}
void ThreadContext::OnDetached(void *arg) {
    CleanupIfNeeded();
}
