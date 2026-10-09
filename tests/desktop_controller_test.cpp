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
#include "controller_ports.h"
#include "button_remap.h"
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
    // Button remap (button_remap.h): the saved text, swap on conflict, and a press.
    BwButtonMap map;
    assert(!bw_button_map_parse("", &map) && bw_button_map_format(map).empty());
    assert(!bw_button_map_parse("1,2,3", &map) && !bw_button_map_parse("0,1,2,3,10,6,9", &map));
    assert(!bw_button_map_parse("0,1,2,3,10,5", &map));  // the Guide button is not a choice
    bw_button_map_set(&map, 0, SDL_GAMEPAD_BUTTON_EAST);  // A takes B's button; B gets A's
    assert(map.native[0] == SDL_GAMEPAD_BUTTON_EAST && map.native[1] == SDL_GAMEPAD_BUTTON_SOUTH);
    bw_button_map_set(&map, 4, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);  // Z on the left shoulder
    BwButtonMap reread;
    assert(bw_button_map_parse(bw_button_map_format(map), &reread));
    for (int i = 0; i < BW_REMAP_BUTTONS; ++i) assert(reread.native[i] == map.native[i]);
    PADStatus status[PAD_CHANMAX];
    PADRestoreDefaultMapping(0); bw_apply_button_map(0, map);
    assert(SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_SOUTH, true));
    assert(SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, true));
    SDL_UpdateJoysticks(); SDL_UpdateGamepads(); PADRead(status);
    assert((status[0].button & (PAD_BUTTON_A | PAD_BUTTON_B | PAD_TRIGGER_Z)) == (PAD_BUTTON_B | PAD_TRIGGER_Z));
    assert(SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_SOUTH, false));
    assert(SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, false));
    SDL_UpdateJoysticks(); SDL_UpdateGamepads(); PADRead(status);
    PADRestoreDefaultMapping(0);
    assert(SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_SOUTH, true));
    assert(SDL_SetJoystickVirtualAxis(joystick, SDL_GAMEPAD_AXIS_LEFTX, 24000));
    SDL_UpdateJoysticks(); SDL_UpdateGamepads();
    PADRead(status);
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

    // #61: a controller SDL doesn't know until gamecontrollerdb.txt is loaded. Detected as a
    // plain joystick it gets no player slot; the mapping then makes it a gamepad, still without
    // one, so Aurora opened it but never read it.
    SDL_VirtualJoystickDesc generic = desc;
    generic.type = SDL_JOYSTICK_TYPE_UNKNOWN;
    generic.name = "BlueWake synthetic generic pad";
    SDL_JoystickID late = SDL_AttachVirtualJoystick(&generic); assert(late);
    assert(!SDL_IsGamepad(late));
    char guid[64];
    SDL_GUIDToString(SDL_GetJoystickGUIDForID(late), guid, sizeof guid);
    const std::string line = std::string(guid) + ",BlueWake synthetic generic pad,a:b0,b:b1,x:b2,y:b3,"
                             "start:b6,leftx:a0,lefty:a1,rightx:a2,righty:a3";
    assert(SDL_AddGamepadMapping(line.c_str()) >= 0);
    assert(SDL_IsGamepad(late));
    bw_log_gamepad_mapping(late);                           // names the line above in the log
    joystick = SDL_OpenJoystick(late); assert(joystick);
    assert(aurora::input::add_controller(late) == late);   // what Aurora does on SDL_EVENT_GAMEPAD_ADDED
    assert(SDL_GetGamepadPlayerIndex(SDL_GetGamepadFromID(late)) < 0);
    assert(PADGetIndexForPort(0) < 0);                     // the bug: nothing reads it
    assert(bw_claim_player_one());
    assert(bw_controller_connection(0) == late);
    PADRestoreDefaultMapping(0);
    assert(SDL_SetJoystickVirtualButton(joystick, 0, true));
    SDL_UpdateJoysticks(); SDL_UpdateGamepads(); PADRead(status);
    assert(status[0].button & PAD_BUTTON_A);
    assert(SDL_SetJoystickVirtualButton(joystick, 0, false));
    SDL_UpdateJoysticks(); SDL_UpdateGamepads(); PADRead(status);

    // Player 1 is never taken from the controller that has it.
    SDL_JoystickID second = SDL_AttachVirtualJoystick(&desc); assert(second);
    SDL_Joystick* second_joystick = SDL_OpenJoystick(second); assert(second_joystick);
    assert(aurora::input::add_controller(second) == second);
    aurora::input::set_player_index(second, 1);
    assert(!bw_claim_player_one() && bw_controller_connection(0) == late);

    // A controller left as player 2 when player 1's is unplugged becomes player 1.
    aurora::input::remove_controller(late);
    SDL_CloseJoystick(joystick); assert(SDL_DetachVirtualJoystick(late));
    assert(PADGetIndexForPort(0) < 0);
    assert(bw_claim_player_one() && bw_controller_connection(0) == second);
    assert(!bw_claim_player_one());
    aurora::input::remove_controller(second);
    SDL_CloseJoystick(second_joystick); assert(SDL_DetachVirtualJoystick(second));
    assert(!bw_claim_player_one());                        // nothing connected: nothing to claim
    bw_handoff_connected_controllers();                    // nor anything to hand over

    // #138: a controller connected at launch. Aurora adds it while it starts, before the
    // event observer exists, so no SDL_EVENT_GAMEPAD_ADDED reaches the handoff: it plays
    // as player 1 but keeps Aurora's dead zone until the launch handoff runs.
    SDL_JoystickID at_launch = SDL_AttachVirtualJoystick(&desc); assert(at_launch);
    joystick = SDL_OpenJoystick(at_launch); assert(joystick);
    assert(aurora::input::add_controller(at_launch) == at_launch);
    assert(PADGetDeadZones(0) != nullptr && PADGetDeadZones(0)->useDeadzones);   // the bug
    bw_handoff_connected_controllers();
    assert(bw_controller_connection(0) == at_launch);
    assert(PADGetDeadZones(0) != nullptr && !PADGetDeadZones(0)->useDeadzones);
    aurora::input::remove_controller(at_launch);
    SDL_CloseJoystick(joystick); assert(SDL_DetachVirtualJoystick(at_launch));

    // #138: the stick's scale. Aurora's cutoff dropped everything below 8000 of 32767 and then
    // jumped to 31; with the game's own dead zone, the full travel is a GameCube stick's 100.
    SDL_JoystickID stick = SDL_AttachVirtualJoystick(&desc); assert(stick);
    joystick = SDL_OpenJoystick(stick); assert(joystick);
    assert(aurora::input::add_controller(stick) == stick);
    aurora::input::set_player_index(stick, 0);
    auto left = [&](Sint16 x, Sint16 y) {
        assert(SDL_SetJoystickVirtualAxis(joystick, SDL_GAMEPAD_AXIS_LEFTX, x));
        assert(SDL_SetJoystickVirtualAxis(joystick, SDL_GAMEPAD_AXIS_LEFTY, y));
        SDL_UpdateJoysticks(); SDL_UpdateGamepads(); PADRead(status);
    };
    left(7000, 0); assert(status[0].stickX == 0);    // the old cutoff
    left(8001, 0); assert(status[0].stickX == 31);   // then straight to 31
    bw_game_dead_zone(0);
    assert(!PADGetDeadZones(0)->useDeadzones);
    left(7000, 0); assert(status[0].stickX == 21);
    left(32767, 0); assert(status[0].stickX == 100);
    left(-32768, 0); assert(status[0].stickX == -100);
    left(0, -32768); assert(status[0].stickX == 0 && status[0].stickY == 100);   // SDL's up is negative
    left(0, 32767); assert(status[0].stickY == -100);
    left(16384, 0); assert(status[0].stickX == 50);
    assert(SDL_SetJoystickVirtualAxis(joystick, SDL_GAMEPAD_AXIS_RIGHTX, -16384));
    SDL_UpdateJoysticks(); SDL_UpdateGamepads(); PADRead(status);
    assert(status[0].substickX == -50);
    bw_game_dead_zone(0);                              // once is enough; a second call changes nothing
    aurora::input::remove_controller(stick);
    SDL_CloseJoystick(joystick); assert(SDL_DetachVirtualJoystick(stick));
    SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
    std::filesystem::remove_all(directory);
}
