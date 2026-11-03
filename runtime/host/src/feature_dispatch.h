#ifndef BLUEWAKE_FEATURE_DISPATCH_H
#define BLUEWAKE_FEATURE_DISPATCH_H

#include "audio_watch.h"
#include "climb.h"
#include "draw_tags.h"
#include "forest_water.h"
#include "mouse_camera.h"
#include "quick_doors.h"

/* These hooks only run at fixed GZLE01 addresses in this interval. Reject
 * addresses outside it before checking each hook's addresses/armed flags.
 * Jump remains separate: its pending press can observe a dynamic proc call. */
static inline bool bluewake_feature_observes(u32 address) {
    return bluewake_mouse_camera_observes(address) || bluewake_climb_observes(address) ||
           bluewake_quick_doors_observes(address) || bluewake_draw_tags_observes(address) ||
           bluewake_forest_water_observes(address) || bluewake_audio_watch_observes(address);
}

/* The chassis services every block boundary except the block a host turn
 * starts on, which the host dispatches itself. The host calls this for that
 * block; Forest Water has its own turn-start call. */
static inline void bluewake_feature_dispatch_turn_entry(CPUState* cpu, u32 address) {
    // The audio watch's hooks are outside the interval below.
    bluewake_audio_watch_dispatch(cpu, address);
    if (address - BLUEWAKE_QUICK_DOORS_ACTOR_CREATE >
        BLUEWAKE_PARTICLE_DRAW_LAST - BLUEWAKE_QUICK_DOORS_ACTOR_CREATE)
        return;
    bluewake_mouse_camera_dispatch(cpu, address);
    bluewake_climb_dispatch(cpu, address);
    bluewake_quick_doors_dispatch(cpu, address);
    bluewake_draw_tags_dispatch(cpu, address);
}

static inline void bluewake_feature_dispatch(CPUState* cpu, u32 address) {
    // Forest Water's hook is outside the interval above.
    bluewake_forest_water_dispatch(cpu, address);
    bluewake_feature_dispatch_turn_entry(cpu, address);
}

/* The handoff between the two: when the edge service observes an address and
 * returns to the host without dispatching it, the turn that dispatches it must
 * not observe it again. */
#define BLUEWAKE_TURN_ENTRY_NONE 0xFFFFFFFFu
typedef struct BluewakeTurnEntry {
    u32 edge_returned_pc;
} BluewakeTurnEntry;

// The edge service's result, passed through; a return to the host marks its address.
static inline bool bluewake_turn_entry_edge(BluewakeTurnEntry* entry, u32 address, bool returned) {
    if (returned)
        entry->edge_returned_pc = address;
    return returned;
}

// Just before the host dispatches a turn's first block, at pc.
static inline void bluewake_turn_entry_begin(BluewakeTurnEntry* entry, CPUState* cpu, u32 pc) {
    if (pc != entry->edge_returned_pc)
        bluewake_feature_dispatch_turn_entry(cpu, pc);
    entry->edge_returned_pc = BLUEWAKE_TURN_ENTRY_NONE;
}

#endif
