// SPDX-License-Identifier: GPL-3.0-or-later
// Controller button remapping for the desktop menus (Mac and Windows), the same
// six GameCube buttons, defaults and choices as the iPad's (apple/ios/src/
// controller_settings.mm). Saved as six SDL gamepad button numbers in the order
// A, B, X, Y, Z, Start; empty means the controller's own layout.
#pragma once
#include <SDL3/SDL_gamepad.h>
#include <dolphin/pad.h>
#include <cstdio>
#include <string>

enum { BW_REMAP_BUTTONS = 6, BW_REMAP_CHOICES = 10 };

inline const char* const kBwRemapNames[BW_REMAP_BUTTONS] = {"A", "B", "X", "Y", "Z", "Start"};
inline constexpr PADButton kBwRemapPad[BW_REMAP_BUTTONS] = {
    PAD_BUTTON_A, PAD_BUTTON_B, PAD_BUTTON_X, PAD_BUTTON_Y, PAD_TRIGGER_Z, PAD_BUTTON_START};
// Aurora's standard layout: face buttons by position, right shoulder Z.
inline constexpr unsigned kBwRemapDefault[BW_REMAP_BUTTONS] = {
    SDL_GAMEPAD_BUTTON_SOUTH, SDL_GAMEPAD_BUTTON_EAST, SDL_GAMEPAD_BUTTON_WEST,
    SDL_GAMEPAD_BUTTON_NORTH, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, SDL_GAMEPAD_BUTTON_START};
inline constexpr unsigned kBwRemapChoice[BW_REMAP_CHOICES] = {
    SDL_GAMEPAD_BUTTON_SOUTH, SDL_GAMEPAD_BUTTON_EAST, SDL_GAMEPAD_BUTTON_WEST,
    SDL_GAMEPAD_BUTTON_NORTH, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER,
    SDL_GAMEPAD_BUTTON_LEFT_STICK, SDL_GAMEPAD_BUTTON_RIGHT_STICK, SDL_GAMEPAD_BUTTON_START,
    SDL_GAMEPAD_BUTTON_BACK};
inline const char* const kBwRemapChoiceNames[BW_REMAP_CHOICES] = {
    "Bottom face (A / Cross)", "Right face (B / Circle)", "Left face (X / Square)",
    "Top face (Y / Triangle)", "Left shoulder (LB / L1)", "Right shoulder (RB / R1)",
    "Left stick click", "Right stick click", "Menu / Start", "View / Select"};

struct BwButtonMap {
    unsigned native[BW_REMAP_BUTTONS];
};

inline BwButtonMap bw_button_map_default() {
    BwButtonMap map;
    for (int i = 0; i < BW_REMAP_BUTTONS; i++)
        map.native[i] = kBwRemapDefault[i];
    return map;
}

inline int bw_remap_choice_index(unsigned native) {
    for (int i = 0; i < BW_REMAP_CHOICES; i++)
        if (kBwRemapChoice[i] == native)
            return i;
    return -1;
}

// "" or a malformed value: the default layout (and false).
inline bool bw_button_map_parse(const std::string& text, BwButtonMap* map) {
    *map = bw_button_map_default();
    unsigned v[BW_REMAP_BUTTONS];
    char trailing;
    if (std::sscanf(text.c_str(), "%u,%u,%u,%u,%u,%u%c", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &trailing) != 6)
        return false;
    for (unsigned n : v)
        if (bw_remap_choice_index(n) < 0)
            return false;
    for (int i = 0; i < BW_REMAP_BUTTONS; i++)
        map->native[i] = v[i];
    return true;
}

inline std::string bw_button_map_format(const BwButtonMap& map) {
    for (int i = 0; i < BW_REMAP_BUTTONS; i++)
        if (map.native[i] != kBwRemapDefault[i]) {
            char text[64];
            std::snprintf(text, sizeof text, "%u,%u,%u,%u,%u,%u", map.native[0], map.native[1], map.native[2],
                          map.native[3], map.native[4], map.native[5]);
            return text;
        }
    return "";  // the default layout is stored as nothing
}

// Give GameCube button index the controller button native, swapping with the
// button that had it so no controller button does two things (as on the iPad).
inline void bw_button_map_set(BwButtonMap* map, int index, unsigned native) {
    const unsigned previous = map->native[index];
    for (int i = 0; i < BW_REMAP_BUTTONS; i++)
        if (i != index && map->native[i] == native)
            map->native[i] = previous;
    map->native[index] = native;
}

// Call after PADRestoreDefaultMapping.
inline void bw_apply_button_map(unsigned port, const BwButtonMap& map) {
    for (int i = 0; i < BW_REMAP_BUTTONS; i++)
        PADSetButtonMapping(port, PADButtonMapping{map.native[i], kBwRemapPad[i]});
}

#ifdef IMGUI_VERSION
// The menu rows: one choice per GameCube button and a reset. True when changed.
inline bool bw_button_map_ui(BwButtonMap* map) {
    bool changed = false;
    for (int i = 0; i < BW_REMAP_BUTTONS; i++) {
        int choice = bw_remap_choice_index(map->native[i]);
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 14);
        char label[32];
        std::snprintf(label, sizeof label, "GameCube %s", kBwRemapNames[i]);
        if (ImGui::Combo(label, &choice, kBwRemapChoiceNames, BW_REMAP_CHOICES) && choice >= 0) {
            bw_button_map_set(map, i, kBwRemapChoice[choice]);
            changed = true;
        }
    }
    if (ImGui::SmallButton("Reset buttons")) {
        *map = bw_button_map_default();
        changed = true;
    }
    return changed;
}
#endif
