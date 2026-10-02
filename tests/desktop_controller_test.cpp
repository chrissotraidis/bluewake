// SPDX-License-Identifier: GPL-3.0-or-later
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <SDL3/SDL.h>
#include <cassert>
#include <chrono>
#include <filesystem>
#include "input.hpp"
#include "internal.hpp"
#include "controller_face_swap.h"
int main() {
    assert(SDL_InitSubSystem(SDL_INIT_GAMEPAD));
    auto directory = std::filesystem::temp_directory_path() / ("bluewake-pad-test-" + std::to_string(SDL_GetTicksNS()));
    std::filesystem::create_directory(directory);
    std::string path = directory.string();
    aurora::g_config.userPath = path.c_str();
    SDL_VirtualJoystickDesc desc{}; SDL_INIT_INTERFACE(&desc);
    desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
    desc.naxes = SDL_GAMEPAD_AXIS_COUNT; desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
    desc.axis_mask = (1u << SDL_GAMEPAD_AXIS_COUNT) - 1;
    desc.button_mask = (1u << SDL_GAMEPAD_BUTTON_COUNT) - 1;
    desc.name = "BlueWake synthetic controller";
    SDL_JoystickID id = SDL_AttachVirtualJoystick(&desc); assert(id);
    SDL_Joystick* joystick = SDL_OpenJoystick(id); assert(joystick);
    assert(aurora::input::add_controller(id));
    aurora::input::set_player_index(id, 0);
    assert(PADInit()); PADRestoreDefaultMapping(0);
    u32 count;
    PADButtonMapping* defaults = PADGetButtonMappings(0, &count); assert(defaults && count == PAD_BUTTON_COUNT);
    PADButtonMapping original[PAD_BUTTON_COUNT];
    std::copy_n(defaults, PAD_BUTTON_COUNT, original);
    bw_apply_face_swaps(0, true, true);
    auto* swapped = PADGetButtonMappings(0, &count);
    for (unsigned i = 0; i < PAD_BUTTON_COUNT; ++i) {
        uint32_t target = bw_swap_face_buttons(original[i].padButton, true, true);
        bool found = false;
        for (unsigned j = 0; j < PAD_BUTTON_COUNT; ++j)
            found |= swapped[j].padButton == target && swapped[j].nativeButton == original[i].nativeButton;
        assert(found);
    }
    PADRestoreDefaultMapping(0);
    assert(SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_SOUTH, true));
    assert(SDL_SetJoystickVirtualAxis(joystick, SDL_GAMEPAD_AXIS_LEFTX, 24000));
    SDL_UpdateJoysticks(); SDL_UpdateGamepads();
    PADStatus status[PAD_CHANMAX]; PADRead(status);
    assert(status[0].button & PAD_BUTTON_A); assert(status[0].stickX != 0);
    PADBlockInput(true); PADRead(status);
    assert(status[0].button == 0 && status[0].stickX == 0);
    PADBlockInput(false); PADRead(status);
    assert(!(status[0].button & PAD_BUTTON_A)); // closing press is swallowed
    PADRead(status); assert(!(status[0].button & PAD_BUTTON_A));
    assert(SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_SOUTH, false));
    SDL_UpdateJoysticks(); SDL_UpdateGamepads(); PADRead(status);
    assert(SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_SOUTH, true));
    SDL_UpdateJoysticks(); SDL_UpdateGamepads(); PADRead(status);
    assert(status[0].button & PAD_BUTTON_A); // only a new press reaches the game
    aurora::input::remove_controller(id);
    SDL_CloseJoystick(joystick); assert(SDL_DetachVirtualJoystick(id));
    SDL_JoystickID replacement = SDL_AttachVirtualJoystick(&desc); assert(replacement && replacement != id);
    joystick = SDL_OpenJoystick(replacement); assert(joystick);
    assert(aurora::input::add_controller(replacement));
    aurora::input::set_player_index(replacement, 0);
    assert(PADGetIndexForPort(0) == 0); // Same index would miss this replacement.
    assert(bw_controller_connection(0) == replacement && bw_controller_connection(0) != id);
    PADRestoreDefaultMapping(0); bw_apply_face_swaps(0, true, false);
    assert(SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_SOUTH, true));
    SDL_UpdateJoysticks(); SDL_UpdateGamepads(); PADRead(status);
    assert((status[0].button & (PAD_BUTTON_A | PAD_BUTTON_B)) == PAD_BUTTON_B);
    aurora::input::remove_controller(replacement);
    SDL_CloseJoystick(joystick); assert(SDL_DetachVirtualJoystick(replacement));
    SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
    std::filesystem::remove_all(directory);
}
