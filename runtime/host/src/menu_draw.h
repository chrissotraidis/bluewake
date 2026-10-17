// SPDX-License-Identifier: GPL-3.0-or-later
// Drawing a tab from settings_table.h, shared by every desktop menu: the Mac
// and Linux one (settings_menu.cpp) and the Windows one (win_settings.cpp).
//
// The table says what a setting is and how it is drawn; this says how to draw
// it. What differs between the two menus is behind five hooks each one
// defines: reading and writing a setting, saying it changed, drawing the rows
// that have a control of their own, and the shared widgets' own helpers.
#pragma once
#include <imgui.h>
#include <algorithm>
#include <cstdlib>
#include <string>
#include "menu_shared.h"
#include "settings_table.h"

namespace bw_menu {

// Defined by each menu, which holds its own settings and its own idea of when
// a change has to be written or waits for the next launch.
std::string get(const char* key, const char* fallback); // the setting's value
void put(const char* key, const std::string& value);    // set it, for the game and the file
void changed(const bw_settings::Setting& setting);      // it was just edited
bool custom(const bw_settings::Setting& setting);       // a row with a control of its own: drawn?
// "(next launch)" beside a row, when this menu takes the setting at the next
// launch: the table's next_launch, and on Windows a few more (win_settings.cpp).
void next_launch_note(const bw_settings::Setting& setting);
// What a list of choices offers: what is shown, what is stored, how many.
// Kept per menu because some of it comes from Aurora's own headers.
bool choices(const char* key, const char* const** shown, const char* const** stored, int* count);

inline bool on(const char* key, bool fallback) {
    const std::string value = get(key, fallback ? "1" : "0");
    return !value.empty() && value[0] != '0' && value != "off" && value != "false";
}

// The setting named `key`, or a complaint and a blank: an unknown name is a
// mistake in the menu, not something a player can reach.
inline const bw_settings::Setting& setting(const char* key) {
    static const bw_settings::Setting kMissing{.label = "?"};
    const bw_settings::Setting* found = bw_settings::find(key);
    if (found == nullptr)
        std::fprintf(stderr, "[settings] %s is not in settings_table.h\n", key);
    return found != nullptr ? *found : kMissing;
}

// A switch with its label and default from the table: true when the player
// moved it, and *value is where it now stands.
inline bool draw_switch(const bw_settings::Setting& s, bool* value) {
    *value = on(s.key, bw_settings::default_on(s));
    if (!bw_menu_checkbox(s.label, value))
        return false;
    put(s.key, *value ? "1" : "0");
    return true;
}

// A slider with its label, default, range and formats from the table.
inline bool draw_slider(const bw_settings::Setting& s) {
    const char* fallback_text = bw_settings::fallback_for(s);
    const float fallback = std::strtof(fallback_text, nullptr);
    float value = std::strtof(get(s.key, fallback_text).c_str(), nullptr);
    if (!(value > 0.f))
        value = fallback;
    if (!bw_menu_slider(s.label, &value, s.low, s.high, s.shown != nullptr ? s.shown : "%.2f"))
        return false;
    char text[24];
    std::snprintf(text, sizeof text, s.stored != nullptr ? s.stored : "%.3f", value);
    put(s.key, text);
    return true;
}

inline void draw_control(const bw_settings::Setting& s);

// One row: the control the table asks for, its next-launch note and its quiet
// line. A setting nobody can change right now is drawn greyed, as both menus
// have always drawn it.
inline void draw_setting(const bw_settings::Setting& s) {
    if (s.control == bw_settings::Control::None)
        return;
    if (s.label == nullptr && s.control != bw_settings::Control::Custom)
        return;
    const bool waiting = s.needs != nullptr && !on(s.needs, bw_settings::default_on(setting(s.needs)));
    const bool overridden = s.dead_when != nullptr && !get(s.dead_when, "").empty();
    if (s.group != nullptr)
        s.group[0] == '\0' ? ImGui::Separator() : ImGui::SeparatorText(s.group);
    ImGui::BeginDisabled(waiting || overridden);
    draw_control(s);
    ImGui::EndDisabled();
}

// The row's control itself, its note and what a change does.
inline void draw_control(const bw_settings::Setting& s) {
    if (s.control == bw_settings::Control::Custom) {
        custom(s);
        return;
    }
    bool edited = false;
    switch (s.control) {
    case bw_settings::Control::Switch: {
        bool value = false;
        edited = draw_switch(s, &value);
        break;
    }
    case bw_settings::Control::Slider:
        edited = draw_slider(s);
        break;
    case bw_settings::Control::Chooser: {
        const char* const* shown = nullptr;
        const char* const* stored = nullptr;
        int count = 0;
        if (!choices(s.key, &shown, &stored, &count) || count <= 0)
            break;
        // A value that is none of the choices reads as the default.
        const std::string now = get(s.key, bw_settings::fallback_for(s));
        int index = 0;
        for (int i = 0; i < count; ++i)
            if (std::string(bw_settings::fallback_for(s)) == stored[i])
                index = i;
        for (int i = 0; i < count; ++i)
            if (now == stored[i])
                index = i;
        if (bw_menu_combo(s.label, &index, shown, count)) {
            put(s.key, stored[index]);
            edited = true;
        }
        break;
    }
    default:
        break;
    }
    next_launch_note(s);
    if (s.note != nullptr)
        ImGui::TextDisabled("%s", s.note);
    if (edited)
        changed(s);
}

// Every row of one tab, in the table's order. Experimental rows wait for the
// Mods tab's last section (draw_experimental).
inline void draw_tab(bw_settings::Tab tab) {
    for (const bw_settings::Setting& s : bw_settings::kSettings)
        if (s.tab == tab && s.experimental == nullptr)
            draw_setting(s);
}

// The Experimental section: every row with an experimental line, each with
// that line under it saying what has not been checked yet.
inline void draw_experimental() {
    ImGui::SeparatorText("Experimental");
    ImGui::TextDisabled("Off by default, and still being checked.");
    for (const bw_settings::Setting& s : bw_settings::kSettings) {
        if (s.experimental == nullptr)
            continue;
        draw_setting(s);
        ImGui::TextDisabled("%s", s.experimental);
    }
}

} // namespace bw_menu
