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
// The right stick's left-and-right and up-and-down for the game's own C-stick,
// which has the camera when Link swims, sails or targets. The direct stick
// camera inverts itself (BLUEWAKE_STICK_CAMERA_INVERT_X/_Y); without this the
// game's camera turned the other way from it in those places. Call after
// PADRestoreDefaultMapping.
inline void bw_apply_camera_axes(unsigned port, bool invert_x, bool invert_y) {
    const PADAxisMapping axes[4] = {
        {{SDL_GAMEPAD_AXIS_RIGHTX, invert_x ? AXIS_SIGN_NEGATIVE : AXIS_SIGN_POSITIVE},
         SDL_GAMEPAD_BUTTON_INVALID, PAD_AXIS_RIGHT_X_POS},
        {{SDL_GAMEPAD_AXIS_RIGHTX, invert_x ? AXIS_SIGN_POSITIVE : AXIS_SIGN_NEGATIVE},
         SDL_GAMEPAD_BUTTON_INVALID, PAD_AXIS_RIGHT_X_NEG},
        {{SDL_GAMEPAD_AXIS_RIGHTY, invert_y ? AXIS_SIGN_POSITIVE : AXIS_SIGN_NEGATIVE},
         SDL_GAMEPAD_BUTTON_INVALID, PAD_AXIS_RIGHT_Y_POS},
        {{SDL_GAMEPAD_AXIS_RIGHTY, invert_y ? AXIS_SIGN_NEGATIVE : AXIS_SIGN_POSITIVE},
         SDL_GAMEPAD_BUTTON_INVALID, PAD_AXIS_RIGHT_Y_NEG},
    };
    for (const PADAxisMapping& axis : axes) PADSetAxisMapping(port, axis);
}
