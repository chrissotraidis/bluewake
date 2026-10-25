#ifndef BLUEWAKE_HD_LOOK_H
#define BLUEWAKE_HD_LOOK_H

#include "core/cpu.h"
#include <stdbool.h>

// The Wind Waker HD look (docs/WWHD_LOOK.md): the HD release's changes to how
// the game is drawn, rebuilt on the GameCube game. Every part is off by
// default. BLUEWAKE_HD_LOOK=1 turns on every part that exists except sun
// shadows; a part's own variable (=1 or =0) overrides that.
//
// Distant islands (BLUEWAKE_HD_DISTANT_ISLANDS): each island's far-away model
// (d_a_lod_bg) appears and is unloaded 100,000 units further out, as in HD.
// Cloud shadows (BLUEWAKE_HD_CLOUD_SHADOWS), sun shadows (BLUEWAKE_HD_SUN_SHADOWS)
// and edge light (BLUEWAKE_HD_EDGE_LIGHT) outdoors, from the game's
// camera and sun. Torch light (BLUEWAKE_HD_TORCH_LIGHT) everywhere, from the
// game's own point lights. Ambient occlusion (BLUEWAKE_HD_AO), far blur (BLUEWAKE_HD_FAR_BLUR) and
// bloom (BLUEWAKE_HD_BLOOM): Aurora's post_fx, on the finished scene before
// the HUD.
// Read again by the settings menus.
void bluewake_hd_look_reload(void);
// The softer starting strengths for Aurora's parts; call before Aurora starts.
void bluewake_hd_look_defaults(void);
// Once per retrace.
void bluewake_hd_look_retrace(CPUState* cpu);

extern bool bluewake_hd_distant_islands;

// The room model's culling, widened while sun shadows from the map are shown
// on the high shadow quality (BLUEWAKE_HD_ROOM_CULL=0 or 1 overrides): set when
// the edge service should call bluewake_hd_look_room_clip at clipByBox's entry
// (0x80256CB8), which then culls the room's joints itself and returns.
extern bool bluewake_hd_room_clip;
void bluewake_hd_look_room_clip(CPUState* cpu, u32 address);

#endif
