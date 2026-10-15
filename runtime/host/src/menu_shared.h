// SPDX-License-Identifier: GPL-3.0-or-later
// What the settings menus share, so they look and work the same on every
// desktop build: the Mac and Linux menu (settings_menu.cpp) and the Windows one
// (windows/src/win_settings.cpp). Widgets that wrap in a narrow menu, and a
// controller in them.
#pragma once
#include <imgui.h>
#include <algorithm>
#include <imgui_internal.h> // ClearActiveID, ClosePopupsExceptModals, CalcTypematicRepeatAmount
#include <cmath>
#include <utility>
#include <string>
#include "button_remap.h"

// A dropdown with its label on the right, or above it when the window is too
// narrow for both.
inline bool bw_menu_combo(const char* label, int* index, const char* const* items, int count) {
    const float avail = ImGui::GetContentRegionAvail().x;
    if (ImGui::CalcTextSize(label).x + ImGui::GetStyle().ItemInnerSpacing.x <= avail * 0.5f) {
        ImGui::SetNextItemWidth(std::max(80.f, avail * 0.48f));
        return ImGui::Combo(label, index, items, count);
    }
    ImGui::PushID(label);
    ImGui::TextWrapped("%s", label);
    ImGui::SetNextItemWidth(-1.f);
    const bool changed = ImGui::Combo("##value", index, items, count);
    ImGui::PopID();
    return changed;
}

inline int bw_menu_slider_nudge();  // defined below, with the rest of the controller's keys

// A slider with its name above it and the bar the full width, which a
// controller nudges left and right a twentieth of its range at a time.
inline bool bw_menu_slider(const char* label, float* value, float low, float high, const char* format) {
    ImGui::PushID(label);
    ImGui::TextWrapped("%s", label);
    ImGui::SetNextItemWidth(-1.f);
    bool changed = ImGui::SliderFloat("##value", value, low, high, format);
    if (const int step = bw_menu_slider_nudge()) {
        *value = std::clamp(*value + static_cast<float>(step) * (high - low) / 20.f, low, high);
        changed = true;
    }
    ImGui::PopID();
    return changed;
}

// A checkbox whose label wraps when the window is too narrow for it; clicking
// the wrapped label toggles it too.
inline bool bw_menu_checkbox(const char* label, bool* value) {
    const ImGuiStyle& style = ImGui::GetStyle();
    if (ImGui::GetFrameHeight() + style.ItemInnerSpacing.x + ImGui::CalcTextSize(label).x <=
        ImGui::GetContentRegionAvail().x)
        return ImGui::Checkbox(label, value);
    ImGui::PushID(label);
    bool changed = ImGui::Checkbox("##box", value);
    ImGui::SameLine(0.f, style.ItemInnerSpacing.x);
    ImGui::TextWrapped("%s", label);
    if (ImGui::IsItemClicked()) {
        *value = !*value;
        changed = true;
    }
    ImGui::PopID();
    return changed;
}

// --- a controller in the menu ----------------------------------------------
//
// ImGui's own controller navigation is off in the menus (NavEnableGamepad):
// it scrolls the window with the left stick, which nothing outside ImGui can
// switch off, and it chooses and cancels with fixed face buttons. Instead
// bw_menu_controller reads the controller from SDL once a frame and hands ImGui
// keys: the D-pad and the left stick are the arrow keys, held while they're
// held so ImGui repeats them as it does a held key, and the game's A is Space,
// which chooses. The game's B, the bumpers and the right stick come back to
// the menu, which acts on them itself.

struct BwMenuPad {
    int tab_step = 0;   // -1 for LB, 1 for RB, pressed this frame
    bool back = false;  // the game's B, pressed this frame
    float scroll = 0.f; // the right stick, -1 (up) to 1 (down); 0 inside its dead zone
};

// Call it once a frame while the menu is open, before its window. The game's A
// and B are where the player put them: a custom layout (Controls › Controller
// buttons; "" is the controller's own), else Swap A and B, as the game reads them.
inline BwMenuPad bw_menu_controller(const std::string& button_map, bool swap_ab) {
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableGamepad;
    BwButtonMap map;
    const bool custom = bw_button_map_parse(button_map, &map);
    unsigned a = map.native[0], b = map.native[1];
    if (!custom && swap_ab)
        std::swap(a, b);
    bool held[SDL_GAMEPAD_BUTTON_COUNT] = {};
    float lx = 0.f, ly = 0.f, ry = 0.f;
    int count = 0;
    SDL_JoystickID* ids = SDL_GetGamepads(&count);
    for (int i = 0; ids != nullptr && i < count; ++i) {
        SDL_Gamepad* pad = SDL_GetGamepadFromID(ids[i]);
        if (pad == nullptr)
            continue;
        for (int k = 0; k < SDL_GAMEPAD_BUTTON_COUNT; ++k)
            held[k] = held[k] || SDL_GetGamepadButton(pad, static_cast<SDL_GamepadButton>(k));
        const auto stronger = [pad](float& to, SDL_GamepadAxis axis) {
            const float v = SDL_GetGamepadAxis(pad, axis) / 32767.f;
            if (std::fabs(v) > std::fabs(to))
                to = v;
        };
        stronger(lx, SDL_GAMEPAD_AXIS_LEFTX);
        stronger(ly, SDL_GAMEPAD_AXIS_LEFTY);
        stronger(ry, SDL_GAMEPAD_AXIS_RIGHTY);
    }
    SDL_free(ids);
    const auto button = [&held](unsigned k) { return k < SDL_GAMEPAD_BUTTON_COUNT && held[k]; };
    // The stick is one direction at a time, its stronger axis, past half way.
    const bool stick = std::fabs(lx) >= 0.5f || std::fabs(ly) >= 0.5f;
    const bool across = std::fabs(lx) > std::fabs(ly);
    // Presses are a physical button going down, so a button already held when
    // the menu opens, or when the player remaps it in Controls (picking the
    // game's B with the button that then becomes B), does nothing until it is
    // pressed again.
    static bool was_held[SDL_GAMEPAD_BUTTON_COUNT];
    static int last_frame = -10;
    const bool reopened = ImGui::GetFrameCount() != last_frame + 1;
    last_frame = ImGui::GetFrameCount();
    const auto pressed = [&](unsigned k) {
        return !reopened && k < SDL_GAMEPAD_BUTTON_COUNT && held[k] && !was_held[k];
    };
    // The keys ImGui gets: the arrows while the D-pad or the stick is held, and
    // Space from a press of the game's A until it's let go.
    static const ImGuiKey kKey[] = {ImGuiKey_LeftArrow, ImGuiKey_RightArrow, ImGuiKey_UpArrow,
                                    ImGuiKey_DownArrow, ImGuiKey_Space};
    static bool key_down[5];
    const bool now[5] = {
        button(SDL_GAMEPAD_BUTTON_DPAD_LEFT) || (stick && across && lx < 0.f),
        button(SDL_GAMEPAD_BUTTON_DPAD_RIGHT) || (stick && across && lx > 0.f),
        button(SDL_GAMEPAD_BUTTON_DPAD_UP) || (stick && !across && ly < 0.f),
        button(SDL_GAMEPAD_BUTTON_DPAD_DOWN) || (stick && !across && ly > 0.f),
        button(a) && (key_down[4] || pressed(a)),
    };
    // Opened again (or the first time): a direction already held counts once
    // it's let go and pushed again, as A does.
    static bool wait_release[4];
    for (int i = 0; i < 5; ++i) {
        if (i < 4) {
            if (reopened)
                wait_release[i] = now[i];
            else if (!now[i])
                wait_release[i] = false;
        }
        const bool down = !reopened && now[i] && !(i < 4 && wait_release[i]);
        if (down != key_down[i])
            io.AddKeyEvent(kKey[i], down);
        key_down[i] = down;
    }
    BwMenuPad out;
    out.back = pressed(b);
    if (pressed(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER))
        out.tab_step = -1;
    else if (pressed(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER))
        out.tab_step = 1;
    for (int k = 0; k < SDL_GAMEPAD_BUTTON_COUNT; ++k)
        was_held[k] = held[k];
    if (std::fabs(ry) > 0.2f)
        out.scroll = ry;
    return out;
}

// A selected slider moves with left and right (the arrow keys, which the
// D-pad and the left stick hold: bw_menu_controller) without A to grab it
// first: -1, 1, or 0 for no press, repeating while held. Call it right after
// the slider; it fills its row, so left and right have nothing else to do there.
inline int bw_menu_slider_nudge() {
    if (!ImGui::IsItemFocused() || ImGui::IsItemActive())
        return 0;
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, true))
        return -1;
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, true))
        return 1;
    return 0;
}

// The right stick (BwMenuPad::scroll) scrolls the current window, a tab's body:
// the game isn't listening while the menu is open.
inline void bw_menu_stick_scroll(float scroll) {
    if (scroll != 0.f)
        ImGui::SetScrollY(ImGui::GetScrollY() + scroll * 900.f * ImGui::GetIO().DeltaTime);
}

// The game's B (BwMenuPad::back) ends an edit or closes an open dropdown, and
// otherwise closes the menu. Call it inside the menu's window; true: close.
inline bool bw_menu_back(bool pressed) {
    if (!pressed)
        return false;
    if (ImGui::IsAnyItemActive()) {
        ImGui::ClearActiveID();
        return false;
    }
    if (ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId)) {
        ImGui::ClosePopupsExceptModals();
        return false;
    }
    return true;
}

// The footer both menus end with: the hint, the next-launch note when there is
// one, and the buttons, on one row or, when narrow, two.
enum BwMenuButton { BW_MENU_NONE, BW_MENU_RESUME, BW_MENU_SAVE_STATE, BW_MENU_LOAD_STATE, BW_MENU_QUIT };
inline const char* const kBwMenuButtons[] = {"Resume", "Save state (F5)", "Load latest state (F8)", "Quit the game"};

// The footer's height, for the tab body above it (BeginChild's -height).
inline float bw_menu_footer_height(bool restart_note) {
    const ImGuiStyle& style = ImGui::GetStyle();
    float buttons = -style.ItemSpacing.x;
    for (const char* label : kBwMenuButtons)
        buttons += ImGui::CalcTextSize(label).x + style.FramePadding.x * 2.f + style.ItemSpacing.x;
    const int rows = buttons > ImGui::GetContentRegionAvail().x ? 2 : 1;
    return style.ItemSpacing.y * 2.f + ImGui::GetTextLineHeightWithSpacing() +
           (restart_note ? ImGui::GetFrameHeightWithSpacing() : 0.f) + ImGui::GetFrameHeightWithSpacing() * rows;
}

// The hint and the next-launch note. The caller puts its Restart now button,
// if it has one, on the same line right after it (ImGui::SameLine()).
inline void bw_menu_footer_note(bool restart_note) {
    ImGui::Separator();
    ImGui::TextDisabled("A chooses, B closes, LB and RB change tabs, the right stick scrolls.");
    if (restart_note) {
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(ImVec4(1.f, 0.8f, 0.3f, 1.f), "Some changes take effect at the next launch.");
    }
}

// The buttons; returns the one pressed. Resume's tooltip names where the
// preferences are saved, when given.
inline BwMenuButton bw_menu_footer_buttons(const char* settings_path) {
    BwMenuButton pressed = BW_MENU_NONE;
    for (int i = 0; i < 4; ++i) {
        if (i > 0) {
            const float width = ImGui::CalcTextSize(kBwMenuButtons[i]).x + ImGui::GetStyle().FramePadding.x * 2.f;
            const float right = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;
            if (right - ImGui::GetItemRectMax().x > width + ImGui::GetStyle().ItemSpacing.x)
                ImGui::SameLine();
        }
        if (ImGui::Button(kBwMenuButtons[i]))
            pressed = static_cast<BwMenuButton>(i + 1);
        if (i == 0 && settings_path != nullptr && settings_path[0] != '\0' && ImGui::IsItemHovered())
            ImGui::SetTooltip("Preferences are saved when you resume:\n%s", settings_path);
    }
    return pressed;
}
