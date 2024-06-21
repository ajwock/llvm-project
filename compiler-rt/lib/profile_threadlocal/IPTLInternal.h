#ifndef IPTL_INTERNAL_H
#define IPTL_INTERNAL_H

#include "sanitizer_common/sanitizer_mutex.h"

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
      head.prev = NULL;
      head.fn = NULL;
      head.next = &tail;
      tail.prev = &head;
      tail.fn = NULL;
      tail.next = NULL;
  }
  Mutex mtx;
  texit_fn_node head;
  texit_fn_node tail;

  void register_handler(texit_fn_node *new_nodep);
  void unregister_handler(texit_fn_node *old_nodep);
  void run_handlers();
};


void register_profile_intercepts(void);
void run_thread_exit_handlers(void);

#endif IPTL_INTERNAL_H
