#ifndef BLUEWAKE_CLIMB_H
#define BLUEWAKE_CLIMB_H

#include "core/cpu.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Climbing (optional, BLUEWAKE_CLIMB=1): Link climbs any plain wall the way he
// climbs ivy, Breath of the Wild style, on a stamina wheel. Walking or jumping
// into a steep wall grabs it; climbing and hanging on it use stamina (hanging
// still, less), and when the wheel runs out he lets go and falls, and cannot
// climb again until it has refilled on the ground. Ivy, ladders, ledges he
// pulls himself onto, walls he sidles along and blocks he pushes keep the
// game's own behaviour, and ivy costs no stamina.
//
// It is the game's ivy climbing (daPy_lk_c::setFrontWallType, the climb procs
// and setMoveBGCorrectClimb), told that a plain wall (wall code 0) is ivy
// (code 1): at setFrontWallType, only where the game itself found a steep wall
// Link faces that goes on above where he grabs ledges (not a ledge to pull
// himself onto; a wall to sidle along stays one); while climbing, each frame,
// while there is stamina. So its animations, the wall's steepness limit, climbing
// onto the top, and falling off are the game's.
//
//   BLUEWAKE_CLIMB=1                on
//   BLUEWAKE_CLIMB_STAMINA=12       seconds of climbing on a full wheel
//   BLUEWAKE_CLIMB_TRACE=1          log grabs, falls and the wheel

void bluewake_climb_attach(CPUState* cpu);
// Reads the settings again (the options menu).
void bluewake_climb_reload(void);

// The wheel for the overlay (any thread): false when it is hidden. `x`, `y`
// are where it goes in the game's picture (0..1 from its top left), `fraction`
// the stamina left, `exhausted` when it ran out and is refilling, `alpha` its
// fade.
bool bluewake_climb_hud(float* fraction, bool* exhausted, float* x, float* y, float* aspect,
                        float* alpha);

// At every dispatch boundary: a flag when off, a few compares when on.
extern bool bluewake_climb_on;
void bluewake_climb_hook(CPUState* cpu, u32 address);
static inline bool bluewake_climb_observes(u32 address) {
    return bluewake_climb_on &&
        (address == 0x8010F0DCu || address == 0x8010F554u || address == 0x80135FE4u ||
         address == 0x80122D30u || address == 0x8017C350u);
}
static inline void bluewake_climb_dispatch(CPUState* cpu, u32 address) {
    if (__builtin_expect(bluewake_climb_observes(address), 0))
        bluewake_climb_hook(cpu, address);
}

#ifdef __cplusplus
}
#endif

#endif
