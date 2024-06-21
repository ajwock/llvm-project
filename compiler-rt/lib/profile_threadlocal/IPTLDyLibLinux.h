#ifndef INSTR_PROF_DYLIB_LINUX_H
#define INSTR_PROF_DYLIB_LINUX_H

typedef struct thread_entry {
  // Pointer to the glibc DTV
  void ***dtv;
} thread_entry;


void ***get_self_dtv(void);

// tid will actually be equal to the dtv***
void register_thread(uint64_t tid);
void teardown_thread(uint64_t tid);

#endif
