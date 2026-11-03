// SPDX-License-Identifier: GPL-3.0-or-later
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include "gfx/render_worker.hpp"
int main() {
    namespace worker = aurora::gfx::render_worker;
    for (unsigned i = 0; i < 1000; ++i) {
        assert(!worker::is_worker_thread());
        worker::initialize();
        worker::enqueue_work([] { assert(worker::is_worker_thread()); });
        worker::synchronize();
        assert(!worker::is_worker_thread());
        worker::shutdown();
    }
}
