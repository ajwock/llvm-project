// C++ only header
#ifndef IPTL_INTERNAL_H
#define IPTL_INTERNAL_H

#include "sanitizer_common/sanitizer_mutex.h"
#include "sanitizer_common/sanitizer_thread_registry.h"

#include "IPTLThreadRegistry.h"

using namespace __sanitizer;

extern "C" {

typedef void (*texit_fnc)(void);

struct texit_fn_node {
  struct texit_fn_node *prev;
  texit_fnc fn;
  struct texit_fn_node *next;
};


void register_tls_prfcnts_module_thread_exit_handler(texit_fn_node *);
void unregister_tls_prfcnts_module_thread_exit_handler(texit_fn_node *);

}

class TexitFnRegistry {
public:
  TexitFnRegistry() : mtx() {
      head.prev = nullptr;
      head.fn = nullptr;
      head.next = &tail;
      tail.prev = &head;
      tail.fn = nullptr;
      tail.next = nullptr;
  }
  Mutex mtx;
  texit_fn_node head;
  texit_fn_node tail;

  void register_handler(texit_fn_node *new_nodep);
  void unregister_handler(texit_fn_node *old_nodep);
  void run_handlers();
};

class InstrProfThreadLocalCtx final {
public:
    InstrProfThreadLocalCtx() : texit_handlers(),
        thread_registry(&thread_factory) {}

    TexitFnRegistry     texit_handlers;
    ThreadRegistry      thread_registry;
};

extern InstrProfThreadLocalCtx ctx;

void register_profile_intercepts(void);
void run_thread_exit_handlers(void);

#endif
