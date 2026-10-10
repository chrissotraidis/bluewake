// SPDX-License-Identifier: GPL-3.0-or-later
// Where the frame rate counter goes, for the desktop menus (Mac and Windows):
// the choices as the menus show them and as the settings and
// DOL_AURORA_FPS_POSITION spell them, in AuroraFpsOverlayPosition's order, so a
// choice is its position (aurora_set_fps_overlay_position).
#pragma once
#include <aurora/aurora.h>
#include <cstring>

enum { BW_FPS_POSITIONS = 5 };

inline const char* const kBwFpsPositionNames[BW_FPS_POSITIONS] = {"Top Center", "Top Left", "Top Right",
                                                                  "Bottom Left", "Bottom Right"};
inline const char* const kBwFpsPositionValues[BW_FPS_POSITIONS] = {"top-center", "top-left", "top-right",
                                                                   "bottom-left", "bottom-right"};
static_assert(FPS_OVERLAY_TOP_CENTER == 0 && FPS_OVERLAY_TOP_LEFT == 1 && FPS_OVERLAY_TOP_RIGHT == 2 &&
                  FPS_OVERLAY_BOTTOM_LEFT == 3 && FPS_OVERLAY_BOTTOM_RIGHT == BW_FPS_POSITIONS - 1,
              "the choices follow AuroraFpsOverlayPosition");

// A saved or DOL_AURORA_FPS_POSITION value as its choice: the top center unless
// it names a corner, as Aurora reads the variable.
inline int bw_fps_position(const char* value) {
    for (int i = 0; value != nullptr && i < BW_FPS_POSITIONS; i++)
        if (std::strcmp(value, kBwFpsPositionValues[i]) == 0)
            return i;
    return FPS_OVERLAY_TOP_CENTER;
}
