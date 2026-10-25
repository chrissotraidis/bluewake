#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdlib>
#include <cstddef>
#include <cstring>
#include <set>
#include <string>
#include "../runtime/host/src/settings_table.h"

using namespace bw_settings;

// Every setting once, in the order the table gives: the order the menus draw
// each tab in, and the order the settings file is written in. A key that moves
// changes where its line sits in the file, never what the file says.
static const char* const kFileOrder[] = {
    "DOL_AURORA_TEXTURE_PACK", "BLUEWAKE_ASPECT", "BLUEWAKE_HD_LOOK", "DOL_AURORA_SHOW_FPS",
    "DOL_AURORA_ASPECT_FIT", "BLUEWAKE_PICTO_CROP", "BLUEWAKE_LOOSE_REL_RETRACES", "BLUEWAKE_HD_SUN_SHADOWS",
    "DOL_AURORA_SUN_SHADOW_QUALITY", "BLUEWAKE_PAUSE_UNFOCUSED", "BLUEWAKE_SHADERS_FIRST", "DOL_AURORA_FULLSCREEN", "DOL_AURORA_RENDER_SCALE",
    "DOL_AURORA_FRAME_INTERP", "DOL_AURORA_FRAME_INTERP_STEPS", "DOL_AURORA_FPS_POSITION", "DOL_AURORA_FORCE_ANISO",
    "BLUEWAKE_MODS", "BLUEWAKE_OPTIONS", "BLUEWAKE_STAGE_SELECT_TOGGLE", "BLUEWAKE_FOREST_WATER_KEEP_TREES",
    "BLUEWAKE_FOREST_WATER_30_MINUTES", "BLUEWAKE_FADE_FRAMES", "BLUEWAKE_FAST_FORWARD", "BLUEWAKE_QUICK_DOORS",
    "BLUEWAKE_CLIMB", "BLUEWAKE_CLIMB_STAMINA", "BLUEWAKE_JUMP_BUTTON", "BLUEWAKE_SPRINT_SPEED",
    "BLUEWAKE_SPRINT_CHOICE", "BLUEWAKE_PAD_SWAP_AB", "BLUEWAKE_PAD_SWAP_XY", "BLUEWAKE_PAD_SWAP_LR",
    "BLUEWAKE_DPAD_MENUS", "BLUEWAKE_BACKGROUND_INPUT", "BLUEWAKE_BUTTON_MAP", "BLUEWAKE_KEY_MAP", "BLUEWAKE_MOUSE_CAMERA",
    "BLUEWAKE_MOUSE_SENSITIVITY", "BLUEWAKE_MOUSE_INVERT_Y", "BLUEWAKE_MOUSE_BUTTONS", "BLUEWAKE_STICK_CAMERA",
    "BLUEWAKE_STICK_CAMERA_SPEED", "BLUEWAKE_STICK_AIM", "BLUEWAKE_STICK_AIM_SPEED", "BLUEWAKE_STICK_ZOOM", "BLUEWAKE_STICK_ZOOM_SPEED",
    "BLUEWAKE_STICK_CAMERA_INVERT_X", "BLUEWAKE_STICK_CAMERA_INVERT_Y", "BLUEWAKE_AIM_INVERT_Y",
    "BLUEWAKE_GYRO_AIM", "BLUEWAKE_GYRO_AIM_SPEED", "BLUEWAKE_HAPTICS", "BLUEWAKE_HAPTICS_STRENGTH",
    "BLUEWAKE_HAPTICS_TRIGGERS", "BLUEWAKE_DSP_MODE", "BLUEWAKE_MENU_PAUSES", "BLUEWAKE_MENU_TEXT_SIZE",
    "BLUEWAKE_MENU_CONTRAST", "BLUEWAKE_MENU_PLACE", "BLUEWAKE_MENU_SIZE", "BLUEWAKE_TEXTURE_PACK_FOLDER",
    "DOL_AURORA_WINDOW", "BLUEWAKE_WINDOW_POSITION",
};

// Each platform's rules, as the menus had them in code before the table held
// them (win_settings.cpp's defaults and waits_for_launch, settings_menu.cpp's
// load): a row that changes one of these changes what a player gets.
static const char* const kWindowsOwnDefaults[][2] = {
    {"DOL_AURORA_ASPECT_FIT", "1"}, {"BLUEWAKE_FAST_FORWARD", "0"}, {"BLUEWAKE_QUICK_DOORS", "0"},
    // The game keeps running under the menu on Windows until pausing is checked there.
    {"BLUEWAKE_MENU_PAUSES", "0"}};
static const char* const kWindowsOnlyNextLaunch[] = {"BLUEWAKE_FAST_FORWARD", "BLUEWAKE_QUICK_DOORS",
                                                     "BLUEWAKE_JUMP_BUTTON", "BLUEWAKE_SPRINT_SPEED"};
static const char* const kFilledEverywhere[] = {"BLUEWAKE_DSP_MODE", "BLUEWAKE_JUMP_BUTTON", "BLUEWAKE_SPRINT_SPEED"};
static const char* const kWindowsOnly[] = {"DOL_AURORA_WINDOW", "BLUEWAKE_WINDOW_POSITION"};
static const char* const kNotFilled[] = {"DOL_AURORA_FORCE_ANISO", "BLUEWAKE_MENU_PLACE",
                                         "BLUEWAKE_MENU_SIZE", "BLUEWAKE_TEXTURE_PACK_FOLDER",
                                         "DOL_AURORA_WINDOW", "BLUEWAKE_WINDOW_POSITION"};

template <size_t N> static bool listed(const char* const (&list)[N], const char* key) {
    for (const char* k : list)
        if (std::strcmp(k, key) == 0)
            return true;
    return false;
}

int main() {
    assert(kCount == static_cast<int>(sizeof kFileOrder / sizeof kFileOrder[0]));
    std::set<std::string> keys;
    for (int i = 0; i < kCount; ++i) {
        const Setting& s = kSettings[i];
        assert(std::strcmp(s.key, kFileOrder[i]) == 0);
        assert(keys.insert(s.key).second);
        assert(find(s.key) == &s);
        for (const Os os : {Os::Windows, Os::Linux, Os::Mac}) {
            const char* fallback = fallback_for(s, os);
            if (s.kind == Kind::Bool)
                assert(std::strcmp(fallback, "0") == 0 || std::strcmp(fallback, "1") == 0);
            // A slider's default sits inside its range.
            if ((s.kind == Kind::Int || s.kind == Kind::Float) && s.label != nullptr && s.high > s.low) {
                const float value = std::strtof(fallback, nullptr);
                assert(value >= s.low && value <= s.high);
            }
        }
        // A switch's default is where it is on: it has no other. Windows' own
        // defaults, and only those, differ from Mac's and Linux's.
        if (s.kind == Kind::Bool)
            assert(s.fallback[0] == '\0');
        else
            assert(s.on == Os::None);
        assert(std::strcmp(fallback_for(s, Os::Mac), fallback_for(s, Os::Linux)) == 0);
        bool own = false;
        for (const auto& pair : kWindowsOwnDefaults)
            if (std::strcmp(pair[0], s.key) == 0) {
                own = true;
                assert(std::strcmp(fallback_for(s, Os::Windows), pair[1]) == 0 &&
                       std::strcmp(fallback_for(s, Os::Mac), pair[1]) != 0);
            }
        if (!own)
            assert(std::strcmp(fallback_for(s, Os::Windows), fallback_for(s, Os::Mac)) == 0);
        // The next launch: the same everywhere, but for Windows' four.
        const bool windows_only = listed(kWindowsOnlyNextLaunch, s.key);
        assert(next_launch_on(s, Os::Windows) == (next_launch_on(s, Os::Mac) || windows_only));
        assert(next_launch_on(s, Os::Mac) == next_launch_on(s, Os::Linux));
        if (windows_only)
            assert(!next_launch_on(s, Os::Mac));
        // Where it exists.
        assert(exists_on(s, Os::Windows));
        assert(exists_on(s, Os::Mac) == !listed(kWindowsOnly, s.key) && exists_on(s, Os::Linux) == exists_on(s, Os::Mac));
        // Windows puts every default in the environment at launch; Mac and Linux
        // only the three whose game default differs.
        assert(filled_on(s, Os::Windows) == !listed(kNotFilled, s.key));
        assert(filled_on(s, Os::Mac) == listed(kFilledEverywhere, s.key));
        assert(filled_on(s, Os::Linux) == filled_on(s, Os::Mac));
    }
    assert(find("BLUEWAKE_NOT_A_SETTING") == nullptr);
    assert(default_on(*find("BLUEWAKE_MOUSE_CAMERA")) && !default_on(*find("BLUEWAKE_HD_LOOK")));
    assert(std::strcmp(find("BLUEWAKE_ASPECT")->old_windows, "aspect") == 0);
    return 0;
}
