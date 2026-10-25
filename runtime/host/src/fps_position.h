// SPDX-License-Identifier: GPL-3.0-or-later
// Where the frame rate counter goes, for the desktop menus (Mac and Windows):
// the choices as the menus show them and as the settings and
// DOL_AURORA_FPS_POSITION spell them, in AuroraFpsOverlayPosition's order, so a
// choice is its position (aurora_set_fps_overlay_position).
#pragma once
#include <aurora/aurora.h>
#include <cstring>

enum { BW_FPS_POSITIONS = 5 };

inline const char* const kBwFpsPositionNames[BW_FPS_POSITIONS] = {"Top center", "Top left", "Top right",
                                                                  "Bottom left", "Bottom right"};
inline const char* const kBwFpsPositionValues[BW_FPS_POSITIONS] = {"top-center", "top-left", "top-right",
                                                                   "bottom-left", "bottom-right"};
static_assert(FPS_OVERLAY_TOP_CENTER == 0 && FPS_OVERLAY_TOP_LEFT == 1 && FPS_OVERLAY_TOP_RIGHT == 2 &&
                  FPS_OVERLAY_BOTTOM_LEFT == 3 && FPS_OVERLAY_BOTTOM_RIGHT == BW_FPS_POSITIONS - 1,
              "the choices follow AuroraFpsOverlayPosition");

// The places in the menu's order: from the bottom left, clockwise, and where
// each is on the screen (a share of its width and height), for the picture
// the menu chooses it on.
inline const int kBwFpsPositionOrder[BW_FPS_POSITIONS] = {FPS_OVERLAY_BOTTOM_LEFT, FPS_OVERLAY_TOP_LEFT,
                                                          FPS_OVERLAY_TOP_CENTER, FPS_OVERLAY_TOP_RIGHT,
                                                          FPS_OVERLAY_BOTTOM_RIGHT};
inline const float kBwFpsPositionSpot[BW_FPS_POSITIONS][2] = {{0.f, 1.f}, {0.f, 0.f}, {0.5f, 0.f}, {1.f, 0.f},
                                                              {1.f, 1.f}};

// A saved or DOL_AURORA_FPS_POSITION value as its choice: the top center unless
// it names a corner, as Aurora reads the variable.
inline int bw_fps_position(const char* value) {
    for (int i = 0; value != nullptr && i < BW_FPS_POSITIONS; i++)
        if (std::strcmp(value, kBwFpsPositionValues[i]) == 0)
            return i;
    return FPS_OVERLAY_TOP_CENTER;
}
