#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include "../windows/src/restart_request.h"
#include "../runtime/host/src/host_stop.h"
int main() {
    RestartRequest restart;
    int events = 0;
    assert(!restart.take());
    assert(!restart.request(false, [&] { ++events; return true; }));
    assert(events == 0 && !restart.take());
    assert(!restart.request(true, [&] { ++events; return false; }));
    assert(!restart.take());
    assert(restart.request(true, [&] { ++events; return true; }));
    assert(restart.take() && !restart.take());
    assert(bw_host_stop_status(nullptr) == 0);
    assert(bw_host_stop_status("quit") == 0);
    assert(bw_host_stop_status("exception") == 1);
}
