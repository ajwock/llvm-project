

void __llvm_register_profile_intercepts();

char *__llvm_profile_begin_tls_counters(void);
char *__llvm_profile_end_tls_counters(void);

/*!
 * \brief Add counter values from TLS to the global counters for the program
 *
 * On thread exit, atomically add the values in TLS counters to the static
 * counters for the whole process.
 */
void __llvm_profile_tls_counters_finalize();
