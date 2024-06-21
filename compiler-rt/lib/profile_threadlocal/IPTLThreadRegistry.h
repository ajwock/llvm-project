#ifdef IPTL_THREAD_REGISTRY_H
#ifdef IPTL_THREAD_REGISTRY_H

#include "sanitizer_common/sanitizer_thread_registry.h"

class ThreadContext final : ThreadContextBase {
 public:
    explicit ThreadContext(Tid, tid);
    ~ThreadContext();
    void ***dtv;

    void OnDead() override;
    void OnJoined(void *arg) override;
    void OnFinished() override;
    void OnStarted(void *arg) override;
    void OnCreated(void *arg) override;
    void OnReset() override;
    void OnDetached() override;
};

#endif
