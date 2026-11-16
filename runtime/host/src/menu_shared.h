// SPDX-License-Identifier: GPL-3.0-or-later
// What the settings menus share, so they look and work the same on every
// desktop build: the Mac and Linux menu (settings_menu.cpp) and the Windows one
// (windows/src/win_settings.cpp). Widgets that wrap in a narrow menu, and a
// controller in them.
#pragma once
#include <imgui.h>
#include <algorithm>
#include <imgui_internal.h> // ClearActiveID, ClosePopupsExceptModals, CalcTypematicRepeatAmount
#include <array>
#include <cmath>
#include <cstdio>
#include <utility>
#include <string>
#include "button_remap.h"
#include "desktop_theme.h"

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

// --- the rows -----------------------------------------------------------------
//
// Each setting is a row: its name on the left, its control on the right, the
// whole row one stop for the pad's cursor. The row the cursor is on (or the
// pointer is over) is lit in gold, the light gliding from row to row with the
// wind's swirl beside it, and what the row is for is read on the slip under the
// list (bw_menu_slip). A tab's body calls bw_menu_rows_begin first and
// bw_menu_rows_end last; draw_setting (menu_draw.h) wraps each of its rows in
// bw_menu_row_begin and bw_menu_row_end.

// Which leads, the pointer or the keys (the arrows, which the pad holds):
// whichever moved last. Worked out once a frame.
inline bool bw_menu_pointer_leads() {
    static bool pointer = false;
    static int frame = -1;
    if (frame == ImGui::GetFrameCount())
        return pointer;
    frame = ImGui::GetFrameCount();
    const ImGuiIO& io = ImGui::GetIO();
    if (io.MouseDelta.x != 0.f || io.MouseDelta.y != 0.f || ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        pointer = true;
    for (ImGuiKey key : {ImGuiKey_UpArrow, ImGuiKey_DownArrow, ImGuiKey_LeftArrow, ImGuiKey_RightArrow,
                         ImGuiKey_Space, ImGuiKey_Tab})
        if (ImGui::IsKeyPressed(key, false))
            pointer = false;
    return pointer || !ImGui::GetCurrentContext()->NavCursorVisible;
}

// The lit row: where it is, in its window's own place (so it scrolls with
// it), and what the slip says about it. Filled as the rows are drawn and drawn
// from the next frame, so the light can go under the row's text.
struct BwMenuLit {
    ImGuiID window = 0;
    ImRect rect;
    bool found = false;
    std::string title, help;
};
inline BwMenuLit& bw_menu_lit_now() { static BwMenuLit lit; return lit; }
inline BwMenuLit& bw_menu_lit_shown() { static BwMenuLit lit; return lit; }

// How far the rows sit in from the body's edge: room for the swirl.
inline float bw_menu_swirl_room() { return ImGui::GetFontSize() * 1.25f; }

// Call first thing in a tab's body: draws last frame's lit row under this
// frame's text, eased to where it now is, and sets the rows in past the swirl.
inline void bw_menu_rows_begin() {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    BwMenuLit& shown = bw_menu_lit_shown();
    shown = bw_menu_lit_now();
    bw_menu_lit_now() = BwMenuLit();
    const ImVec2 origin = window->DC.CursorStartPos;
    ImGuiStorage* storage = ImGui::GetStateStorage();
    const ImGuiID id = ImGui::GetID("##lit");
    const bool here = shown.found && shown.window == window->ID;
    // It fades out where it was when nothing is lit, and back in from there.
    const float fade = storage->GetFloat(id + 10, 0.f);
    const float target = here ? 1.f : 0.f;
    const float alpha = fade + (target - fade) * std::min(1.f, ImGui::GetIO().DeltaTime * 14.f);
    storage->SetFloat(id + 10, std::fabs(alpha - target) < 0.01f ? target : alpha);
    ImVec4 rect(storage->GetFloat(id + 11, 0.f), storage->GetFloat(id + 12, 0.f), storage->GetFloat(id + 13, 0.f),
                storage->GetFloat(id + 14, 0.f));
    if (here) {
        rect = bw_menu_slide(id, ImVec4(shown.rect.Min.x, shown.rect.Min.y, shown.rect.Max.x, shown.rect.Max.y));
        storage->SetFloat(id + 11, rect.x);
        storage->SetFloat(id + 12, rect.y);
        storage->SetFloat(id + 13, rect.z);
        storage->SetFloat(id + 14, rect.w);
    }
    if (alpha > 0.01f && rect.z > rect.x) {
        using namespace bluewake_ui::palette;
        ImDrawList* paint = ImGui::GetWindowDrawList();
        const ImVec2 a(origin.x + rect.x, origin.y + rect.y), b(origin.x + rect.z, origin.y + rect.w);
        const float round = ImGui::GetStyle().FrameRounding * 1.25f;
        paint->AddRect(ImVec2(a.x - 1.5f, a.y - 1.5f), ImVec2(b.x + 1.5f, b.y + 1.5f),
                       bluewake_ui::u32(kGold, 0.16f * alpha), round + 1.5f, 0, 3.f);
        paint->AddRectFilledMultiColor(ImVec2(a.x + round * 0.3f, a.y + 1.f), ImVec2(b.x - round * 0.3f, b.y - 1.f),
                                       bluewake_ui::u32(kGold, 0.30f * alpha), bluewake_ui::u32(kGold, 0.12f * alpha),
                                       bluewake_ui::u32(kGold, 0.12f * alpha), bluewake_ui::u32(kGold, 0.30f * alpha));
        paint->AddRect(a, b, bluewake_ui::u32(kGold, 0.75f * alpha), round, 0, 1.5f);
        // The swirl bobs gently towards the row.
        const float bob = std::sin(static_cast<float>(ImGui::GetTime()) * 4.f) * ImGui::GetFontSize() * 0.08f;
        const float r = ImGui::GetFontSize() * 0.42f;
        const float top = std::min(b.y, a.y + ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.y);
        const float side = ImGui::GetStyle().WindowPadding.x * 0.6f;
        bluewake_ui::swirl(paint, ImVec2(a.x + side + bw_menu_swirl_room() * 0.4f + bob, (a.y + top) * 0.5f), r,
                           bluewake_ui::u32(kGold, alpha), std::max(1.5f, ImGui::GetFontSize() * 0.11f));
    }
    ImGui::Indent(bw_menu_swirl_room());
}

// Where a row (or the item a cursor is on) is lit: across the body, from its
// edge to its edge, half the spacing above and below so lit rows meet.
inline void bw_menu_light(ImGuiWindow* window, float top, float bottom, const char* title, const char* help) {
    BwMenuLit& lit = bw_menu_lit_now();
    const float half = ImGui::GetStyle().ItemSpacing.y * 0.5f;
    const float side = ImGui::GetStyle().WindowPadding.x * 0.6f;
    const ImVec2 origin = window->DC.CursorStartPos;
    lit.window = window->ID;
    lit.rect = ImRect(window->WorkRect.Min.x - side - origin.x, top - half - origin.y,
                      window->WorkRect.Max.x + side - origin.x, bottom + half - origin.y);
    lit.found = true;
    lit.title = title != nullptr ? title : "";
    lit.help = help != nullptr ? help : "";
}

// Wraps one row. The cursor's item is known to be in it when ImGui met the
// cursor's item while the row was drawn (NavIdIsAlive, false until then).
inline bool& bw_menu_row_cursor_before() { static bool before = false; return before; }
inline void bw_menu_row_begin() {
    bw_menu_row_cursor_before() = ImGui::GetCurrentContext()->NavIdIsAlive;
    ImGui::BeginGroup();
}
inline void bw_menu_row_end(const char* title, const char* help) {
    ImGui::EndGroup();
    ImGuiContext& g = *ImGui::GetCurrentContext();
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    const ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
    if (bw_menu_pointer_leads()) {
        const ImVec2 mouse = ImGui::GetIO().MousePos;
        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && mouse.y >= a.y &&
            mouse.y < b.y + ImGui::GetStyle().ItemSpacing.y)
            bw_menu_light(window, a.y, b.y, title, help);
        return;
    }
    if (bw_menu_row_cursor_before() || !g.NavIdIsAlive || g.NavWindow != window)
        return;
    // A tall row (a table, a list of its own) lights the line the cursor is
    // on, not all of itself.
    if (b.y - a.y > ImGui::GetFrameHeightWithSpacing() * 3.5f) {
        const ImRect item = ImGui::WindowRectRelToAbs(window, window->NavRectRel[0]);
        bw_menu_light(window, item.Min.y, item.Max.y, title, help);
        return;
    }
    bw_menu_light(window, a.y, b.y, title, help);
}

// Call last thing in a tab's body: a cursor on something that is not a row (a
// button under the rows) is lit there.
inline void bw_menu_rows_end() {
    ImGui::Unindent(bw_menu_swirl_room());
    ImGuiContext& g = *ImGui::GetCurrentContext();
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (bw_menu_lit_now().found || bw_menu_pointer_leads() || !g.NavIdIsAlive || g.NavWindow != window)
        return;
    const ImRect item = ImGui::WindowRectRelToAbs(window, window->NavRectRel[0]);
    bw_menu_light(window, item.Min.y, item.Max.y, nullptr, nullptr);
}

// The row's own lit look: the cursor on it, or the pointer over it.
inline bool bw_menu_item_lit() {
    return (ImGui::IsItemFocused() && !bw_menu_pointer_leads()) ||
           ImGui::IsItemHovered(ImGuiHoveredFlags_NoNavOverride | ImGuiHoveredFlags_AllowWhenDisabled);
}

// A row's name, on its left, wrapped to `width`.
inline float bw_menu_label_height(const char* label, float width) {
    return ImGui::CalcTextSize(label, ImGui::FindRenderedTextEnd(label), false, std::max(1.f, width)).y;
}
inline void bw_menu_draw_label(ImVec2 at, const char* label, float width, bool lit) {
    ImDrawList* paint = ImGui::GetWindowDrawList();
    paint->AddText(ImGui::GetFont(), ImGui::GetFontSize(), at,
                   lit ? ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, 1.f)) : ImGui::GetColorU32(ImGuiCol_Text), label,
                   ImGui::FindRenderedTextEnd(label), std::max(1.f, width));
}

// The arrows either side of a value: ‹ and ›, drawn as small chevrons.
inline void bw_menu_chevron(ImDrawList* paint, ImVec2 centre, float size, int dir, ImU32 colour) {
    const float w = size * 0.32f, h = size * 0.5f;
    paint->AddPolyline(std::array<ImVec2, 3>{ImVec2(centre.x - dir * w * 0.5f, centre.y - h * 0.5f),
                                             ImVec2(centre.x + dir * w * 0.5f, centre.y),
                                             ImVec2(centre.x - dir * w * 0.5f, centre.y + h * 0.5f)}
                           .data(),
                       3, colour, 0, std::max(1.5f, size * 0.12f));
}

// A list's choices as one value with arrows either side and a dot for each
// choice under it, the chosen one gold. Left and right step through them, A
// steps on to the next (round to the first), and the pointer clicks an arrow,
// a dot, or the row for the next. The name is on the left; when the row is too
// narrow for both, the value goes under it.
inline bool bw_menu_combo(const char* label, int* index, const char* const* items, int count) {
    constexpr int kMost = 16;
    count = std::clamp(count, 0, kMost);
    if (count == 0)
        return false;
    ImGui::PushID(label);
    const float font = ImGui::GetFontSize();
    const float width = std::max(1.f, ImGui::GetContentRegionAvail().x);
    const float line = ImGui::GetFrameHeight();
    float widest = 0.f;
    for (int i = 0; i < count; ++i)
        widest = std::max(widest, ImGui::CalcTextSize(items[i]).x);
    const float arrow = font * 1.1f;
    const float value_w = std::min(width, std::max(widest + font * 0.6f, font * 4.f) + arrow * 2.f);
    const float pips_h = font * 0.4f;
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const bool stacked = width - value_w - spacing < font * 6.f;
    const float label_w = stacked ? width : width - value_w - spacing;
    const float label_h = bw_menu_label_height(label, label_w);
    const float control_h = line + pips_h;
    const float height = stacked ? label_h + control_h : std::max(label_h, control_h);
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const ImVec2 va(origin.x + width - value_w, stacked ? origin.y + label_h : origin.y + (height - control_h) * 0.5f);
    const ImVec2 vb(va.x + value_w, va.y + line);
    const bool pressed = ImGui::InvisibleButton("##choices", ImVec2(width, height), ImGuiButtonFlags_EnableNav);
    const bool lit = bw_menu_item_lit();
    bool changed = false;
    const auto choose = [&](int i) {
        i = std::clamp(i, 0, count - 1);
        changed = changed || i != *index;
        *index = i;
    };
    const int was = std::clamp(*index, 0, count - 1);
    const float pip_gap = font * 0.5f;
    const float pips_x = (va.x + vb.x) * 0.5f - pip_gap * static_cast<float>(count - 1) * 0.5f;
    const float pips_y = vb.y + pips_h * 0.35f;
    if (pressed) {
        const ImVec2 mouse = ImGui::GetIO().MousePos;
        const bool by_pointer = ImGui::GetCurrentContext()->ActiveIdSource == ImGuiInputSource_Mouse ||
                                ImGui::IsMouseReleased(ImGuiMouseButton_Left);
        int pip = -1;
        for (int i = 0; i < count && by_pointer; ++i)
            if (std::fabs(mouse.x - (pips_x + pip_gap * static_cast<float>(i))) < pip_gap * 0.5f && std::fabs(mouse.y - pips_y) < pips_h)
                pip = i;
        if (pip >= 0)
            choose(pip);
        else if (by_pointer && mouse.x >= va.x && mouse.x < va.x + arrow && mouse.y >= va.y && mouse.y < vb.y)
            choose(was - 1);
        else if (by_pointer && mouse.x >= vb.x - arrow && mouse.x < vb.x && mouse.y >= va.y && mouse.y < vb.y)
            choose(was + 1);
        else
            choose((was + 1) % count);
    }
    if (const int step = bw_menu_slider_nudge())
        choose(*index + step);
    const int at = std::clamp(*index, 0, count - 1);

    using namespace bluewake_ui::palette;
    ImDrawList* paint = ImGui::GetWindowDrawList();
    bw_menu_draw_label(ImVec2(origin.x, stacked ? origin.y : origin.y + (height - label_h) * 0.5f), label, label_w,
                       lit);
    const ImU32 gold = bluewake_ui::u32(kGold);
    if (lit) {
        bw_menu_chevron(paint, ImVec2(va.x + arrow * 0.5f, (va.y + vb.y) * 0.5f), font, -1,
                        at > 0 ? gold : bluewake_ui::u32(kGold, 0.35f));
        bw_menu_chevron(paint, ImVec2(vb.x - arrow * 0.5f, (va.y + vb.y) * 0.5f), font, 1,
                        at < count - 1 ? gold : bluewake_ui::u32(kGold, 0.35f));
    }
    const ImVec2 text = ImGui::CalcTextSize(items[at]);
    const ImVec2 ta(va.x + arrow, va.y), tb(vb.x - arrow, vb.y);
    paint->PushClipRect(ta, tb, true);
    paint->AddText(ImVec2(std::max(ta.x, (ta.x + tb.x - text.x) * 0.5f), va.y + (line - text.y) * 0.5f),
                   lit ? gold : ImGui::GetColorU32(ImGuiCol_Text), items[at]);
    paint->PopClipRect();
    for (int i = 0; i < count; ++i)
        paint->AddCircleFilled(ImVec2(pips_x + pip_gap * static_cast<float>(i), pips_y), font * 0.13f,
                               i == at ? gold : bluewake_ui::u32(kCream, 0.3f));
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

// A pane that opens and closes under its name: a button as wide as the name
// and a little room after it, not a bar across the menu. True while it is open.
inline bool bw_menu_pane(const char* label) {
    const std::string shown = std::string(label) + "    ###" + label;
    return ImGui::TreeNodeEx(shown.c_str(), ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanLabelWidth |
                                                ImGuiTreeNodeFlags_NoTreePushOnOpen);
}

// A tooltip at the size of the text it is for. A tooltip is a window of its
// own, so the menu's window scale (each platform's, times the menu text size)
// doesn't reach it, and it came out at the font's own size whatever the
// player chose.
inline void bw_menu_tooltip(const char* text) {
    const float size = ImGui::GetFontSize();
    if (!ImGui::BeginTooltip())
        return;
    // Against the font's own size: the tooltip window keeps its scale from one
    // frame to the next.
    ImGui::SetWindowFontScale(size / ImGui::GetCurrentContext()->FontBaseSize);
    ImGui::PushTextWrapPos(size * 30.f);
    ImGui::TextUnformatted(text);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
}

// A slider as a row: its name on the left, then a track with a diamond on it
// and the value. A controller nudges it left and right a twentieth of its
// range at a time; the pointer drags along the track or clicks the arrows.
// When the row is too narrow, the track goes under the name.
inline bool bw_menu_slider(const char* label, float* value, float low, float high, const char* format) {
    ImGui::PushID(label);
    const float font = ImGui::GetFontSize();
    const float width = std::max(1.f, ImGui::GetContentRegionAvail().x);
    const float line = ImGui::GetFrameHeight();
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    char shown[64], widest[64];
    std::snprintf(shown, sizeof shown, format, *value);
    std::snprintf(widest, sizeof widest, format, std::fabs(low) > std::fabs(high) ? low : high);
    const float number_w = std::max(ImGui::CalcTextSize(widest).x, ImGui::CalcTextSize(shown).x);
    const float arrow = font * 1.1f;
    const float track_min = font * 7.f;
    const float control_w = std::min(width, std::max(track_min, width * 0.42f) + arrow * 2.f + number_w + spacing);
    const bool stacked = width - control_w - spacing < font * 6.f;
    const float label_w = stacked ? width : width - control_w - spacing;
    const float label_h = bw_menu_label_height(label, label_w);
    const float height = stacked ? label_h + line : std::max(label_h, line);
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const float cy = stacked ? origin.y + label_h + line * 0.5f : origin.y + height * 0.5f;
    const float cx0 = origin.x + width - control_w;
    const ImVec2 ta(cx0 + arrow, cy - font * 0.14f), tb(origin.x + width - number_w - spacing - arrow, cy + font * 0.14f);
    ImGui::InvisibleButton("##value", ImVec2(width, height), ImGuiButtonFlags_EnableNav);
    const bool lit = bw_menu_item_lit();
    ImGuiStorage* storage = ImGui::GetStateStorage();
    const ImGuiID dragging = ImGui::GetID("##drag");
    const ImVec2 mouse = ImGui::GetIO().MousePos;
    const bool by_pointer = ImGui::GetCurrentContext()->ActiveIdSource == ImGuiInputSource_Mouse;
    bool changed = false;
    const auto set = [&](float v) {
        v = std::clamp(v, low, high);
        changed = changed || v != *value;
        *value = v;
    };
    const float step = (high - low) / 20.f;
    if (ImGui::IsItemActivated() && by_pointer) {
        const bool on_track = mouse.x >= ta.x - font * 0.5f && mouse.x <= tb.x + font * 0.5f;
        storage->SetBool(dragging, on_track);
        if (!on_track && mouse.x >= cx0 && mouse.x < ta.x)
            set(*value - step);
        else if (!on_track && mouse.x > tb.x && mouse.x <= tb.x + arrow)
            set(*value + step);
    }
    if (ImGui::IsItemActive() && by_pointer && storage->GetBool(dragging, false) && tb.x > ta.x)
        set(low + (high - low) * std::clamp((mouse.x - ta.x) / (tb.x - ta.x), 0.f, 1.f));
    if (!ImGui::IsItemActive())
        storage->SetBool(dragging, false);
    if (const int nudge = bw_menu_slider_nudge())
        set(*value + static_cast<float>(nudge) * step);

    using namespace bluewake_ui::palette;
    ImDrawList* paint = ImGui::GetWindowDrawList();
    bw_menu_draw_label(ImVec2(origin.x, stacked ? origin.y : origin.y + (height - label_h) * 0.5f), label, label_w,
                       lit);
    const ImU32 gold = bluewake_ui::u32(kGold);
    const float t = high > low ? std::clamp((*value - low) / (high - low), 0.f, 1.f) : 0.f;
    const float knob_x = ta.x + (tb.x - ta.x) * t;
    const float round = (tb.y - ta.y) * 0.5f;
    paint->AddRectFilled(ta, tb, ImGui::GetColorU32(ImGuiCol_FrameBg), round);
    paint->AddRectFilled(ta, ImVec2(knob_x, tb.y), lit ? gold : bluewake_ui::u32(kCream, 0.8f), round);
    const float d = font * 0.42f;
    const ImVec2 diamond[4] = {ImVec2(knob_x, cy - d), ImVec2(knob_x + d, cy), ImVec2(knob_x, cy + d),
                               ImVec2(knob_x - d, cy)};
    paint->AddConvexPolyFilled(diamond, 4, lit ? gold : ImGui::GetColorU32(kCream));
    paint->AddPolyline(diamond, 4, bluewake_ui::u32(kInk), ImDrawFlags_Closed, std::max(1.5f, font * 0.1f));
    if (lit) {
        bw_menu_chevron(paint, ImVec2(cx0 + arrow * 0.45f, cy), font, -1, gold);
        bw_menu_chevron(paint, ImVec2(tb.x + arrow * 0.55f, cy), font, 1, gold);
    }
    std::snprintf(shown, sizeof shown, format, *value);
    const ImVec2 text = ImGui::CalcTextSize(shown);
    paint->AddText(ImVec2(origin.x + width - text.x, cy - text.y * 0.5f), lit ? gold : ImGui::GetColorU32(ImGuiCol_Text),
                   shown);
    ImGui::PopID();
    return changed;
}

// A switch as a row: its name on the left and Off and On on the right, the one
// it is at in gold. A or a click turns it over, left is Off and right is On,
// and a click on Off or On sets that.
inline bool bw_menu_checkbox(const char* label, bool* value) {
    ImGui::PushID(label);
    const float font = ImGui::GetFontSize();
    const float width = std::max(1.f, ImGui::GetContentRegionAvail().x);
    const float line = ImGui::GetFrameHeight();
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float cell = std::max(ImGui::CalcTextSize("Off").x, ImGui::CalcTextSize("On").x) + font * 1.2f;
    const float pill_w = cell * 2.f + 4.f, pill_h = line * 0.86f;
    const bool stacked = width - pill_w - spacing < font * 6.f;
    const float label_w = stacked ? width : width - pill_w - spacing;
    const float label_h = bw_menu_label_height(label, label_w);
    const float height = stacked ? label_h + line : std::max(label_h, line);
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const float cy = stacked ? origin.y + label_h + line * 0.5f : origin.y + height * 0.5f;
    const ImVec2 pa(origin.x + width - pill_w, cy - pill_h * 0.5f), pb(origin.x + width, cy + pill_h * 0.5f);
    const bool pressed = ImGui::InvisibleButton("##switch", ImVec2(width, height), ImGuiButtonFlags_EnableNav);
    const bool lit = bw_menu_item_lit();
    bool changed = false;
    if (pressed) {
        const ImVec2 mouse = ImGui::GetIO().MousePos;
        const bool on_pill = ImGui::IsMouseReleased(ImGuiMouseButton_Left) && mouse.x >= pa.x && mouse.y >= pa.y &&
                             mouse.y <= pb.y;
        const bool want = on_pill ? mouse.x >= pa.x + pill_w * 0.5f : !*value;
        changed = want != *value;
        *value = want;
    }
    if (const int step = bw_menu_slider_nudge()) {
        const bool want = step > 0;
        changed = changed || want != *value;
        *value = want;
    }

    using namespace bluewake_ui::palette;
    ImDrawList* paint = ImGui::GetWindowDrawList();
    bw_menu_draw_label(ImVec2(origin.x, stacked ? origin.y : origin.y + (height - label_h) * 0.5f), label, label_w,
                       lit);
    paint->AddRectFilled(pa, pb, ImGui::GetColorU32(ImGuiCol_FrameBg), pill_h * 0.5f);
    // The gold half slides to where the switch now is.
    const float left = pa.x + 2.f + (*value ? cell : 0.f);
    const ImVec4 knob = bw_menu_slide(ImGui::GetID("##knob"), ImVec4(left, pa.y + 2.f, left + cell, pb.y - 2.f));
    // On is gold; Off is a quiet cream, so the two read apart at a glance.
    paint->AddRectFilled(ImVec2(knob.x, knob.y), ImVec2(knob.z, knob.w),
                         *value ? bluewake_ui::u32(kGold, lit ? 1.f : 0.88f) : bluewake_ui::u32(kCream, lit ? 0.32f : 0.22f),
                         (pill_h - 4.f) * 0.5f);
    const char* names[2] = {"Off", "On"};
    for (int i = 0; i < 2; ++i) {
        const ImVec2 size = ImGui::CalcTextSize(names[i]);
        const bool chosen = (i == 1) == *value;
        paint->AddText(ImVec2(pa.x + 2.f + cell * static_cast<float>(i) + (cell - size.x) * 0.5f, cy - size.y * 0.5f),
                       chosen ? (*value ? bluewake_ui::u32(kInk) : ImGui::GetColorU32(ImGuiCol_Text))
                              : ImGui::GetColorU32(ImGuiCol_TextDisabled, 0.75f),
                       names[i]);
    }
    ImGui::PopID();
    return changed;
}

// --- the frame -----------------------------------------------------------------

// The panel's stitched edge, a little in from the window's border.
inline void bw_menu_panel_edge(float scale) {
    const ImGuiWindow* self = ImGui::GetCurrentWindow();
    const float in = 7.f * scale;
    bluewake_ui::stitch(ImGui::GetWindowDrawList(), ImVec2(self->Pos.x + in, self->Pos.y + in),
                        ImVec2(self->Pos.x + self->Size.x - in, self->Pos.y + self->Size.y - in),
                        std::max(0.f, ImGui::GetStyle().WindowRounding - in),
                        bluewake_ui::u32(bluewake_ui::palette::kCream, 0.35f), 5.f * scale, std::max(1.f, 1.5f * scale));
}

// The tabs on a wooden plaque `width` wide, as the game titles its pages: LB
// and RB at its ends (a click on one steps the tab too), the open tab's name in
// gold outlined in dark brown with a bar under it. Each name is a stop for the
// pad's cursor; A or a click opens it.
inline void bw_menu_tab_plaque(const char* const* names, int count, int* shown, float width, float height,
                               float scale) {
    using namespace bluewake_ui::palette;
    ImDrawList* paint = ImGui::GetWindowDrawList();
    const ImVec2 a = ImGui::GetCursorScreenPos(), b(a.x + width, a.y + height);
    const float round = height * 0.3f;
    paint->AddRectFilled(ImVec2(a.x, a.y + 3.f * scale), ImVec2(b.x, b.y + 3.f * scale), bluewake_ui::u32(kWoodDark),
                         round);
    paint->AddRectFilled(a, b, bluewake_ui::u32(kWood), round);
    paint->AddLine(ImVec2(a.x + round, a.y + 1.f), ImVec2(b.x - round, a.y + 1.f), IM_COL32(255, 255, 255, 34));
    const float pad_x = height * 0.2f;
    const float r = ImGui::GetTextLineHeight() * 0.5f;
    const float bumper = bluewake_ui::glyph_width("LB", r);
    const float mid_y = (a.y + b.y) * 0.5f;
    for (int side = 0; side < 2; ++side) {
        const char* name = side == 0 ? "LB" : "RB";
        const float x = side == 0 ? a.x + pad_x : b.x - pad_x - bumper;
        ImGui::SetCursorScreenPos(ImVec2(x, a.y));
        ImGui::PushID(name);
        if (ImGui::InvisibleButton("##bumper", ImVec2(bumper, height)))
            *shown = (*shown + count + (side == 0 ? -1 : 1)) % count;
        ImGui::PopID();
        const ImVec2 c(x + bumper * 0.5f, mid_y);
        paint->AddRectFilled(ImVec2(c.x - bumper * 0.5f, c.y - r), ImVec2(c.x + bumper * 0.5f, c.y + r),
                             bluewake_ui::u32(kCream, ImGui::IsItemHovered() ? 1.f : 0.85f), r * 0.6f);
        const ImVec2 size = ImGui::CalcTextSize(name);
        paint->AddText(ImVec2(c.x - size.x * 0.5f, c.y - size.y * 0.5f), bluewake_ui::u32(kInk), name);
    }
    const float left = a.x + pad_x * 2.f + bumper, room = std::max(1.f, b.x - pad_x * 2.f - bumper - left);
    // Each name gets room for itself and a share of what is left. Names that
    // don't fit are drawn smaller, down to seven tenths; narrower still, only
    // the open tab's name is shown, and LB and RB step through them.
    constexpr int kMost = 12;
    count = std::min(count, kMost);
    const float font = ImGui::GetFontSize();
    float need[kMost], total = 0.f;
    for (int i = 0; i < count; ++i) {
        need[i] = ImGui::CalcTextSize(names[i]).x;
        total += need[i] + font;
    }
    const float shrink = std::min(1.f, room / std::max(1.f, total));
    const bool only_open = shrink < 0.7f;
    const float size = font * (only_open ? 1.f : shrink);
    const float spare = only_open ? 0.f : std::max(0.f, room - total * shrink) / static_cast<float>(count);
    float x = left;
    for (int i = 0; i < count; ++i) {
        if (only_open && i != *shown)
            continue;
        const float cell = only_open ? room : (need[i] + font) * shrink + spare;
        const ImVec2 ca(only_open ? left : x, a.y), cb(ca.x + cell, b.y);
        x += cell;
        ImGui::SetCursorScreenPos(ca);
        ImGui::PushID(i);
        if (ImGui::InvisibleButton("##tab", ImVec2(std::max(1.f, cell), height), ImGuiButtonFlags_EnableNav))
            *shown = i;
        ImGui::PopID();
        const bool cursor = ImGui::IsItemFocused() && !bw_menu_pointer_leads();
        const bool lit = ImGui::IsItemHovered() || cursor;
        const ImVec2 la(ca.x + 2.f, ca.y + 4.f * scale), lb(cb.x - 2.f, cb.y - 4.f * scale);
        if (lit && i != *shown)
            paint->AddRectFilled(la, lb, bluewake_ui::u32(kCream, 0.14f), round * 0.7f);
        if (cursor)
            paint->AddRect(la, lb, bluewake_ui::u32(kGold, 0.85f), round * 0.7f, 0, 1.5f);
        const float text_w = need[i] * size / font, text_h = ImGui::GetTextLineHeight() * size / font;
        const ImVec2 at(ca.x + (cell - text_w) * 0.5f, mid_y - text_h * 0.5f);
        paint->PushClipRect(ca, cb, true);
        if (i == *shown) {
            bluewake_ui::outlined_text(paint, at, bluewake_ui::u32(kGold), bluewake_ui::u32(kInk), names[i], nullptr,
                                       std::max(1.f, 1.5f * scale), size);
            const float bar = std::min(text_w * 0.4f, cell * 0.4f);
            paint->AddRectFilled(ImVec2((ca.x + cb.x - bar) * 0.5f, b.y - 6.f * scale),
                                 ImVec2((ca.x + cb.x + bar) * 0.5f, b.y - 3.f * scale), bluewake_ui::u32(kGold),
                                 2.f * scale);
        } else {
            paint->AddText(ImGui::GetFont(), size, at, bluewake_ui::u32(kCream, lit ? 0.95f : 0.6f), names[i]);
        }
        paint->PopClipRect();
    }
    ImGui::SetCursorScreenPos(ImVec2(a.x, b.y + 3.f * scale));
    ImGui::Dummy(ImVec2(1.f, ImGui::GetStyle().ItemSpacing.y * 0.5f));
}

// The cream slip under the list: the lit row's name and what it is for, as
// the game's own options explain the chosen one underneath. `height` is fixed
// per tab (bw_menu::slip_height), so the slip doesn't jump.
inline void bw_menu_slip(float height, float pad, float scale) {
    using namespace bluewake_ui::palette;
    const BwMenuLit& lit = bw_menu_lit_shown();
    ImDrawList* paint = ImGui::GetWindowDrawList();
    const ImVec2 a = ImGui::GetCursorScreenPos();
    const ImVec2 b(a.x + ImGui::GetContentRegionAvail().x, a.y + height);
    const float text_w = std::max(1.f, b.x - a.x - pad * 2.f);
    const float round = ImGui::GetStyle().FrameRounding * 1.5f;
    paint->AddRectFilled(ImVec2(a.x, a.y + 3.f * scale), ImVec2(b.x, b.y + 3.f * scale), bluewake_ui::u32(kSlipEdge),
                         round);
    paint->AddRectFilled(a, b, bluewake_ui::u32(kSlip), round);
    paint->PushClipRect(ImVec2(a.x + pad * 0.5f, a.y), ImVec2(b.x - pad * 0.5f, b.y), true);
    ImVec2 at(a.x + pad, a.y + pad);
    const bool any = lit.found && !lit.title.empty();
    const char* title = any ? lit.title.c_str() : "Choose a setting to read about it here.";
    ImFont* font = ImGui::GetFont();
    const float size = ImGui::GetFontSize();
    paint->AddText(font, size, at, bluewake_ui::u32(any ? kInk : kSlipInk), title, ImGui::FindRenderedTextEnd(title),
                   text_w);
    if (any && !lit.help.empty()) {
        at.y += ImGui::CalcTextSize(title, nullptr, true, text_w).y + ImGui::GetStyle().ItemSpacing.y * 0.4f;
        paint->AddText(font, size, at, bluewake_ui::u32(kSlipInk), lit.help.c_str(), nullptr, text_w);
    }
    paint->PopClipRect();
    ImGui::Dummy(ImVec2(1.f, height + 3.f * scale));
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
// button mapping; "" is the controller's own), else Swap A and B, as the game reads them.
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

// Down (the D-pad, the left stick or the arrow key) on a tab's last setting
// moves the cursor nowhere, so the notes and greyed rows under it were never
// shown: a press that leaves the cursor where it was scrolls the body instead,
// and so does Up on the first. ImGui moves the cursor a frame or two after the
// press, so the press is looked at again two frames later. Call it inside the
// tab's body, after its rows.
inline void bw_menu_scroll_past_ends() {
    static ImGuiID s_from = 0;
    static int s_dir = 0, s_wait = 0;
    const ImGuiID now = ImGui::GetFocusID();
    if (s_dir != 0 && --s_wait <= 0) {
        if (now == s_from && now != 0)
            ImGui::SetScrollY(ImGui::GetScrollY() + s_dir * ImGui::GetTextLineHeightWithSpacing() * 3.f);
        s_dir = 0;
    }
    if (s_dir == 0 && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
        const int dir = ImGui::IsKeyPressed(ImGuiKey_DownArrow, true) ? 1 : ImGui::IsKeyPressed(ImGuiKey_UpArrow, true) ? -1 : 0;
        if (dir != 0) {
            s_from = now;
            s_dir = dir;
            s_wait = 2;
        }
    }
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

// The hint, as the game's own corner shows it (the buttons' glyphs and what
// they do), and the next-launch note. The caller puts its Restart now button,
// if it has one, on the same line right after it (ImGui::SameLine()).
inline void bw_menu_footer_note(bool restart_note) {
    using namespace bluewake_ui::palette;
    ImGui::Spacing();
    ImDrawList* paint = ImGui::GetWindowDrawList();
    const float h = ImGui::GetTextLineHeight();
    const float r = h * 0.5f;
    ImVec2 at = ImGui::GetCursorScreenPos();
    const float right = at.x + ImGui::GetContentRegionAvail().x;
    const struct { const char* glyph; const ImVec4* colour; const char* does; } hints[] = {
        {"A", &kButtonA, "Choose"}, {"B", &kButtonB, "Back"}, {"LB", &kBumper, nullptr},
        {"RB", &kBumper, "Tabs"}, {"RS", &kBumper, "Scroll"}};
    for (const auto& hint : hints) {
        const float gw = bluewake_ui::glyph_width(hint.glyph, r);
        const float tw = hint.does != nullptr ? ImGui::CalcTextSize(hint.does).x + h * 0.3f : 0.f;
        if (at.x + gw + tw > right)
            break;
        bluewake_ui::glyph(paint, ImVec2(at.x + gw * 0.5f, at.y + r), r, bluewake_ui::u32(*hint.colour), hint.glyph);
        at.x += gw + h * 0.3f;
        if (hint.does != nullptr) {
            paint->AddText(at, ImGui::GetColorU32(ImGuiCol_Text), hint.does);
            at.x += tw + h * 0.6f;
        }
    }
    ImGui::Dummy(ImVec2(1.f, h));
    if (restart_note) {
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(kGold, "Some changes take effect at the next launch.");
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
        // Pills, Resume in gold: the way back to the game is the one to find.
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, ImGui::GetFrameHeight() * 0.5f);
        if (i == 0) {
            using namespace bluewake_ui::palette;
            ImGui::PushStyleColor(ImGuiCol_Button, kGold);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.f, 0.86f, 0.40f, 1.f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.f, 0.90f, 0.50f, 1.f));
            ImGui::PushStyleColor(ImGuiCol_Text, kInk);
        }
        if (ImGui::Button(kBwMenuButtons[i]))
            pressed = static_cast<BwMenuButton>(i + 1);
        if (i == 0)
            ImGui::PopStyleColor(4);
        ImGui::PopStyleVar();
        if (i == 0 && settings_path != nullptr && settings_path[0] != '\0' && ImGui::IsItemHovered())
            bw_menu_tooltip((std::string("Preferences are saved when you resume:\n") + settings_path).c_str());
    }
    return pressed;
}
