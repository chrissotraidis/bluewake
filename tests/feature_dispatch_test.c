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
bool bluewake_forest_water_enabled;
u32 bluewake_forest_water_tree_timer_check;
static unsigned calls[6];
static unsigned order;
static u32 camera_seen[8];
static unsigned camera_seen_count;
void bluewake_mouse_camera_hook(CPUState* cpu, u32 address) {
    (void)cpu; ++calls[0]; order = order * 6 + 1;
    if (camera_seen_count < 8)
        camera_seen[camera_seen_count++] = address;
}
void bluewake_climb_hook(CPUState* cpu, u32 address) {
    (void)cpu; (void)address; ++calls[1]; order = order * 6 + 2;
}
void bluewake_quick_doors_enter(CPUState* cpu) {
    (void)cpu; ++calls[2]; order = order * 6 + 3;
}
void bluewake_draw_tags_enter(CPUState* cpu, u32 address) {
    (void)cpu; (void)address; ++calls[3]; order = order * 6 + 4;
}
void bluewake_forest_water_enter(CPUState* cpu, u32 address) {
    (void)cpu; (void)address; ++calls[4]; order = order * 6 + 0;
}
void bluewake_audio_watch_cue(CPUState* cpu) {
    (void)cpu; ++calls[5]; order = order * 6 + 5;
}

static void compare_dispatch(u32 address) {
    memset(calls, 0, sizeof calls);
    order = 0;
    bluewake_forest_water_dispatch(NULL, address);
    bluewake_audio_watch_dispatch(NULL, address);
    bluewake_mouse_camera_dispatch(NULL, address);
    bluewake_climb_dispatch(NULL, address);
    bluewake_quick_doors_dispatch(NULL, address);
    bluewake_draw_tags_dispatch(NULL, address);
    unsigned expected[6];
    const unsigned expected_order = order;
    assert(bluewake_feature_observes(address) ==
           (calls[0] + calls[1] + calls[2] + calls[3] + calls[4] + calls[5] != 0));
    memcpy(expected, calls, sizeof calls);
    memset(calls, 0, sizeof calls);
    order = 0;
    bluewake_feature_dispatch(NULL, address);
    assert(memcmp(expected, calls, sizeof calls) == 0);
    assert(order == expected_order);
    /* The turn-entry form is the same less Forest Water, which the host
     * dispatches at turn start itself. Forest Water runs first and adds no
     * digit to the order, so the order is unchanged. */
    memset(calls, 0, sizeof calls);
    order = 0;
    bluewake_feature_dispatch_turn_entry(NULL, address);
    expected[4] = 0;
    assert(memcmp(expected, calls, sizeof calls) == 0);
    assert(order == expected_order);
}

/* main.c's edge service: the features first, then whether it returns to the host. */
static bool edge_service(BluewakeTurnEntry* entry, u32 address, bool returns) {
    bluewake_feature_dispatch(NULL, address);
    return bluewake_turn_entry_edge(entry, address, returns);
}

static void check_turn_entry_handoff(void) {
    const u32 bump = BLUEWAKE_MOUSE_CAMERA_BUMP;
    const u32 draw = BLUEWAKE_MOUSE_CAMERA_DRAW;
    BluewakeTurnEntry entry = {BLUEWAKE_TURN_ENTRY_NONE};

    /* A turn starts at a hooked block no edge saw (the last turn's budget ran
     * out just before it): observed once. */
    camera_seen_count = 0;
    bluewake_turn_entry_begin(&entry, NULL, bump);
    assert(camera_seen_count == 1 && camera_seen[0] == bump);

    /* An edge observed it and returned to the host without dispatching it: the
     * turn that dispatches it does not observe it again. */
    camera_seen_count = 0;
    assert(edge_service(&entry, bump, true));
    bluewake_turn_entry_begin(&entry, NULL, bump);
    assert(camera_seen_count == 1 && camera_seen[0] == bump);

    /* Returned to the host, which sent the turn elsewhere: the new pc is observed. */
    camera_seen_count = 0;
    assert(edge_service(&entry, bump, true));
    bluewake_turn_entry_begin(&entry, NULL, draw);
    assert(camera_seen_count == 2 && camera_seen[0] == bump && camera_seen[1] == draw);

    /* The mark lasts one turn: the next turn at the same pc is observed. */
    camera_seen_count = 0;
    assert(edge_service(&entry, bump, true));
    bluewake_turn_entry_begin(&entry, NULL, bump);
    bluewake_turn_entry_begin(&entry, NULL, bump);
    assert(camera_seen_count == 2);
    assert(entry.edge_returned_pc == BLUEWAKE_TURN_ENTRY_NONE);

    /* An edge that let the chassis go on leaves no mark. */
    camera_seen_count = 0;
    assert(!edge_service(&entry, bump, false));
    assert(entry.edge_returned_pc == BLUEWAKE_TURN_ENTRY_NONE);
    bluewake_turn_entry_begin(&entry, NULL, bump);
    assert(camera_seen_count == 2);
}

int main(void) {
    /* Forest Water's tree check is in a module, outside MEM1. */
    bluewake_forest_water_tree_timer_check = 0xC13B1850u;
    for (unsigned flags = 0; flags < 16; ++flags) {
        bluewake_climb_on = (flags & 1) != 0;
        bluewake_quick_doors_armed = (flags & 2) != 0;
        bluewake_draw_tags_enabled = (flags & 4) != 0;
        bluewake_forest_water_enabled = (flags & 8) != 0;
        /* Every aligned MEM1 address, including the complete particle/wake
         * ranges, not just a list that could omit a newly added hook. */
        for (u32 address = 0x80000000u; address < 0x81800000u; address += 4)
            compare_dispatch(address);
        const u32 edges[] = {0, 0xFFFFFFFFu, 0xC0000000u, 0x81E00000u, 0xC13B1850u,
                            BLUEWAKE_QUICK_DOORS_ACTOR_CREATE - 1,
                            BLUEWAKE_QUICK_DOORS_ACTOR_CREATE,
                            BLUEWAKE_QUICK_DOORS_ACTOR_CREATE + 1,
                            BLUEWAKE_PARTICLE_DRAW_LAST - 1,
                            BLUEWAKE_PARTICLE_DRAW_LAST,
                            BLUEWAKE_PARTICLE_DRAW_LAST + 1};
        for (size_t i = 0; i < sizeof edges / sizeof edges[0]; ++i)
            compare_dispatch(edges[i]);
    }
    check_turn_entry_handoff();
    puts("Feature dispatch: all aligned MEM1 addresses and sixteen flag combinations agree");
    return 0;
}
