// SPDX-License-Identifier: GPL-3.0-or-later
// The desktop menu's settings core, one for Mac, Linux and Windows
// (desktop_menu.h, desktop_settings.h).
//
// The settings are the host's own variables (BLUEWAKE_*, DOL_*): the settings
// file sets them at launch, where the launch has not already, the menu reads
// and writes them, and the file is written back from them in
// settings_table.h's order. What the launch chose wins for the session while
// the file keeps its own value for it, unless the player changes it here.
#include "desktop_settings.h"
#include "atomic_file.h"
#include "launch_marker.h"
#include "old_windows_settings.h"
#include "settings_table.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
// The Windows host's POSIX shim (windows/compat/bw_posix_compat.h) defines
// rename() as a macro, which breaks <filesystem>'s own rename; set it aside here.
#pragma push_macro("rename")
#undef rename
#include <filesystem>
#pragma pop_macro("rename")
#include <map>
#include <set>
#include <string>
#include <system_error>
#include <vector>

// game_options.h, declared with plain types: the header brings in the guest
// CPU's, which the core has no use for.
extern "C" const char* bluewake_game_options_describe(uint32_t position, const char** title, bool* default_on,
                                                      bool* on);

namespace bw_desktop {
namespace {

std::string g_path;      // the settings file ("" for none)
std::string g_data_dir;  // saves and logs
Lines g_other;           // the file's lines nobody here knows, kept as they were
// Settings the launch chose: their value in the file, kept unless the player
// changes them in the menu (g_edited).
std::map<std::string, std::string> g_launch;
std::set<std::string> g_edited;
// The file's value of every setting as this session started (note_launch).
std::map<std::string, std::string> g_launched;
LaunchChoices g_choices;
bool g_dirty = false;
long long g_dirty_at = 0;
bool g_safe_mode = false;
bool g_safe_backed_up = false;

// The file keeps the folder HD textures were on with while they are off.
constexpr const char* kPackFolderKey = "BLUEWAKE_TEXTURE_PACK_FOLDER";

long long now_ms() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

// A variable for this session ("" unsets it on Windows and leaves it empty
// elsewhere; either reads as unset).
void put_env(const char* key, const std::string& value) {
#if defined(_WIN32)
    _putenv_s(key, value.c_str());
#else
    setenv(key, value.c_str(), 1);
#endif
}

std::string trim(std::string text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return "";
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

bool read_file(const std::string& path, Lines* lines) {
    if (path.empty())
        return false;
    FILE* file = std::fopen(path.c_str(), "r");
    if (file == nullptr)
        return false;
    char line[4096];
    while (std::fgets(line, sizeof line, file) != nullptr) {
        const std::string text = trim(line);
        const auto equals = text.find('=');
        if (text.empty() || text[0] == '#' || equals == std::string::npos || equals == 0)
            continue;
        lines->emplace_back(trim(text.substr(0, equals)), trim(text.substr(equals + 1)));
    }
    std::fclose(file);
    return true;
}

// One line of the file into this session: its variable, unless the launch
// already chose that setting. A key this platform doesn't have (another
// platform's, or one this build doesn't know) is kept as it was.
void read_line(const std::string& key, const std::string& value) {
    if (key == kPackFolderKey) {
        g_choices.last_pack = value;
        return;
    }
    // The pack the file names is also the one to come back to: a safe launch
    // turns HD textures off for this session, and the folder is kept.
    if (key == "DOL_AURORA_TEXTURE_PACK" && !value.empty())
        g_choices.last_pack = value;
    const bw_settings::Setting* row = bw_settings::find(key.c_str());
    const bool known = row != nullptr && bw_settings::exists_on(*row);
    const auto launched = g_launch.find(key);
    if (launched != g_launch.end())
        launched->second = value;  // for this session the launch wins; the file keeps this
    else if (known || std::getenv(key.c_str()) == nullptr)
        put_env(key.c_str(), value);
    if (!known)
        g_other.emplace_back(key, value);
}

// Better Wind Waker's options as the file keeps them: "none" and the ones on.
std::string options_value() {
    std::string list = "none";
    for (const auto& [name, on] : g_choices.options)
        if (on)
            list += "," + name;
    return list;
}

// What the file keeps for one setting: the menu's own copy for the next-launch
// choices the session has already read, otherwise this session's variable, and
// the file's own value for a setting the launch chose and nobody changed here.
std::string file_value(const char* key) {
    std::string value;
    if (std::strcmp(key, "BLUEWAKE_ASPECT") == 0)
        value = g_choices.aspect;
    else if (std::strcmp(key, "BLUEWAKE_MODS") == 0)
        value = g_choices.betterww ? "betterww" : "";
    else if (std::strcmp(key, "BLUEWAKE_OPTIONS") == 0)
        value = g_choices.options.empty() ? env(key) : options_value();
    else if (std::strcmp(key, "DOL_AURORA_TEXTURE_PACK") == 0)
        value = g_choices.texture_pack;
    else if (std::strcmp(key, kPackFolderKey) == 0)
        value = g_choices.texture_pack.empty() ? g_choices.last_pack : "";
    else
        value = env(key);
    const auto launched = g_launch.find(key);
    if (launched != g_launch.end() && g_edited.count(key) == 0)
        value = launched->second;
    return value;
}

}  // namespace

// --- the settings, as variables ---------------------------------------------

std::string env(const char* key, const char* fallback) {
    const char* value = std::getenv(key);
    return value != nullptr ? value : fallback;
}

bool env_on(const char* key, bool fallback) {
    const char* value = std::getenv(key);
    if (value == nullptr || value[0] == '\0')
        return fallback;
    return value[0] != '0';
}

int env_int(const char* key, int fallback) {
    const char* value = std::getenv(key);
    return value != nullptr && value[0] != '\0' ? std::atoi(value) : fallback;
}

void set_env(const char* key, const std::string& value) {
    put_env(key, value);
    g_edited.insert(key);
    mark_dirty();
}

void env_default(const char* key, const std::string& value) {
    const char* now = std::getenv(key);
    if (now == nullptr || now[0] == '\0')
        put_env(key, value);
}

void mark_dirty() {
    g_dirty = true;
    g_dirty_at = now_ms();
}

bool unsaved() { return g_dirty; }
bool save_due() { return g_dirty && now_ms() - g_dirty_at > 1000; }

// --- the next-launch choices ---------------------------------------------------

LaunchChoices& launch_choices() { return g_choices; }

void capture_launch_choices() {
    static bool captured = false;
    if (!captured) {
        captured = true;
        g_choices.aspect = env("BLUEWAKE_ASPECT", "4:3");
        g_choices.betterww = env("BLUEWAKE_MODS").find("betterww") != std::string::npos;
        g_choices.texture_pack = env("DOL_AURORA_TEXTURE_PACK");
        if (!g_choices.texture_pack.empty())
            g_choices.last_pack = g_choices.texture_pack;
        g_launched["BLUEWAKE_ASPECT"] = g_choices.aspect;
        g_launched["BLUEWAKE_MODS"] = g_choices.betterww ? "betterww" : "";
        g_launched["DOL_AURORA_TEXTURE_PACK"] = g_choices.texture_pack;
    }
    // Better Wind Waker's options are listed once the game has started.
    if (!g_choices.options.empty())
        return;
    g_choices.option_titles.clear();
    for (uint32_t i = 0;; ++i) {
        const char* title = nullptr;
        bool default_on = false, on = false;
        const char* name = bluewake_game_options_describe(i, &title, &default_on, &on);
        if (name == nullptr)
            break;
        g_choices.options.emplace_back(name, g_choices.betterww ? on : default_on);
        g_choices.option_titles.emplace_back(title != nullptr ? title : name);
    }
    if (!g_choices.options.empty())
        g_launched["BLUEWAKE_OPTIONS"] = options_value();
}

void choice_changed(const char* key) {
    g_edited.insert(key);
    mark_dirty();
}

void set_texture_pack(const std::string& folder) {
    if (!g_choices.texture_pack.empty())
        g_choices.last_pack = g_choices.texture_pack;
    g_choices.texture_pack = folder;
    choice_changed("DOL_AURORA_TEXTURE_PACK");
}

// --- what a restart would change ---------------------------------------------------

bool next_launch(const bw_settings::Setting& s) { return bw_settings::next_launch_on(s); }

void note_launch() {
    for (const bw_settings::Setting& s : bw_settings::kSettings)
        g_launched[s.key] = file_value(s.key);
}

bool needs_restart() {
    for (const bw_settings::Setting& s : bw_settings::kSettings) {
        if (!next_launch(s))
            continue;
        const auto was = g_launched.find(s.key);
        if (was != g_launched.end() && was->second != file_value(s.key))
            return true;
    }
    return false;
}

// --- the file ---------------------------------------------------------------

namespace {
// A switch that used to be two settings, read as one where the table says
// (settings_table.h's joined and whole), as Windows always held them: Fast
// transitions decides and its fades follow it; Jump & Sprint is on with either
// half on, at the sprint speed chosen. A half the launch chose stays as it is.
void read_joined() {
    for (const bw_settings::Setting& s : bw_settings::kSettings) {
        if (s.joined == nullptr || !bw_settings::includes(s.whole, bw_settings::kThisOs))
            continue;
        const bool launched_half = g_launch.count(s.joined) != 0;
        if (std::strcmp(s.key, "BLUEWAKE_FAST_FORWARD") == 0) {
            if (!launched_half)
                put_env(s.joined, env_on(s.key, bw_settings::default_on(s)) ? "6" : "0");
        } else if (std::strcmp(s.key, "BLUEWAKE_JUMP_BUTTON") == 0) {
            const double speed = std::atof(env(s.joined, "1").c_str());
            const bool on = env_on(s.key, bw_settings::default_on(s)) || speed > 1.01;
            double choice = std::atof(env("BLUEWAKE_SPRINT_CHOICE", "0").c_str());
            if (choice < 1.1 || choice > 2.0)
                choice = speed > 1.01 && speed <= 2.0 ? std::max(speed, 1.1) : 1.5;
            char text[16];
            std::snprintf(text, sizeof text, "%.2f", choice);
            if (g_launch.count(s.key) == 0)
                put_env(s.key, on ? "1" : "0");
            if (!launched_half)
                put_env(s.joined, on ? text : "1");
        }
    }
}
}  // namespace

void load(const std::string& path, const std::string& read_instead) {
    g_path = path;
    for (const bw_settings::Setting& s : bw_settings::kSettings)
        if (std::getenv(s.key) != nullptr)
            g_launch[s.key] = "";
    Lines lines;
    const bool found = read_file(path, &lines) || read_file(read_instead, &lines);
    if (!found) {
        if (!path.empty())
            std::fprintf(stderr, "[settings] no settings file yet (%s)\n", path.c_str());
    } else {
        // A Windows file from before the menus were one is read through the
        // table's old keys (old_windows_settings.h).
        const bool converted = bw_settings::convert_old_windows(lines, bw_platform::default_pack_folder());
        for (const auto& [key, value] : lines)
            read_line(key, value);
        std::fprintf(stderr, "[settings] %d settings from %s\n", static_cast<int>(lines.size()), path.c_str());
        if (converted) {
            // The old file is kept beside the new one, once; if it can't be
            // copied, it is left as it is and read the old way next time.
            const std::string backup = path + ".before-shared-menu";
            if (bw_atomic_copy_if_missing(path.c_str(), backup.c_str())) {
                std::fprintf(stderr, "[settings] the settings file is in the shared format now; the old one is %s\n",
                             backup.c_str());
                g_dirty = true;
                save();
            } else {
                std::fprintf(stderr, "[settings] kept the old settings file as it was (could not copy it to %s)\n",
                             backup.c_str());
            }
        }
    }
    read_joined();
    if (!g_launch.empty()) {
        std::string names;
        for (const auto& [key, value] : g_launch)
            names += (names.empty() ? "" : ", ") + key;
        std::fprintf(stderr, "[settings] for this session, as launched: %s\n", names.c_str());
    }
}

void fill_defaults() {
    for (const bw_settings::Setting& s : bw_settings::kSettings) {
        if (!bw_settings::exists_on(s) || !bw_settings::filled_on(s))
            continue;
        // The frames a fade skips have no default of their own: they follow
        // Fast transitions, as the menu sets them (desktop_menu.cpp, apply_now).
        if (std::strcmp(s.key, "BLUEWAKE_FADE_FRAMES") == 0) {
            const bw_settings::Setting* fast = bw_settings::find("BLUEWAKE_FAST_FORWARD");
            env_default(s.key, env_on(fast->key, bw_settings::default_on(*fast)) ? "6" : "0");
        } else if (bw_settings::fallback_for(s)[0] != '\0') {
            env_default(s.key, bw_settings::fallback_for(s));
        }
    }
}

void launch_choice(const char* key, const std::string& value) {
    if (g_launch.count(key) == 0)
        g_launch[key] = env(key);
    put_env(key, value);
}

void apply_safe_mode() {
    g_safe_mode = true;
    g_safe_backed_up = !g_path.empty() && bw_launch_backup(g_path.c_str());
    if (!g_safe_backed_up)
        std::fprintf(stderr, g_path.empty() ? "[safe-mode] no settings file; only this session is reset\n"
                                            : "[safe-mode] settings backup failed; only this session is reset\n");
    static const char* const kSafe[][2] = {
        {"BLUEWAKE_DSP_MODE", "hle"},        {"BLUEWAKE_MODS", "none"},
        {"BLUEWAKE_OPTIONS", "none"},        {"BLUEWAKE_ASPECT", "4:3"},
        {"DOL_AURORA_ASPECT_FIT", ""},       {"DOL_AURORA_TEXTURE_PACK", ""},
        {"DOL_AURORA_FULLSCREEN", "0"},      {"BLUEWAKE_JUMP_BUTTON", "0"},
        {"BLUEWAKE_SPRINT_SPEED", "1"},      {"BLUEWAKE_FAST_FORWARD", "0"},
        {"BLUEWAKE_FADE_FRAMES", "0"},       {"BLUEWAKE_QUICK_DOORS", "0"},
        {"BLUEWAKE_STAGE_SELECT_TOGGLE", "0"},
        {"BLUEWAKE_HD_LOOK", "0"},
        // Clear overrides so the HD switch can enable every effect again later.
        {"BLUEWAKE_HD_DISTANT_ISLANDS", ""}, {"BLUEWAKE_HD_BLOOM", ""},
        {"BLUEWAKE_HD_FAR_BLUR", ""},        {"BLUEWAKE_HD_AO", ""},
        {"BLUEWAKE_HD_CLOUD_SHADOWS", ""},   {"BLUEWAKE_HD_SUN_SHADOWS", ""},
        {"BLUEWAKE_HD_EDGE_LIGHT", ""},      {"BLUEWAKE_HD_TORCH_LIGHT", ""},
    };
    for (const auto& pair : kSafe) {
        if (g_safe_backed_up) {
            // Saved as safe, even over a launch choice; per-effect overrides,
            // otherwise kept verbatim as unmanaged keys, go.
            g_edited.insert(pair[0]);
            g_other.erase(std::remove_if(g_other.begin(), g_other.end(),
                                         [&pair](const std::pair<std::string, std::string>& line) {
                                             return line.first == pair[0];
                                         }),
                          g_other.end());
        } else if (bw_settings::find(pair[0]) != nullptr && g_launch.count(pair[0]) == 0) {
            // Not copied aside: safe for this session only, as a launch choice
            // is, so a later save keeps the file's own value.
            g_launch[pair[0]] = env(pair[0]);
        }
        put_env(pair[0], pair[1]);
    }
    std::fprintf(stderr, "[safe-mode] Fast sound, 4:3, a window, no HD look, mods or options%s\n",
                 g_safe_backed_up ? "; the earlier settings were copied beside settings.ini" : "");
    if (g_safe_backed_up) {
        g_dirty = true;
        save();
    }
}

bool safe_mode() { return g_safe_mode; }
bool safe_backed_up() { return g_safe_backed_up; }
const std::string& settings_path() { return g_path; }
void set_data_dir(const std::string& dir) { g_data_dir = dir; }
const std::string& data_dir() { return g_data_dir; }

void save() {
    if (g_path.empty() || !g_dirty)
        return;
    g_dirty_at = now_ms(); // back off after a failure rather than retry every frame
    // The aspect, the mods and the texture pack are written from the menu's
    // copies, read when it first opens: a key (F2, F9, F10) that saves before
    // then would otherwise write them empty.
    capture_launch_choices();
    std::error_code error;
    const std::filesystem::path folder = std::filesystem::path(g_path).parent_path();
    if (!folder.empty())
        std::filesystem::create_directories(folder, error);
    char* pending = bw_atomic_path(g_path.c_str());
    FILE* file = pending != nullptr ? std::fopen(pending, "w") : nullptr;
    if (file == nullptr) {
        std::fprintf(stderr, "[settings] cannot write %s\n", g_path.c_str());
        std::free(pending);
        return;
    }
    std::fputs("# BlueWake settings, written by the options menu (F1 or Esc in the game).\n"
               "# KEY=VALUE, the host's settings; a launch option or a variable set before\n"
               "# BlueWake starts wins for that session.\n",
               file);
    for (const bw_settings::Setting& s : bw_settings::kSettings)
        if (bw_settings::exists_on(s))
            std::fprintf(file, "%s=%s\n", s.key, file_value(s.key).c_str());
    for (const auto& [key, value] : g_other)
        std::fprintf(file, "%s=%s\n", key.c_str(), value.c_str());
    const bool ok = bw_atomic_finish_dirty(file, pending, g_path.c_str(), &g_dirty);
    std::free(pending);
    std::fprintf(stderr, "[settings] %s %s\n", ok ? "saved" : "save failed; retry pending for", g_path.c_str());
}

}  // namespace bw_desktop
