// SPDX-License-Identifier: GPL-3.0-or-later
// The Windows settings file from before the menus were one (aspect=21:9,
// option.NAME=1, ...), read through settings_table.h: each row's old_windows
// names its old key, and its kind says how the value is read (a switch's
// true/on/yes, a number in its stored format). The few old keys that became
// more than one setting, or a different kind of value, are converted by name.
// A setting the old file doesn't name gets its default at launch, as before.
// No Windows calls here: the host tests run it on every platform.
#pragma once
#include "settings_table.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace bw_settings {

using OldLines = std::vector<std::pair<std::string, std::string>>;

// An old switch: 1, true, on or yes.
inline bool old_on(const std::string& value) {
    return value == "1" || value == "true" || value == "on" || value == "yes";
}

// The row whose old key is `key`, or nullptr. Two old keys became two rows
// each (fast_transitions, sprint_speed); those are converted by name below.
inline const Setting* old_row(const std::string& key) {
    for (const Setting& s : kSettings)
        if (s.old_windows != nullptr && key == s.old_windows)
            return &s;
    return nullptr;
}

// Whether `key` is one the old file used (and the new one never does).
inline bool old_key(const std::string& key) {
    if (key.rfind("BLUEWAKE_", 0) == 0 || key.rfind("DOL_", 0) == 0)
        return false;
    return old_row(key) != nullptr || key == "hd_textures" || key.rfind("option.", 0) == 0;
}

// A value in a row's own form: a switch as 1 or 0, a whole number as one
// (inside the row's range when it has one), a fractional one in the format the
// menu stores it in; a choice or text as it was.
inline std::string old_value(const Setting& s, const std::string& value) {
    switch (s.kind) {
    case Kind::Bool:
        return old_on(value) ? "1" : "0";
    case Kind::Int: {
        int number = std::atoi(value.c_str());
        if (s.high > s.low)
            number = std::clamp(number, static_cast<int>(s.low), static_cast<int>(s.high));
        return std::to_string(number);
    }
    case Kind::Float: {
        char text[32];
        std::snprintf(text, sizeof text, s.stored != nullptr ? s.stored : "%.3f", std::atof(value.c_str()));
        return text;
    }
    default:
        return value;
    }
}

// `lines` from an old file as the desktop format's, in the table's order, the
// lines it didn't know after them: true when they were old and are converted,
// false (and left alone) otherwise. `default_pack` is the folder an old
// "HD textures on" without a folder of its own meant.
inline bool convert_old_windows(OldLines& lines, const std::string& default_pack) {
    if (std::none_of(lines.begin(), lines.end(), [](const auto& line) { return old_key(line.first); }))
        return false;
    std::map<std::string, std::string> converted;
    std::map<std::string, bool> options;
    OldLines unknown;
    bool hd = false, have_pack = false, extras = false, have_sprint = false;
    std::string pack;
    double sprint = 1.5;
    for (const auto& [key, value] : lines) {
        if (!old_key(key)) {
            unknown.emplace_back(key, value);
        } else if (key == "hd_textures") {
            hd = old_on(value);
            have_pack = true;
        } else if (key == "texture_pack") {
            pack = value;
            have_pack = true;
        } else if (key.rfind("option.", 0) == 0) {
            options[key.substr(7)] = old_on(value);
        } else if (key == "movement_extras") {
            extras = old_on(value);
            have_sprint = true;
        } else if (key == "sprint_speed") {
            sprint = std::clamp(std::atof(value.c_str()), 1.1, 2.0);
            have_sprint = true;
        } else if (key == "fast_transitions") {
            converted["BLUEWAKE_FAST_FORWARD"] = old_on(value) ? "1" : "0";
            converted["BLUEWAKE_FADE_FRAMES"] = old_on(value) ? "6" : "0";
        } else if (key == "smooth_motion_fps") {
            converted["DOL_AURORA_FRAME_INTERP_STEPS"] =
                value == "display" ? "display" : std::atoi(value.c_str()) >= 120 ? "3" : "1";
        } else if (key == "aspect") {
            converted["BLUEWAKE_ASPECT"] = value == "16:9" || value == "16:10" || value == "21:9" ? value : "4:3";
        } else if (key == "haptics") {
            converted["BLUEWAKE_HAPTICS"] = value == "off" || value == "classic" ? value : "enhanced";
        } else if (key == "betterww") {
            converted["BLUEWAKE_MODS"] = old_on(value) ? "betterww" : "";
        } else if (key == "lle_audio") {
            converted["BLUEWAKE_DSP_MODE"] = old_on(value) ? "lle" : "hle";
        } else {
            const Setting& s = *old_row(key);
            converted[s.key] = old_value(s, value);
        }
    }
    // HD textures were a switch and a folder; now the folder is the switch, and
    // the folder is kept while they are off.
    if (have_pack) {
        converted["DOL_AURORA_TEXTURE_PACK"] = hd ? (pack.empty() ? default_pack : pack) : "";
        converted["BLUEWAKE_TEXTURE_PACK_FOLDER"] = hd ? "" : pack;
    }
    // Jump & Sprint was a switch and a speed; now the speed is kept apart.
    if (have_sprint) {
        char speed[16];
        std::snprintf(speed, sizeof speed, "%.2f", sprint);
        converted["BLUEWAKE_JUMP_BUTTON"] = extras ? "1" : "0";
        converted["BLUEWAKE_SPRINT_SPEED"] = extras ? speed : "1";
        converted["BLUEWAKE_SPRINT_CHOICE"] = speed;
    }
    // Better Wind Waker's options: "a,-b", a on and b off.
    if (!options.empty()) {
        std::string list;
        for (const auto& [name, on] : options)
            list += (list.empty() ? "" : ",") + std::string(on ? "" : "-") + name;
        converted["BLUEWAKE_OPTIONS"] = list;
    }
    OldLines out;
    for (const Setting& s : kSettings) {
        const auto found = converted.find(s.key);
        if (found != converted.end())
            out.emplace_back(s.key, found->second);
    }
    out.insert(out.end(), unknown.begin(), unknown.end());
    lines = out;
    return true;
}

}  // namespace bw_settings
