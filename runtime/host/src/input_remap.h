// SPDX-License-Identifier: GPL-3.0-or-later
// Mouse buttons and keyboard keys for the GameCube buttons, for the desktop
// menus (Mac and Windows). The mouse part is plain C, for mouse_camera.c,
// which presses the chosen buttons while the mouse is the camera; the keyboard
// part and the menu rows are C++.
//
//   BLUEWAKE_MOUSE_BUTTONS=A,-,B,-,-   left, middle, right, back and forward
//                                      side buttons: A B X Y Z L R Start or -
//                                      (nothing); "" is the default, left A
//   BLUEWAKE_KEY_MAP=13,14,24,12,20,8,21,40
//                                      SDL scancodes for A B X Y Z L R Start;
//                                      "" is the default, J K U I Q E R Return
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

enum { BW_MOUSE_BUTTONS = 5, BW_MOUSE_CHOICES = 9 };

// Mouse button i is SDL button i + 1 (SDL_BUTTON_LEFT, _MIDDLE, _RIGHT, _X1, _X2).
static inline const char* bw_mouse_button_name(int i) {
    static const char* const names[BW_MOUSE_BUTTONS] = {"Left button", "Middle button (wheel click)",
                                                        "Right button", "Back side button",
                                                        "Forward side button"};
    return i >= 0 && i < BW_MOUSE_BUTTONS ? names[i] : "";
}
static inline const char* bw_mouse_choice_token(int c) {
    static const char* const tokens[BW_MOUSE_CHOICES] = {"-", "A", "B", "X", "Y", "Z", "L", "R", "Start"};
    return c >= 0 && c < BW_MOUSE_CHOICES ? tokens[c] : "-";
}
// The GameCube pad bits (PAD_BUTTON_A ...): A B X Y, Z, L, R, Start.
static inline unsigned short bw_mouse_choice_pad(int c) {
    static const unsigned short bits[BW_MOUSE_CHOICES] = {0x0000, 0x0100, 0x0200, 0x0400, 0x0800,
                                                          0x0010, 0x0040, 0x0020, 0x1000};
    return c >= 0 && c < BW_MOUSE_CHOICES ? bits[c] : 0;
}

typedef struct {
    int choice[BW_MOUSE_BUTTONS];
} BwMouseMap;

static inline BwMouseMap bw_mouse_map_default(void) {
    BwMouseMap map = {{1, 0, 0, 0, 0}};  // left click is A, as it has been
    return map;
}

static inline bool bw_mouse_map_is_default(const BwMouseMap* map) {
    const BwMouseMap fallback = bw_mouse_map_default();
    return memcmp(map, &fallback, sizeof fallback) == 0;
}

// NULL, "" or a malformed value: the default (and false).
static inline bool bw_mouse_map_parse(const char* text, BwMouseMap* map) {
    *map = bw_mouse_map_default();
    if (text == NULL || text[0] == '\0')
        return false;
    BwMouseMap parsed;
    const char* at = text;
    for (int i = 0; i < BW_MOUSE_BUTTONS; i++) {
        const char* end = strchr(at, ',');
        const size_t length = end != NULL ? (size_t)(end - at) : strlen(at);
        if ((end == NULL) != (i == BW_MOUSE_BUTTONS - 1))
            return false;
        parsed.choice[i] = -1;
        for (int c = 0; c < BW_MOUSE_CHOICES; c++)
            if (strlen(bw_mouse_choice_token(c)) == length && strncmp(at, bw_mouse_choice_token(c), length) == 0)
                parsed.choice[i] = c;
        if (parsed.choice[i] < 0)
            return false;
        at = end != NULL ? end + 1 : at + length;
    }
    *map = parsed;
    return true;
}

// The default is written as nothing.
static inline void bw_mouse_map_format(const BwMouseMap* map, char* out, size_t size) {
    if (size == 0)
        return;
    out[0] = '\0';
    if (bw_mouse_map_is_default(map))
        return;
    snprintf(out, size, "%s,%s,%s,%s,%s", bw_mouse_choice_token(map->choice[0]),
             bw_mouse_choice_token(map->choice[1]), bw_mouse_choice_token(map->choice[2]),
             bw_mouse_choice_token(map->choice[3]), bw_mouse_choice_token(map->choice[4]));
}

#ifdef __cplusplus
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_scancode.h>
#include <dolphin/pad.h>
#include <string>

enum { BW_KEY_BUTTONS = 8 };

inline const char* const kBwKeyNames[BW_KEY_BUTTONS] = {"A", "B", "X", "Y", "Z", "L", "R", "Start"};
inline constexpr PADButton kBwKeyPad[BW_KEY_BUTTONS] = {PAD_BUTTON_A,  PAD_BUTTON_B,  PAD_BUTTON_X,
                                                        PAD_BUTTON_Y,  PAD_TRIGGER_Z, PAD_TRIGGER_L,
                                                        PAD_TRIGGER_R, PAD_BUTTON_START};
// RecompCore's keyboard defaults (GXRuntime/backends/aurora/aurora_input.cpp).
inline constexpr int kBwKeyDefault[BW_KEY_BUTTONS] = {SDL_SCANCODE_J, SDL_SCANCODE_K, SDL_SCANCODE_U,
                                                      SDL_SCANCODE_I, SDL_SCANCODE_Q, SDL_SCANCODE_E,
                                                      SDL_SCANCODE_R, SDL_SCANCODE_RETURN};

struct BwKeyMap {
    int scancode[BW_KEY_BUTTONS];
};

inline BwKeyMap bw_key_map_default() {
    BwKeyMap map;
    for (int i = 0; i < BW_KEY_BUTTONS; i++)
        map.scancode[i] = kBwKeyDefault[i];
    return map;
}

// Keys the game can't have: the menu's and the window's, the sticks' and the
// D-pad's (W A S D, T F G H, the arrows), and jump and sprint (Space, Shift).
inline bool bw_key_allowed(int scancode) {
    if (scancode <= SDL_SCANCODE_UNKNOWN || scancode >= SDL_SCANCODE_COUNT)
        return false;
    if (scancode >= SDL_SCANCODE_F1 && scancode <= SDL_SCANCODE_F12)
        return false;
    switch (scancode) {
    case SDL_SCANCODE_ESCAPE: case SDL_SCANCODE_SPACE: case SDL_SCANCODE_LSHIFT: case SDL_SCANCODE_RSHIFT:
    case SDL_SCANCODE_LALT: case SDL_SCANCODE_RALT: case SDL_SCANCODE_LGUI: case SDL_SCANCODE_RGUI:
    case SDL_SCANCODE_W: case SDL_SCANCODE_A: case SDL_SCANCODE_S: case SDL_SCANCODE_D:
    case SDL_SCANCODE_T: case SDL_SCANCODE_F: case SDL_SCANCODE_G: case SDL_SCANCODE_H:
    case SDL_SCANCODE_UP: case SDL_SCANCODE_DOWN: case SDL_SCANCODE_LEFT: case SDL_SCANCODE_RIGHT:
        return false;
    default:
        return true;
    }
}

// "" or a malformed value: the default (and false).
inline bool bw_key_map_parse(const std::string& text, BwKeyMap* map) {
    *map = bw_key_map_default();
    int v[BW_KEY_BUTTONS];
    char trailing;
    if (std::sscanf(text.c_str(), "%d,%d,%d,%d,%d,%d,%d,%d%c", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6],
                    &v[7], &trailing) != BW_KEY_BUTTONS)
        return false;
    for (int i = 0; i < BW_KEY_BUTTONS; i++) {
        if (!bw_key_allowed(v[i]))
            return false;
        for (int j = 0; j < i; j++)
            if (v[j] == v[i])
                return false;
    }
    for (int i = 0; i < BW_KEY_BUTTONS; i++)
        map->scancode[i] = v[i];
    return true;
}

inline std::string bw_key_map_format(const BwKeyMap& map) {
    for (int i = 0; i < BW_KEY_BUTTONS; i++)
        if (map.scancode[i] != kBwKeyDefault[i]) {
            char text[96];
            std::snprintf(text, sizeof text, "%d,%d,%d,%d,%d,%d,%d,%d", map.scancode[0], map.scancode[1],
                          map.scancode[2], map.scancode[3], map.scancode[4], map.scancode[5], map.scancode[6],
                          map.scancode[7]);
            return text;
        }
    return "";
}

// Give GameCube button index the key, swapping with the button that had it.
inline void bw_key_map_set(BwKeyMap* map, int index, int scancode) {
    const int previous = map->scancode[index];
    for (int i = 0; i < BW_KEY_BUTTONS; i++)
        if (i != index && map->scancode[i] == scancode)
            map->scancode[i] = previous;
    map->scancode[index] = scancode;
}

inline const char* bw_key_name(int scancode) {
    const char* name = SDL_GetScancodeName(static_cast<SDL_Scancode>(scancode));
    return name != nullptr && name[0] != '\0' ? name : "?";
}

// Once the pad's keyboard bindings exist (RecompCore installs the defaults at
// start). L and R are a button and an analog trigger each: both follow.
inline bool bw_apply_key_map(unsigned port, const BwKeyMap& map) {
    u32 count = 0;
    if (PADGetKeyButtonBindings(port, &count) == nullptr)
        return false;
    for (int i = 0; i < BW_KEY_BUTTONS; i++)
        PADSetKeyButtonBinding(port, PADKeyButtonBinding{map.scancode[i], kBwKeyPad[i]});
    PADSetKeyAxisBinding(port, PADKeyAxisBinding{map.scancode[5], PAD_AXIS_TRIGGER_L, 32767});
    PADSetKeyAxisBinding(port, PADKeyAxisBinding{map.scancode[6], PAD_AXIS_TRIGGER_R, 32767});
    return true;
}

#ifdef IMGUI_VERSION
// The menu rows for the keyboard: each GameCube button's key, and a button that
// waits for the next key pressed. True when the map changed.
inline bool bw_key_map_ui(BwKeyMap* map) {
    static int capturing = -1, last_frame = -10, done_frame = -10;
    static bool held[SDL_SCANCODE_COUNT];
    static bool refused;
    const int frame = ImGui::GetFrameCount();
    if (last_frame != frame - 1)
        capturing = -1;  // the menu or this section was closed
    last_frame = frame;
    bool changed = false;
    int count = 0;
    const bool* keys = SDL_GetKeyboardState(&count);
    if (count > SDL_SCANCODE_COUNT)
        count = SDL_SCANCODE_COUNT;
    if (capturing >= 0 && keys != nullptr) {
        for (int key = SDL_SCANCODE_A; key < count; key++) {
            if (keys[key] && !held[key] && capturing >= 0) {
                if (bw_key_allowed(key)) {
                    bw_key_map_set(map, capturing, key);
                    capturing = -1;
                    done_frame = frame;
                    refused = false;
                    changed = true;
                } else {
                    refused = true;
                }
            }
            held[key] = keys[key];
        }
    }
    for (int i = 0; i < BW_KEY_BUTTONS; i++) {
        ImGui::PushID(i);
        ImGui::AlignTextToFramePadding();
        ImGui::Text("GameCube %s", kBwKeyNames[i]);
        ImGui::SameLine(ImGui::GetFontSize() * 8);
        const char* label = capturing == i ? "Press a key..." : bw_key_name(map->scancode[i]);
        if (ImGui::Button(label, ImVec2(ImGui::GetFontSize() * 9, 0)) && frame != done_frame) {
            capturing = i;
            refused = false;
            for (int key = 0; key < SDL_SCANCODE_COUNT; key++)
                held[key] = keys != nullptr && key < count && keys[key];
        }
        ImGui::PopID();
    }
    if (capturing >= 0) {
        ImGui::TextWrapped("%s", refused ? "That key moves Link or the camera, jumps, sprints or opens the menu. "
                                           "Press another one."
                                         : "Press the key for this button.");
        if (ImGui::SmallButton("Cancel"))
            capturing = -1;
        ImGui::SameLine();
    }
    if (ImGui::SmallButton("Reset keys")) {
        *map = bw_key_map_default();
        capturing = -1;
        changed = true;
    }
    return changed;
}

// The menu rows for the mouse: what each button presses while the mouse is the
// camera. True when the map changed.
inline bool bw_mouse_map_ui(BwMouseMap* map) {
    static const char* const choices[BW_MOUSE_CHOICES] = {"Nothing", "A", "B", "X", "Y",
                                                          "Z", "L", "R", "Start"};
    bool changed = false;
    for (int i = 0; i < BW_MOUSE_BUTTONS; i++) {
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 9);
        changed |= ImGui::Combo(bw_mouse_button_name(i), &map->choice[i], choices, BW_MOUSE_CHOICES);
    }
    if (ImGui::SmallButton("Reset mouse buttons")) {
        *map = bw_mouse_map_default();
        changed = true;
    }
    return changed;
}
#endif  // IMGUI_VERSION
#endif  // __cplusplus
