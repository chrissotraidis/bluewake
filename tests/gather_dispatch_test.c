#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include "gather_pipe.h"
#include "dispatch_loop.h"

static unsigned delivered, dispatched, service_calls, mode;
static void bytes(const u8* data, u32 size) {
    assert(size == 1 && data[0] == dispatched);
    delivered++;
}
static void word(u64 value, u8 size) {
    (void)value; (void)size;
    assert(0 && "batch should be enabled");
}
static int dispatch(CPUState* cpu, u32 address) {
    dispatched++;
    cpu->pc = address + 4;
    if (mode != 1 && !(mode == 6 && dispatched > 1))
        cpu->downcount--;
    if (mode == 2)
        cpu->exception = 1;
    if (mode == 3)
        cpu->downcount = -cpu->cycle_budget;
    bw_gather_pipe_put(dispatched, 1);
    return mode != 4;
}
static bool service(void* user, CPUState* cpu, u32 address) {
    (void)user; (void)cpu; (void)address;
    assert(bw_gather_pipe_length == 0 && delivered == dispatched);
    service_calls++;
    return mode == 5;
}
typedef int (*Run)(CPUState*, u32, BluewakeCompositeDispatchFn,
                   BluewakeEdgeServiceFn, void*);
static void check(Run run) {
    for (mode = 0; mode < 7; ++mode) {
        CPUState cpu = {0};
        cpu.cycle_budget = 3;
        delivered = dispatched = service_calls = 0;
        assert(run(&cpu, 0x80001000u, dispatch, service, NULL) == (mode != 4));
        assert(delivered == dispatched && bw_gather_pipe_length == 0);
        assert(dispatched == (mode == 0 ? 3u : mode == 6 ? 10u : 1u));
        assert(service_calls == (mode == 0 ? 2u : mode == 5 ? 1u : mode == 6 ? 9u : 0u));
    }
    CPUState cpu = {0};
    delivered = dispatched = 0;
    mode = 0;
    assert(run(&cpu, 0x80001000u, dispatch, NULL, NULL) == 1);
    assert(delivered == 1 && bw_gather_pipe_length == 0);
    assert(run(NULL, 0, dispatch, service, NULL) == 0);
    assert(run(&cpu, 0, NULL, service, NULL) == 0);
}
int main(void) {
    bw_gather_pipe_write = word;
    bw_gather_pipe_bytes = bytes;
    check(bluewake_chassis_dispatch_loop);
    check(bluewake_composite_dispatch_until_boundary);
    puts("gather dispatch boundaries passed");
    return 0;
}
