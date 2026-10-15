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

inline int bw_menu_slider_nudge(); // below: a controller's left and right on a slider

// Where the lit cell of a list sits now, eased towards where it is going so it
// slides there rather than jumping. Kept per control in ImGui's storage; the
// first frame it is drawn, it starts where it belongs.
inline ImVec4 bw_menu_slide(ImGuiID id, const ImVec4& target) {
    ImGuiStorage* storage = ImGui::GetStateStorage();
    const ImGuiID ids[4] = {id + 1, id + 2, id + 3, id + 4};
    const float k = std::min(1.f, ImGui::GetIO().DeltaTime * 18.f);
    const bool seen = storage->GetBool(id, false);
    storage->SetBool(id, true);
    float now[4] = {target.x, target.y, target.z, target.w};
    for (int i = 0; i < 4; ++i) {
        if (seen) {
            const float was = storage->GetFloat(ids[i], now[i]);
            now[i] = std::fabs(now[i] - was) < 0.5f ? now[i] : was + (now[i] - was) * k;
        }
        storage->SetFloat(ids[i], now[i]);
    }
    return ImVec4(now[0], now[1], now[2], now[3]);
}

// A list's choices as one row of cells, every name in its own, the chosen one
// lit; a new choice slides the light across. It works as a slider does: a pad
// lands on the whole row once and moves it with left and right, and the
// pointer clicks a cell or drags along the row. When the names don't fit
// side by side, the cells stack, one choice a line.
inline bool bw_menu_combo(const char* label, int* index, const char* const* items, int count) {
    constexpr int kMost = 16;
    count = std::clamp(count, 0, kMost);
    if (count == 0)
        return false;
    ImGui::PushID(label);
    ImGui::TextWrapped("%s", label);
    const ImGuiStyle& style = ImGui::GetStyle();
    const float width = std::max(1.f, ImGui::GetContentRegionAvail().x);
    const float pad = style.FramePadding.x, line = ImGui::GetFrameHeight();
    // Cells as wide as each other when every name fits that way, else as wide
    // as each name needs with the room left shared out, else stacked.
    float need[kMost], total = 0.f, widest = 0.f;
    for (int i = 0; i < count; ++i) {
        need[i] = ImGui::CalcTextSize(items[i]).x + pad * 2.f;
        total += need[i];
        widest = std::max(widest, need[i]);
    }
    const bool stacked = total > width;
    float cell[kMost];
    for (int i = 0; i < count; ++i)
        cell[i] = stacked ? width
                  : widest * static_cast<float>(count) <= width ? width / static_cast<float>(count)
                                                                : need[i] + (width - total) / static_cast<float>(count);
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const auto cell_rect = [&](int i, ImVec2* a, ImVec2* b) {
        float x = origin.x;
        for (int j = 0; j < i && !stacked; ++j)
            x += cell[j];
        const float y = origin.y + (stacked ? line * static_cast<float>(i) : 0.f);
        *a = ImVec2(x, y);
        *b = ImVec2(x + cell[i], y + line);
    };
    const ImVec2 size(width, stacked ? line * static_cast<float>(count) : line);
    ImGui::InvisibleButton("##choices", size, ImGuiButtonFlags_EnableNav);
    // The pointer itself over it: ImGui also counts a pad's cursor on the row
    // as hovering, and then the cell under a pointer resting elsewhere lit up.
    const bool hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_NoNavOverride);
    // The cell under the pointer: picked on a click and followed while dragged.
    int under = -1;
    if (hovered || (ImGui::IsItemActive() && ImGui::GetCurrentContext()->ActiveIdSource == ImGuiInputSource_Mouse)) {
        const ImVec2 mouse = ImGui::GetIO().MousePos;
        for (int i = 0; i < count; ++i) {
            ImVec2 a, b;
            cell_rect(i, &a, &b);
            if (stacked ? mouse.y < b.y || i == count - 1 : mouse.x < b.x || i == count - 1) {
                under = i;
                break;
            }
        }
    }
    bool changed = false;
    const auto choose = [&](int i) {
        i = std::clamp(i, 0, count - 1);
        changed = changed || i != *index;
        *index = i;
    };
    const ImGuiContext& context = *ImGui::GetCurrentContext();
    if (ImGui::IsItemActive() && context.ActiveIdSource == ImGuiInputSource_Mouse && under >= 0)
        choose(under);
    if (const int step = bw_menu_slider_nudge())
        choose(*index + step);
    const int at = std::clamp(*index, 0, count - 1);

    ImDrawList* paint = ImGui::GetWindowDrawList();
    const float round = style.FrameRounding;
    const ImVec2 end(origin.x + size.x, origin.y + size.y);
    // The cursor on it lights the whole row, as the pointer lights a cell.
    const bool focused = ImGui::IsItemFocused();
    paint->AddRectFilled(origin, end, ImGui::GetColorU32(focused ? ImGuiCol_HeaderHovered : ImGuiCol_FrameBg),
                         round);
    if (hovered && under >= 0 && under != at && !ImGui::IsItemActive()) {
        ImVec2 a, b;
        cell_rect(under, &a, &b);
        paint->AddRectFilled(ImVec2(a.x + 2.f, a.y + 2.f), ImVec2(b.x - 2.f, b.y - 2.f),
                             ImGui::GetColorU32(ImGuiCol_HeaderHovered), round * 0.75f);
    }
    {
        ImVec2 a, b;
        cell_rect(at, &a, &b);
        const ImVec4 lit = bw_menu_slide(ImGui::GetID("##slide"), ImVec4(a.x, a.y, b.x, b.y));
        const ImVec2 la(lit.x + 2.f, lit.y + 2.f), lb(lit.z - 2.f, lit.w - 2.f);
        paint->AddRectFilled(la, lb, ImGui::GetColorU32(ImGuiCol_FrameBgActive), round * 0.75f);
        paint->AddRect(la, lb, ImGui::GetColorU32(ImGuiCol_SliderGrab), round * 0.75f, 0, 1.5f);
    }
    for (int i = 0; i < count; ++i) {
        ImVec2 a, b;
        cell_rect(i, &a, &b);
        const ImVec2 text = ImGui::CalcTextSize(items[i]);
        const float x = stacked ? a.x + pad : a.x + std::max(pad, (b.x - a.x - text.x) * 0.5f);
        const ImU32 ink = ImGui::GetColorU32(i == at || i == under ? ImGuiCol_Text : ImGuiCol_TextDisabled);
        paint->PushClipRect(a, b, true);
        paint->AddText(ImVec2(x, a.y + (line - text.y) * 0.5f), ink, items[i]);
        paint->PopClipRect();
    }
    paint->AddRect(origin, end, ImGui::GetColorU32(ImGuiCol_Border), round, 0, 1.f);
    ImGui::PopID();
    return changed;
}

// A place on the screen, chosen on a small picture of one: a triangle in each
// place it can go (`spots`, as shares of the screen, in their order), the
// chosen one lit. It works as a slider does: a pad lands on the picture once
// and steps through the places in order with left and right, and the pointer
// clicks the place nearest it.
inline bool bw_menu_spot_chooser(const char* label, int* index, const char* const* names, const ImVec2* spots,
                                 int count) {
    if (count <= 0)
        return false;
    ImGui::PushID(label);
    const int was = std::clamp(*index, 0, count - 1);
    ImGui::TextWrapped("%s: %s", label, names[was]);
    const float width = std::min(ImGui::GetContentRegionAvail().x, ImGui::GetFontSize() * 11.f);
    const ImVec2 size(std::max(1.f, width), std::max(1.f, width * 9.f / 16.f));
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##screen", size, ImGuiButtonFlags_EnableNav);
    // The pointer itself over it, as for a list.
    const bool hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_NoNavOverride);
    const float side = size.y * 0.24f, inset = side * 0.3f;
    // Where each spot's triangle sits: in its corner, or against its edge's middle.
    const auto anchor = [&](int i) {
        return ImVec2(origin.x + inset + (size.x - inset * 2.f) * spots[i].x,
                      origin.y + inset + (size.y - inset * 2.f) * spots[i].y);
    };
    int under = -1;
    if (hovered || (ImGui::IsItemActive() && ImGui::GetCurrentContext()->ActiveIdSource == ImGuiInputSource_Mouse)) {
        const ImVec2 mouse = ImGui::GetIO().MousePos;
        float best = 0.f;
        for (int i = 0; i < count; ++i) {
            const ImVec2 p = anchor(i);
            const float d = (p.x - mouse.x) * (p.x - mouse.x) + (p.y - mouse.y) * (p.y - mouse.y);
            if (under < 0 || d < best) {
                under = i;
                best = d;
            }
        }
    }
    bool changed = false;
    const auto choose = [&](int i) {
        i = std::clamp(i, 0, count - 1);
        changed = changed || i != *index;
        *index = i;
    };
    const ImGuiContext& context = *ImGui::GetCurrentContext();
    if (ImGui::IsItemActive() && context.ActiveIdSource == ImGuiInputSource_Mouse && under >= 0)
        choose(under);
    if (const int step = bw_menu_slider_nudge())
        choose(*index + step);
    const int at = std::clamp(*index, 0, count - 1);

    ImDrawList* paint = ImGui::GetWindowDrawList();
    const ImVec2 end(origin.x + size.x, origin.y + size.y);
    const float round = ImGui::GetStyle().FrameRounding;
    // The cursor on it lights the whole picture, as on a list.
    const bool focused = ImGui::IsItemFocused();
    paint->AddRectFilled(origin, end, ImGui::GetColorU32(focused ? ImGuiCol_HeaderHovered : ImGuiCol_FrameBg),
                         round);
    paint->AddRect(origin, end, ImGui::GetColorU32(ImGuiCol_Border), round, 0, 1.f);
    for (int i = 0; i < count; ++i) {
        const ImVec2 p = anchor(i);
        // Into its corner (a right angle there), or pointing at its edge.
        const float sx = spots[i].x < 0.25f ? 1.f : spots[i].x > 0.75f ? -1.f : 0.f;
        const float sy = spots[i].y < 0.5f ? 1.f : -1.f;
        ImVec2 t[3];
        if (sx != 0.f) {
            t[0] = p;
            t[1] = ImVec2(p.x + sx * side, p.y);
            t[2] = ImVec2(p.x, p.y + sy * side);
        } else {
            t[0] = p;
            t[1] = ImVec2(p.x - side * 0.6f, p.y + sy * side * 0.8f);
            t[2] = ImVec2(p.x + side * 0.6f, p.y + sy * side * 0.8f);
        }
        if (i == at)
            paint->AddTriangleFilled(t[0], t[1], t[2], ImGui::GetColorU32(ImGuiCol_SliderGrab));
        else if (i == under)
            paint->AddTriangleFilled(t[0], t[1], t[2], ImGui::GetColorU32(ImGuiCol_Text, 0.45f));
        else
            paint->AddTriangle(t[0], t[1], t[2], ImGui::GetColorU32(ImGuiCol_TextDisabled, 0.6f), 1.f);
    }
    ImGui::PopID();
    return changed;
}

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
