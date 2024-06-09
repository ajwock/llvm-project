#include "InstrProfilingTLS.h"
#include "InstrProfiling.h"

struct texit_fn_node module_node COMPILER_RT_VISIBILITY;

__attribute__((constructor))
COMPILER_RT_VISIBILITY
void __llvm_profile_tls_register_thread_exit_handler(void) {
    module_node.prev = NULL;
    module_node.next = NULL;
    module_node.fn = __llvm_profile_tls_counters_finalize;
    register_tls_prfcnts_module_thread_exit_handler(&module_node);
}

// TODO: Add destructor
// (But not yet, I'm scared)
