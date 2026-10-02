#ifdef NDEBUG
#undef NDEBUG
#endif
#include "feature_dispatch.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

bool bluewake_climb_on;
bool bluewake_quick_doors_armed;
bool bluewake_draw_tags_enabled;
static unsigned calls[4];
static unsigned order;
void bluewake_mouse_camera_hook(CPUState* cpu, u32 address) {
    (void)cpu; (void)address; ++calls[0]; order = order * 5 + 1;
}
void bluewake_climb_hook(CPUState* cpu, u32 address) {
    (void)cpu; (void)address; ++calls[1]; order = order * 5 + 2;
}
void bluewake_quick_doors_enter(CPUState* cpu) {
    (void)cpu; ++calls[2]; order = order * 5 + 3;
}
void bluewake_draw_tags_enter(CPUState* cpu, u32 address) {
    (void)cpu; (void)address; ++calls[3]; order = order * 5 + 4;
}

static void compare_dispatch(u32 address) {
    memset(calls, 0, sizeof calls);
    order = 0;
    bluewake_mouse_camera_dispatch(NULL, address);
    bluewake_climb_dispatch(NULL, address);
    bluewake_quick_doors_dispatch(NULL, address);
    bluewake_draw_tags_dispatch(NULL, address);
    unsigned expected[4];
    const unsigned expected_order = order;
    assert(bluewake_feature_observes(address) == (order != 0));
    memcpy(expected, calls, sizeof calls);
    memset(calls, 0, sizeof calls);
    order = 0;
    bluewake_feature_dispatch(NULL, address);
    assert(memcmp(expected, calls, sizeof calls) == 0);
    assert(order == expected_order);
}

int main(void) {
    for (unsigned flags = 0; flags < 8; ++flags) {
        bluewake_climb_on = (flags & 1) != 0;
        bluewake_quick_doors_armed = (flags & 2) != 0;
        bluewake_draw_tags_enabled = (flags & 4) != 0;
        /* Every aligned MEM1 address, including the complete particle/wake
         * ranges, not just a list that could omit a newly added hook. */
        for (u32 address = 0x80000000u; address < 0x81800000u; address += 4)
            compare_dispatch(address);
        const u32 edges[] = {0, 0xFFFFFFFFu, 0xC0000000u, 0x81E00000u,
                            BLUEWAKE_QUICK_DOORS_ACTOR_CREATE - 1,
                            BLUEWAKE_QUICK_DOORS_ACTOR_CREATE,
                            BLUEWAKE_QUICK_DOORS_ACTOR_CREATE + 1,
                            BLUEWAKE_PARTICLE_DRAW_LAST - 1,
                            BLUEWAKE_PARTICLE_DRAW_LAST,
                            BLUEWAKE_PARTICLE_DRAW_LAST + 1};
        for (size_t i = 0; i < sizeof edges / sizeof edges[0]; ++i)
            compare_dispatch(edges[i]);
    }
    puts("Feature dispatch: all aligned MEM1 addresses and eight flag combinations agree");
    return 0;
}
