#ifndef BLUEWAKE_FEATURE_DISPATCH_H
#define BLUEWAKE_FEATURE_DISPATCH_H

#include "climb.h"
#include "draw_tags.h"
#include "mouse_camera.h"
#include "quick_doors.h"

/* These hooks only run at fixed GZLE01 addresses in this interval. Reject
 * addresses outside it before checking each hook's addresses/armed flags.
 * Jump remains separate: its pending press can observe a dynamic proc call. */
static inline bool bluewake_feature_observes(u32 address) {
    return bluewake_mouse_camera_observes(address) || bluewake_climb_observes(address) ||
           bluewake_quick_doors_observes(address) || bluewake_draw_tags_observes(address);
}

static inline void bluewake_feature_dispatch(CPUState* cpu, u32 address) {
    if (address - BLUEWAKE_QUICK_DOORS_ACTOR_CREATE >
        BLUEWAKE_PARTICLE_DRAW_LAST - BLUEWAKE_QUICK_DOORS_ACTOR_CREATE)
        return;
    bluewake_mouse_camera_dispatch(cpu, address);
    bluewake_climb_dispatch(cpu, address);
    bluewake_quick_doors_dispatch(cpu, address);
    bluewake_draw_tags_dispatch(cpu, address);
}

#endif
