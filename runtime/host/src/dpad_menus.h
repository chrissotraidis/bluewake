#ifndef BLUEWAKE_DPAD_MENUS_H
#define BLUEWAKE_DPAD_MENUS_H

#include "core/cpu.h"
#include "dpad_menus_map.h"
#include "gxruntime/platform.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// The D-pad in the item screens (#245). The Items and Quest Status screens
// move their cursor with the control stick only (dMenu's STControl), and read
// no D-pad direction. While one of them is open and the stick rests, a D-pad
// direction on player 1's controller becomes a full push of the stick that
// way, and the direction itself is not passed on. Everywhere else, including
// the sea chart and dungeon maps (which use the D-pad themselves), nothing
// changes.
//
//   BLUEWAKE_DPAD_MENUS=1   on (off by default)

// Once the guest is running.
void bluewake_dpad_menus_attach(CPUState* cpu);
// After the setting changes.
void bluewake_dpad_menus_reload(void);
// Player 1's pad, as the game is about to read it.
void bluewake_dpad_menus_apply(DolPadState* pad);

#ifdef __cplusplus
}
#endif

#endif
