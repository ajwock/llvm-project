#include "profile/InstrProfiling.h"
#include "sanitizer_common/sanitizer_mutex.h"
#include "IPTL.h"
#include "IPTLInternal.h"
#include <stdlib.h>

// Maintain a linked list of handlers to run on thread exit.
// This is broken out into a dylib so that the registry is truly global across
// dlopen et. al.
//
// Each module has a statically allocated node that gets linked into the
// registry on the constructor and that gets linked out of the registry on
// destroy.
//
// This node is defined in the static portion of the tls counts extension.


static inline texit_fn_node *take_nodep(texit_fn_node **nodepp) {
  texit_fn_node *nodep = *nodepp;
  *nodepp = NULL;
  return nodep;
}

static inline texit_fn_node *replace_nodep(texit_fn_node **nodepp,
                                           texit_fn_node *new_nodep) {
  texit_fn_node *nodep = *nodepp;
  *nodepp = new_nodep;
  return nodep;
}

void TexitFnRegistry::register_handler(texit_fn_node *new_nodep) {
    // TODO: guard pattern locking
    this->mtx.Lock();
    texit_fn_node *prev = replace_nodep(&this->tail.prev, new_nodep);
    texit_fn_node *next = replace_nodep(&prev->next, new_nodep);
    new_nodep->next = next;
    new_nodep->prev = prev;
    this->mtx.Unlock();
}

void TexitFnRegistry::unregister_handler(texit_fn_node *old_nodep) {
    // TODO: guard pattern locking
    this->mtx.Lock();
    texit_fn_node *prev = take_nodep(&old_nodep->prev);
    texit_fn_node *next = take_nodep(&old_nodep->next);
    prev->next = next;
    next->prev = prev;
    this->mtx.Unlock();
}

void TexitFnRegistry::run_handlers() {
    this->mtx.ReadLock();
    for (texit_fn_node *node = this->head.next; node != &this->tail; node = node->next) {
        if (node->fn != NULL)
            node->fn();
    }
    this->mtx.ReadUnlock();
}

__attribute__((constructor))
static void __initialize_tls_exit_registry() {
  register_profile_intercepts();
}


struct TexitFnRegistry texit_registry;

void run_thread_exit_handlers(void) {
    texit_registry.run_handlers();
}

extern "C" {

void register_tls_prfcnts_module_thread_exit_handler(texit_fn_node *new_nodep) {
    texit_registry.register_handler(new_nodep);
}
void unregister_tls_prfcnts_module_thread_exit_handler(texit_fn_node *old_nodep) {
    texit_registry.unregister_handler(old_nodep);
}

void flush_main_thread_counters(void) {
  static int flushed = 0;
  if (!flushed) {
    run_thread_exit_handlers();
    flushed = 1;
  }
}
}
