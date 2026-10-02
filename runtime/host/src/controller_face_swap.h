// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "face_button_swap.h"
#include <dolphin/pad.h>
#include <array>
#include <SDL3/SDL.h>
// SDL's attachment ID changes even when a replacement pad reuses array index 0.
inline SDL_JoystickID bw_controller_connection(unsigned port) {
    int index = PADGetIndexForPort(port);
    if (index < 0) return 0;
    SDL_Gamepad* pad = PADGetSDLGamepadForIndex(static_cast<unsigned>(index));
    return pad ? SDL_GetGamepadID(pad) : 0;
}
// Call after PADRestoreDefaultMapping. Copy first: setting by target while
// iterating the borrowed map could otherwise overwrite a source mapping.
inline void bw_apply_face_swaps(unsigned port, bool ab, bool xy) {
    u32 count = 0;
    PADButtonMapping* defaults = PADGetButtonMappings(port, &count);
    if (!defaults || count != PAD_BUTTON_COUNT) return;
    std::array<PADButtonMapping, PAD_BUTTON_COUNT> mapping;
    for (u32 i = 0; i < count; ++i) {
        mapping[i] = defaults[i];
        mapping[i].padButton = static_cast<PADButton>(bw_swap_face_buttons(mapping[i].padButton, ab, xy));
    }
    for (const auto& button : mapping) PADSetButtonMapping(port, button);
}
