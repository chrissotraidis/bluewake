// Cancellation belongs to an import, not to malformed input. Keep it out of
// FormatError catches, which intentionally skip unsupported archive members.
#pragma once

#include <atomic>
#include <stdexcept>

namespace wwhd {
struct Cancelled : std::runtime_error {
    Cancelled() : std::runtime_error("cancelled") {}
};

// Readers also serve callers without a job. A scope preserves those APIs and
// restores a caller's context on every exit. PNG workers establish their own
// scope; unrelated threads never inherit an import's token.
inline thread_local const std::atomic<bool>* import_cancel = nullptr;

class CancellationScope {
  public:
    explicit CancellationScope(const std::atomic<bool>* token) : previous_(import_cancel) { import_cancel = token; }
    ~CancellationScope() { import_cancel = previous_; }
    CancellationScope(const CancellationScope&) = delete;
    CancellationScope& operator=(const CancellationScope&) = delete;
  private:
    const std::atomic<bool>* previous_;
};

inline bool cancellation_requested() {
    return import_cancel && import_cancel->load(std::memory_order_relaxed);
}

inline void check_cancel() {
    if (cancellation_requested())
        throw Cancelled();
}

#ifdef WWHD_IMPORT_TESTING
// Implemented only by the generated-fixture executable. No environment or
// test callback is consulted by production builds.
void import_test_checkpoint(const char* name);
#define WWHD_CHECKPOINT(name) do { ::wwhd::import_test_checkpoint(name); ::wwhd::check_cancel(); } while (false)
#else
#define WWHD_CHECKPOINT(name) ::wwhd::check_cancel()
#endif
} // namespace wwhd
