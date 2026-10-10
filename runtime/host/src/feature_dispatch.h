#ifndef BLUEWAKE_FEATURE_DISPATCH_H
#define BLUEWAKE_FEATURE_DISPATCH_H

#include "audio_watch.h"
#include "climb.h"
#include "draw_tags.h"
#include "forest_water.h"
#include "mouse_camera.h"
#include "quick_doors.h"
#include "stage_select.h"

/* These hooks only run at fixed GZLE01 addresses in this interval. Reject
 * addresses outside it before checking each hook's addresses/armed flags.
 * Jump remains separate: its pending press can observe a dynamic proc call. */
static inline bool bluewake_feature_observes(u32 address) {
    return bluewake_mouse_camera_observes(address) || bluewake_climb_observes(address) ||
           bluewake_quick_doors_observes(address) || bluewake_draw_tags_observes(address) ||
           bluewake_forest_water_observes(address) || bluewake_audio_watch_observes(address) ||
           bluewake_stage_select_observes(address);
}

static inline void bluewake_feature_dispatch(CPUState* cpu, u32 address) {
    // Forest Water's, the audio watch's and the stage select's hooks are outside the interval below.
    bluewake_forest_water_dispatch(cpu, address);
    bluewake_audio_watch_dispatch(cpu, address);
    bluewake_stage_select_dispatch(cpu, address); // fopScnM_ChangeReq is below the interval
    if (address - BLUEWAKE_QUICK_DOORS_ACTOR_CREATE >
        BLUEWAKE_PARTICLE_DRAW_LAST - BLUEWAKE_QUICK_DOORS_ACTOR_CREATE)
        return;
    bluewake_mouse_camera_dispatch(cpu, address);
    bluewake_climb_dispatch(cpu, address);
    bluewake_quick_doors_dispatch(cpu, address);
    bluewake_draw_tags_dispatch(cpu, address);
}

#endif
