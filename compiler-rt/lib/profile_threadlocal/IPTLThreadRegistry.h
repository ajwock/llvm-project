#ifndef ITPL_THREAD_REGISTRY_H
#define ITPL_THREAD_REGISTRY_H

#include "sanitizer_common/sanitizer_thread_registry.h"

using namespace __sanitizer;

class ThreadContext final : public ThreadContextBase {
 public:
  explicit ThreadContext(u32 tid);
  ~ThreadContext();

  // Override superclass callbacks.
  void OnDead() override;
  void OnJoined(void *arg) override;
  void OnFinished() override;
  void OnStarted(void *arg) override;
  void OnCreated(void *arg) override;
  void OnReset() override;
  void OnDetached(void *arg) override;

  // To be run when thread has finished execution.
  void CleanupIfNeeded();

  bool needs_cleanup;
  void ***dtv;
};

ThreadContextBase *thread_factory(u32 tid);

#endif
