// SPDX-License-Identifier: GPL-3.0-or-later
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <atomic>
#include <cassert>
#include <thread>
#include <type_traits>
#include "aurora_backend_private.h"
template <class T> static auto snapshot(const T& value) {
    if constexpr (std::is_arithmetic_v<T>) return value;
    else return value.load(std::memory_order_relaxed);
}
int main() {
    using namespace gx_aurora;
    constexpr unsigned n = 200000;
    g_shadow_frontend_failed = false;
    g_core_submitted = 0;
    g_core_rejected = 0;
    std::atomic<bool> ready{false};
    std::atomic<unsigned long long> observed{0};
    std::thread reader([&] {
        ready.store(true, std::memory_order_release);
        for (unsigned i = 0; i < n; ++i)
            observed.fetch_add(snapshot(g_core_submitted) + snapshot(g_core_rejected) +
                               snapshot(g_shadow_frontend_failed), std::memory_order_relaxed);
    });
    while (!ready.load(std::memory_order_acquire)) std::this_thread::yield();
    for (unsigned i = 0; i < n; ++i) {
        ++g_core_submitted;
        ++g_core_rejected;
        g_shadow_frontend_failed = (i & 1) != 0;
    }
    reader.join();
    assert(snapshot(g_core_submitted) == n && snapshot(g_core_rejected) == n);
}
