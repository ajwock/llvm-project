#ifndef INSTR_PROFILING_TLS_H
#define INSTR_PROFILING_TLS_H

void __llvm_register_profile_intercepts();

char *__llvm_profile_begin_tls_counters(void);
char *__llvm_profile_end_tls_counters(void);


/*!
 * \brief Add counter values from TLS to the global counters for the program
 *
 * On thread exit, atomically add the values in TLS counters to the static
 * counters for the whole process.
 */
void __llvm_profile_tls_counters_finalize(void);

typedef void (*texit_fnc)(void);


#define NUM_TEXIT_FNCS 128

// TODO: really this should be write-preferring rwlocked
struct texit_func_handler {
    int texit_mtx;
    texit_fnc texit_fncs[NUM_TEXIT_FNCS];
    unsigned int texit_fnc_count;
};

void register_thread_exit_handler(texit_fnc f);

void run_thread_exit_handlers(void);

#endif
