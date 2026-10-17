// SPDX-License-Identifier: GPL-3.0-or-later
// Every desktop setting once, and everything a platform does differently with
// it. Each row is the host's environment name (what the game reads, and the
// settings file's key), the key the old Windows settings file used, its kind,
// its default, its range, its label and tab, how the menu draws it, and sets
// of platforms: where a switch is on by default, where it takes effect only at
// the next launch, where it exists at all, and where its default is put in the
// environment at launch. A feature is off by default until it has been tested
// on a platform (AGENTS.md); turning it on there is adding that platform to
// its row's `on`.
// The order is the menu's and the settings file's. The menu draws its plain
// switches, sliders and lists from it; settings with a control of their own
// (Smooth Motion's rate, Jump & Sprint, the texture pack, the remaps, Better
// Wind Waker's options) take their names and defaults from it. No ImGui here,
// so the host tests can read it.
#pragma once
#include <cstring>

namespace bw_settings {

enum class Kind { Bool, Int, Float, Choice, Text };
// How a tab draws the setting. Switch, Slider and Dropdown are drawn by the
// shared draw_tab; Custom names a drawing function of its own (Smooth Motion's
// rate, the texture pack, the remaps, Better Wind Waker's options, the
// keyboard and controller tables); None is not drawn on its own at all.
enum class Control { None, Switch, Slider, Dropdown, Custom };
enum class Tab { Display, Gameplay, Mods, Controls, SoundFiles };

// A set of platforms, a bit each.
enum class Os : unsigned { None = 0, Windows = 1, Linux = 2, Mac = 4, All = 7 };
constexpr Os operator|(Os a, Os b) { return static_cast<Os>(static_cast<unsigned>(a) | static_cast<unsigned>(b)); }
constexpr bool includes(Os set, Os os) { return (static_cast<unsigned>(set) & static_cast<unsigned>(os)) != 0; }

// The platform this is built for. BW_SETTINGS_OS (Os::Windows, ...) sets
// another, for the host tests: one platform's rules, compiled on another.
#if defined(BW_SETTINGS_OS)
inline constexpr Os kThisOs = BW_SETTINGS_OS;
#elif defined(_WIN32)
inline constexpr Os kThisOs = Os::Windows;
#elif defined(__APPLE__)
inline constexpr Os kThisOs = Os::Mac;
#else
inline constexpr Os kThisOs = Os::Linux;
#endif

struct Setting {
    const char* key = "";                // environment variable and settings file key
    const char* old_windows = nullptr;   // the old Windows settings file's key, nullptr if none
    Kind kind = Kind::Text;
    const char* fallback = "";           // the default ("" for none); a switch's is `on`
    Os on = Os::None;                    // a switch (Kind::Bool): where it is on by default
    float low = 0.f, high = 0.f;         // Int and Float: the menu's range
    const char* label = nullptr;         // nullptr: drawn as part of another setting's control
    Tab tab = Tab::Display;
    Control control = Control::None;     // how the shared draw_tab draws it; table order is display order
    const char* note = nullptr;          // the quiet line under it
    const char* needs = nullptr;         // a Bool setting that must be on for it to be changed
    const char* group = nullptr;         // a heading drawn before it, "" for a plain line
    const char* shown = nullptr;         // Slider: how its value reads ("%.0f seconds"), nullptr for "%.2f"
    const char* stored = nullptr;        // Slider: how it is written to the file, nullptr for "%.3f"
    const char* dead_when = nullptr;     // greyed while this setting has a value
    Os next_launch = Os::None;           // where it takes effect only when BlueWake starts again
    Os platforms = Os::All;              // where it exists (the file keeps it, the menu draws it)
    // Where the default is put in the environment at launch, so the game reads
    // it rather than its own: Windows fills every default, as its settings
    // always have; Mac and Linux only where the game's own differs.
    Os fill = Os::Windows;
    // Experimental: drawn at the bottom of the Mods tab under "Experimental",
    // whatever its tab, with this line saying what has not been checked yet.
    // Off by default. Leaving Experimental is removing this line.
    const char* experimental = nullptr;
};

// clang-format off
inline constexpr Setting kSettings[] = {
    {.key = "DOL_AURORA_TEXTURE_PACK", .old_windows = "texture_pack", .label = "HD textures", .tab = Tab::Mods,
     .control = Control::Custom, .next_launch = Os::All},
    {.key = "BLUEWAKE_ASPECT", .old_windows = "aspect", .kind = Kind::Choice, .fallback = "4:3",
     .label = "Widescreen", .tab = Tab::Mods, .control = Control::Custom, .next_launch = Os::All},
    {.key = "BLUEWAKE_HD_LOOK", .old_windows = "hd_look", .kind = Kind::Bool, .label = "Wind Waker HD look   (F2)",
     .control = Control::Switch,
     .note = "The HD release's look, rebuilt piece by piece. So far: islands on the horizon from further away, soft shading, cloud shadows and edge light outdoors, torch light, a soft distance and bloom. Off by default."},
    // Custom: the button that goes fullscreen now is drawn above the switch.
    {.key = "DOL_AURORA_FULLSCREEN", .old_windows = "fullscreen", .kind = Kind::Bool,
     .label = "Start in fullscreen", .control = Control::Custom, .next_launch = Os::All},
    {.key = "DOL_AURORA_RENDER_SCALE", .old_windows = "render_scale", .kind = Kind::Int, .fallback = "0",
     .high = 4.f, .label = "Render resolution", .control = Control::Dropdown},
    {.key = "DOL_AURORA_FRAME_INTERP", .old_windows = "smooth_motion", .kind = Kind::Bool,
     .label = "Smooth Motion (experimental)   (F10)", .control = Control::Custom},
    {.key = "DOL_AURORA_FRAME_INTERP_STEPS", .old_windows = "smooth_motion_fps", .kind = Kind::Choice,
     .fallback = "1"},
    {.key = "DOL_AURORA_SHOW_FPS", .old_windows = "show_fps", .kind = Kind::Bool,
     .label = "Show the frame rate   (F9)", .control = Control::Switch},
    {.key = "DOL_AURORA_FPS_POSITION", .old_windows = "fps_position", .kind = Kind::Choice,
     .label = "FPS position", .control = Control::Dropdown, .needs = "DOL_AURORA_SHOW_FPS"},
    {.key = "DOL_AURORA_FORCE_ANISO", .old_windows = "anisotropy", .kind = Kind::Int, .fallback = "1", .low = 1.f,
     .high = 16.f, .label = "Texture filtering", .control = Control::Dropdown, .fill = Os::None},
    {.key = "BLUEWAKE_MODS", .old_windows = "betterww", .label = "Better Wind Waker", .tab = Tab::Mods,
     .control = Control::Custom, .next_launch = Os::All},
    {.key = "BLUEWAKE_OPTIONS", .tab = Tab::Mods, .next_launch = Os::All}, // Windows: option.NAME
    {.key = "BLUEWAKE_STAGE_SELECT_TOGGLE", .old_windows = "stage_select", .kind = Kind::Bool,
     .label = "Developers' stage select (F7)", .tab = Tab::Gameplay, .control = Control::Custom,
     .note = "F7 during play opens the menu the developers used to jump to any room, from your own file; F7 in the menu goes back. On a controller, Back (View on Xbox, Share or Create on PlayStation) opens these settings and the button below does the same. Off by default.",
     .next_launch = Os::All},
    {.key = "BLUEWAKE_FOREST_WATER_KEEP_TREES", .old_windows = "forest_water_keep_trees", .kind = Kind::Bool,
     .label = "Keep watered trees when time runs out", .tab = Tab::Gameplay, .control = Control::Switch,
     .note = "Forest Water still expires. Refill and continue with the remaining trees.",
     .group = "Forest Water Challenge"},
    {.key = "BLUEWAKE_FOREST_WATER_30_MINUTES", .old_windows = "forest_water_30_minutes", .kind = Kind::Bool,
     .label = "30-minute Forest Water timer", .tab = Tab::Gameplay, .control = Control::Switch,
     .note = "Applies the next time you collect Forest Water. Both options are off by default."},
    // Fast transitions and Jump & Sprint were two settings each (the fades, the
    // sprint's speed). A file holding one half on loads with both off, on every
    // platform (desktop_settings.cpp); the menu's switch sets both halves.
    {.key = "BLUEWAKE_FADE_FRAMES", .old_windows = "fast_transitions", .kind = Kind::Int, .tab = Tab::Gameplay},
    {.key = "BLUEWAKE_FAST_FORWARD", .old_windows = "fast_transitions", .kind = Kind::Bool,
     .on = Os::Linux | Os::Mac, .label = "Fast transitions (short fades, no waiting in the black)",
     .tab = Tab::Gameplay, .control = Control::Switch, .group = "Doors and exits", .next_launch = Os::Windows},
    {.key = "BLUEWAKE_QUICK_DOORS", .old_windows = "quick_doors", .kind = Kind::Bool, .on = Os::Linux | Os::Mac,
     .label = "Quick doors (no walk-in or door closing behind Link)", .tab = Tab::Gameplay,
     .control = Control::Switch, .next_launch = Os::Windows},
    {.key = "BLUEWAKE_CLIMB", .old_windows = "climb", .kind = Kind::Bool, .label = "Wall climbing (experimental)",
     .tab = Tab::Gameplay, .control = Control::Switch, .group = "Climbing"},
    {.key = "BLUEWAKE_CLIMB_STAMINA", .old_windows = "climb_stamina", .kind = Kind::Float, .fallback = "12",
     .low = 4.f, .high = 30.f, .label = "Climbing stamina", .tab = Tab::Gameplay, .control = Control::Slider,
     .needs = "BLUEWAKE_CLIMB", .shown = "%.0f seconds", .stored = "%.0f"},
    {.key = "BLUEWAKE_JUMP_BUTTON", .old_windows = "movement_extras", .kind = Kind::Bool,
     .label = "Jump & Sprint (Space or the left bumper jumps; Shift or the left stick click sprints)",
     .tab = Tab::Gameplay, .control = Control::Custom, .group = "Jump & Sprint", .next_launch = Os::Windows,
     .fill = Os::All},
    {.key = "BLUEWAKE_SPRINT_SPEED", .old_windows = "sprint_speed", .kind = Kind::Float, .fallback = "1",
     .tab = Tab::Gameplay, .next_launch = Os::Windows, .fill = Os::All},
    {.key = "BLUEWAKE_SPRINT_CHOICE", .old_windows = "sprint_speed", .kind = Kind::Float, .fallback = "1.5",
     .low = 1.1f, .high = 2.f, .label = "Sprint speed", .tab = Tab::Gameplay, .needs = "BLUEWAKE_JUMP_BUTTON",
     .shown = "%.2fx", .stored = "%.2f"},
    {.key = "BLUEWAKE_PAD_SWAP_AB", .old_windows = "controller_swap_ab", .kind = Kind::Bool,
     .label = "Swap A and B", .tab = Tab::Controls, .control = Control::Switch, .group = "Controller",
     .dead_when = "BLUEWAKE_BUTTON_MAP"},
    {.key = "BLUEWAKE_PAD_SWAP_XY", .old_windows = "controller_swap_xy", .kind = Kind::Bool,
     .label = "Swap X and Y", .tab = Tab::Controls, .control = Control::Switch,
     .dead_when = "BLUEWAKE_BUTTON_MAP"},
    {.key = "BLUEWAKE_PAD_SWAP_LR", .old_windows = "controller_swap_lr", .kind = Kind::Bool,
     .label = "Swap L and R (the triggers)", .tab = Tab::Controls, .control = Control::Switch,
     .dead_when = "BLUEWAKE_BUTTON_MAP"},
    {.key = "BLUEWAKE_DPAD_MENUS", .old_windows = "dpad_menus", .kind = Kind::Bool,
     .label = "D-pad moves the cursor in Items and Quest Status", .tab = Tab::Controls,
     .control = Control::Switch},
    {.key = "BLUEWAKE_BUTTON_MAP", .old_windows = "button_map", .label = "Controller buttons",
     .tab = Tab::Controls, .control = Control::Custom},
    {.key = "BLUEWAKE_KEY_MAP", .old_windows = "key_map", .label = "Keyboard keys", .tab = Tab::Controls,
     .control = Control::Custom},
    {.key = "BLUEWAKE_MOUSE_CAMERA", .old_windows = "mouse_camera", .kind = Kind::Bool, .on = Os::All,
     .label = "Mouse camera (click the game to use it)", .tab = Tab::Controls, .control = Control::Switch},
    {.key = "BLUEWAKE_MOUSE_SENSITIVITY", .old_windows = "mouse_sensitivity", .kind = Kind::Float,
     .fallback = "1.0", .low = 0.2f, .high = 3.f, .label = "Mouse sensitivity", .tab = Tab::Controls,
     .control = Control::Slider, .needs = "BLUEWAKE_MOUSE_CAMERA", .shown = "%.2fx", .stored = "%.2f"},
    {.key = "BLUEWAKE_MOUSE_INVERT_Y", .old_windows = "mouse_invert_y", .kind = Kind::Bool,
     .label = "Invert the mouse's up and down", .tab = Tab::Controls, .control = Control::Switch,
     .needs = "BLUEWAKE_MOUSE_CAMERA"},
    {.key = "BLUEWAKE_MOUSE_BUTTONS", .old_windows = "mouse_buttons", .label = "Mouse buttons",
     .tab = Tab::Controls, .control = Control::Custom, .needs = "BLUEWAKE_MOUSE_CAMERA"},
    {.key = "BLUEWAKE_STICK_CAMERA", .old_windows = "stick_camera", .kind = Kind::Bool, .on = Os::All,
     .label = "Direct right-stick camera and aiming", .tab = Tab::Controls, .control = Control::Switch,
     .group = ""},
    {.key = "BLUEWAKE_STICK_CAMERA_SPEED", .old_windows = "stick_camera_speed", .kind = Kind::Float,
     .fallback = "360", .low = 120.f, .high = 720.f, .label = "Right-stick turn speed", .tab = Tab::Controls,
     .control = Control::Slider, .needs = "BLUEWAKE_STICK_CAMERA", .shown = "%.0f degrees a second",
     .stored = "%.0f"},
    {.key = "BLUEWAKE_STICK_AIM_SPEED", .old_windows = "stick_aim_speed", .kind = Kind::Float, .fallback = "180",
     .low = 60.f, .high = 480.f, .label = "Right-stick aim speed (first person, items)", .tab = Tab::Controls,
     .control = Control::Slider, .needs = "BLUEWAKE_STICK_CAMERA", .shown = "%.0f degrees a second",
     .stored = "%.0f"},
    {.key = "BLUEWAKE_STICK_ZOOM", .old_windows = "stick_zoom", .kind = Kind::Bool,
     .label = "Hold the right-stick click and move it up/down to zoom", .tab = Tab::Controls,
     .control = Control::Switch},
    {.key = "BLUEWAKE_STICK_ZOOM_SPEED", .old_windows = "stick_zoom_speed", .kind = Kind::Float,
     .fallback = "1.275", .low = 0.25f, .high = 2.f, .label = "Controller zoom speed", .tab = Tab::Controls,
     .control = Control::Slider, .needs = "BLUEWAKE_STICK_ZOOM", .shown = "%.2fx", .stored = "%.3f"},
    // The inverts are not greyed out with the direct camera off. Whether they then
    // reach the game's own camera still differs (desktop_menu.cpp, apply_controller).
    {.key = "BLUEWAKE_STICK_CAMERA_INVERT_X", .old_windows = "controller_invert_x", .kind = Kind::Bool,
     .label = "Invert the right stick's left and right", .tab = Tab::Controls, .control = Control::Switch},
    {.key = "BLUEWAKE_STICK_CAMERA_INVERT_Y", .old_windows = "controller_invert_y", .kind = Kind::Bool,
     .label = "Invert the right stick's up and down", .tab = Tab::Controls, .control = Control::Switch},
    {.key = "BLUEWAKE_AIM_INVERT_Y", .old_windows = "aim_invert_y", .kind = Kind::Bool,
     .label = "Invert the left stick's up and down when aiming (first person, items)", .tab = Tab::Controls,
     .control = Control::Switch},
    {.key = "BLUEWAKE_GYRO_AIM", .old_windows = "gyro_aim", .kind = Kind::Bool,
     .label = "Gyro aiming: move the controller to aim (DualSense, DualShock 4, Switch Pro, Steam Deck)",
     .tab = Tab::Controls, .control = Control::Switch},
    {.key = "BLUEWAKE_GYRO_AIM_SPEED", .old_windows = "gyro_aim_speed", .kind = Kind::Float, .fallback = "1",
     .low = 0.25f, .high = 4.f, .label = "Gyro aim speed", .tab = Tab::Controls, .control = Control::Custom,
     .needs = "BLUEWAKE_GYRO_AIM", .shown = "%.2fx", .stored = "%.2f"},
    {.key = "BLUEWAKE_HAPTICS", .old_windows = "haptics", .kind = Kind::Choice, .fallback = "enhanced",
     .label = "Controller vibration", .tab = Tab::Controls, .control = Control::Custom, .group = "Haptics"},
    {.key = "BLUEWAKE_HAPTICS_STRENGTH", .old_windows = "haptics_strength", .kind = Kind::Int, .fallback = "80",
     .high = 100.f, .label = "Vibration strength", .tab = Tab::Controls},
    {.key = "BLUEWAKE_HAPTICS_TRIGGERS", .old_windows = "haptics_triggers", .kind = Kind::Bool, .on = Os::All,
     .label = "Trigger feedback (Xbox impulse triggers, DualSense trigger vibration)", .tab = Tab::Controls},
    {.key = "DOL_AURORA_ASPECT_FIT", .old_windows = "keep_aspect", .kind = Kind::Bool, .on = Os::Windows,
     .label = "Keep the picture's shape (black bars rather than stretching)", .control = Control::Custom,
     .next_launch = Os::All},
    // Experimental (the Mods tab's last section).
    {.key = "BLUEWAKE_PICTO_CROP", .old_windows = "picto_crop", .kind = Kind::Bool,
     .label = "Picto Box photos keep their 4:3 shape in widescreen", .tab = Tab::Mods,
     .control = Control::Switch, .next_launch = Os::All,
     .experimental = "Checked on Windows (Direct3D 12) and Linux (Vulkan) at 16:9, 16:10 and 21:9; not yet on a Mac."},
    {.key = "BLUEWAKE_LOOSE_REL_RETRACES", .kind = Kind::Bool,
     .label = "A load time for loose RELs (the stage select's Wind Temple R04 and R08 load)", .tab = Tab::Mods,
     .control = Control::Switch, .next_launch = Os::All,
     .experimental = "Every stage select place checked headless on Windows and Linux; not yet played through, or on a Mac."},
    {.key = "BLUEWAKE_HD_SUN_SHADOWS", .kind = Kind::Bool, .label = "Sun shadows outdoors (from the HD look)",
     .tab = Tab::Mods, .control = Control::Switch,
     .experimental = "Worked out from what is on screen: something off screen casts nothing, and on open ground they can draw long dark streaks."},
    {.key = "BLUEWAKE_PAUSE_UNFOCUSED", .old_windows = "pause_unfocused", .kind = Kind::Bool,
     .label = "Pause while the window is in the background", .control = Control::Switch},
    {.key = "BLUEWAKE_SHADERS_FIRST", .old_windows = "compile_shaders_first", .kind = Kind::Bool,
     .label = "Compile shaders before playing", .control = Control::Switch,
     .note = "At start, wait until the shaders from earlier play are ready, so nothing is missing from the picture the first time it is drawn. Otherwise they are made while you play.",
     .next_launch = Os::All},
    {.key = "BLUEWAKE_DSP_MODE", .old_windows = "lle_audio", .kind = Kind::Choice, .fallback = "hle",
     .tab = Tab::SoundFiles, .control = Control::Custom, .group = "Sound", .next_launch = Os::All,
     .fill = Os::All},
    // Not drawn: the texture pack's folder while HD textures are off, so they come back to it.
    {.key = "BLUEWAKE_TEXTURE_PACK_FOLDER", .tab = Tab::Mods, .fill = Os::None},
    // Windows only, not drawn: the window's size and place as the player left them, checked at launch.
    {.key = "DOL_AURORA_WINDOW", .old_windows = "window", .platforms = Os::Windows, .fill = Os::None},
    {.key = "BLUEWAKE_WINDOW_POSITION", .old_windows = "window_position", .platforms = Os::Windows,
     .fill = Os::None},
};
// clang-format on

inline constexpr int kCount = sizeof kSettings / sizeof kSettings[0];

// The setting named `key`, or nullptr.
inline const Setting* find(const char* key) {
    for (const Setting& setting : kSettings)
        if (std::strcmp(setting.key, key) == 0)
            return &setting;
    return nullptr;
}

// What a row says for one platform: its default (a switch's "1" or "0"),
// whether it waits for the next launch, whether it exists there, and whether
// its default is filled in.
inline const char* fallback_for(const Setting& s, Os os = kThisOs) {
    if (s.kind == Kind::Bool)
        return includes(s.on, os) ? "1" : "0";
    return s.fallback;
}
inline bool next_launch_on(const Setting& s, Os os = kThisOs) { return includes(s.next_launch, os); }
inline bool exists_on(const Setting& s, Os os = kThisOs) { return includes(s.platforms, os); }
inline bool filled_on(const Setting& s, Os os = kThisOs) { return includes(s.fill, os); }

// A Bool setting's default.
inline bool default_on(const Setting& s, Os os = kThisOs) {
    const char* value = fallback_for(s, os);
    return value[0] != '\0' && value[0] != '0';
}

} // namespace bw_settings
