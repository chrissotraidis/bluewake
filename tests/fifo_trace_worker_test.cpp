// SPDX-License-Identifier: GPL-3.0-or-later
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <atomic>
#include <cassert>
#include <cstdlib>
#include <thread>
// Compile the production start decision with a bounded synthetic worker.
static bool g_trace_armed;
static bool in_trace_window;
static bool g_fifo_worker_started;
static bool g_worker_mode;
static std::thread g_fifo_worker_thread;
static std::atomic<unsigned> worker_runs;
static bool trace_should_record() { return g_trace_armed && in_trace_window; }
static void g_fifo_worker_main() { ++worker_runs; }
#include "fifo_worker_start_under_test.inc"
static void join_worker() {
    if (g_fifo_worker_thread.joinable()) g_fifo_worker_thread.join();
    g_fifo_worker_started = false;
    g_worker_mode = false;
}
int main() {
    g_trace_armed = true;
    in_trace_window = false;
    assert(!trace_should_record());
    g_fifo_worker_start();
    const bool started_before_capture = g_fifo_worker_started;
    join_worker();
    assert(!started_before_capture);
    assert(worker_runs == 0);
    in_trace_window = true;
    g_fifo_worker_start();
    assert(!g_fifo_worker_started);
    g_trace_armed = false;
    for (unsigned i = 0; i < 100; ++i) {
        g_fifo_worker_start();
        assert(g_fifo_worker_started && g_worker_mode);
        g_fifo_worker_start(); // Already started: do not replace a joinable thread.
        join_worker();
    }
    assert(worker_runs == 100);
}
