// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <climits>
#include <map>
#include <string>
struct Settings {
    // Display: apply at once.
    bool fullscreen = false;
    int window_w = 0, window_h = 0;  // 0: sized from the screen
    int window_x = INT_MIN, window_y = INT_MIN;
    int render_scale = 0;  // 0: the window's own pixels; 1-4: x 480 lines
    int anisotropy = 1;    // 1: the game's own filtering; 2-16 forced
    bool smooth_motion = false;  // experimental: off unless the player turns it on
    int smooth_steps = 1; // 60 FPS; 3: 120; -1: match the display, up to 240.
    bool show_fps = false;
    bool pause_unfocused = false;
    // At start, the game waits until the saved and bundled pipelines are compiled.
    bool shaders_first = false;
    // Controls: apply at once.
    bool mouse_camera = true;
    double mouse_sensitivity = 1.0;
    bool mouse_invert_y = false;
    bool controller_swap_ab = false, controller_swap_xy = false;
    // A custom controller button layout (button_remap.h); "" is the controller's own.
    std::string button_map;
    // What the mouse buttons press and the GameCube buttons' keys
    // (input_remap.h); "" is the default (left click A; J K U I Q E R Return).
    std::string mouse_buttons, key_map;
    bool pad_invert_x = false, pad_invert_y = false;
    // The fast right-stick camera (mouse_camera.h): the stick turns the view
    // and aims directly, instead of the game's eased C-stick camera.
    bool stick_camera = true;
    int stick_speed = 360;      // degrees a second at full tilt
    int stick_aim_speed = 180;  // the same when aiming
    bool aim_invert_y = false;  // the left stick's up and down the other way when aiming (#154)
    bool climb = false;         // climb any wall on a stamina wheel (climb.h)
    int climb_stamina = 12;     // seconds of climbing on a full wheel
    // The Forest Water challenge (forest_water.h): watered trees kept when the
    // water runs out, and a 30-minute timer for the next scoop. Both off.
    bool forest_keep_trees = false;
    bool forest_30_minutes = false;
    // Elliott Tate's controller feedback: off, classic, enhanced.
    int haptics = 2;
    int haptics_strength = 80;
    bool haptics_triggers = true;
    // At the next launch.
    std::string aspect = "4:3";
    bool keep_aspect = true;
    bool betterww = false;
    std::map<std::string, bool> options;  // only those changed from their default
    bool option_defaults_off = false; // Session-only --options none baseline.
    bool hd_textures = false;
    // The pack's folder when it is not Load\Textures\GZLE01: the Wind Waker HD
    // importer's --install names its own (docs/WWHD_TEXTURES.md).
    std::string texture_pack;
    bool lle_audio = false;
    bool movement_extras = false;
    bool fast_transitions = false;
    bool quick_doors = false;
};

inline bool bw_settings_option_value(const Settings& settings, const std::string& name, bool default_on) {
    auto chosen = settings.options.find(name);
    return chosen != settings.options.end() ? chosen->second : default_on && !settings.option_defaults_off;
}

inline void bw_settings_keep_edits(Settings& saved, const Settings& before, const Settings& session) {
    if (before.fullscreen != session.fullscreen) saved.fullscreen = session.fullscreen;
    if (before.window_w != session.window_w) saved.window_w = session.window_w;
    if (before.window_h != session.window_h) saved.window_h = session.window_h;
    if (before.window_x != session.window_x) saved.window_x = session.window_x;
    if (before.window_y != session.window_y) saved.window_y = session.window_y;
    if (before.render_scale != session.render_scale) saved.render_scale = session.render_scale;
    if (before.anisotropy != session.anisotropy) saved.anisotropy = session.anisotropy;
    if (before.smooth_motion != session.smooth_motion) saved.smooth_motion = session.smooth_motion;
    if (before.smooth_steps != session.smooth_steps) saved.smooth_steps = session.smooth_steps;
    if (before.show_fps != session.show_fps) saved.show_fps = session.show_fps;
    if (before.pause_unfocused != session.pause_unfocused) saved.pause_unfocused = session.pause_unfocused;
    if (before.shaders_first != session.shaders_first) saved.shaders_first = session.shaders_first;
    if (before.mouse_camera != session.mouse_camera) saved.mouse_camera = session.mouse_camera;
    if (before.mouse_sensitivity != session.mouse_sensitivity) saved.mouse_sensitivity = session.mouse_sensitivity;
    if (before.mouse_invert_y != session.mouse_invert_y) saved.mouse_invert_y = session.mouse_invert_y;
    if (before.pad_invert_x != session.pad_invert_x) saved.pad_invert_x = session.pad_invert_x;
    if (before.pad_invert_y != session.pad_invert_y) saved.pad_invert_y = session.pad_invert_y;
    if (before.stick_camera != session.stick_camera) saved.stick_camera = session.stick_camera;
    if (before.stick_speed != session.stick_speed) saved.stick_speed = session.stick_speed;
    if (before.stick_aim_speed != session.stick_aim_speed) saved.stick_aim_speed = session.stick_aim_speed;
    if (before.aim_invert_y != session.aim_invert_y) saved.aim_invert_y = session.aim_invert_y;
    if (before.climb != session.climb) saved.climb = session.climb;
    if (before.climb_stamina != session.climb_stamina) saved.climb_stamina = session.climb_stamina;
    if (before.forest_keep_trees != session.forest_keep_trees) saved.forest_keep_trees = session.forest_keep_trees;
    if (before.forest_30_minutes != session.forest_30_minutes) saved.forest_30_minutes = session.forest_30_minutes;
    if (before.haptics != session.haptics) saved.haptics = session.haptics;
    if (before.haptics_strength != session.haptics_strength) saved.haptics_strength = session.haptics_strength;
    if (before.haptics_triggers != session.haptics_triggers) saved.haptics_triggers = session.haptics_triggers;
    if (before.aspect != session.aspect) saved.aspect = session.aspect;
    if (before.keep_aspect != session.keep_aspect) saved.keep_aspect = session.keep_aspect;
    if (before.betterww != session.betterww) saved.betterww = session.betterww;
    if (before.hd_textures != session.hd_textures) saved.hd_textures = session.hd_textures;
    if (before.texture_pack != session.texture_pack) saved.texture_pack = session.texture_pack;
    if (before.lle_audio != session.lle_audio) saved.lle_audio = session.lle_audio;
    if (before.movement_extras != session.movement_extras) saved.movement_extras = session.movement_extras;
    if (before.fast_transitions != session.fast_transitions) saved.fast_transitions = session.fast_transitions;
    if (before.quick_doors != session.quick_doors) saved.quick_doors = session.quick_doors;
    if (before.controller_swap_ab != session.controller_swap_ab) saved.controller_swap_ab = session.controller_swap_ab;
    if (before.controller_swap_xy != session.controller_swap_xy) saved.controller_swap_xy = session.controller_swap_xy;
    if (before.button_map != session.button_map) saved.button_map = session.button_map;
    if (before.mouse_buttons != session.mouse_buttons) saved.mouse_buttons = session.mouse_buttons;
    if (before.key_map != session.key_map) saved.key_map = session.key_map;
    for (const auto& [key, value] : session.options) {
        auto old = before.options.find(key);
        if (old == before.options.end() || old->second != value) saved.options[key] = value;
    }
    for (const auto& [key, value] : before.options) {
        if (!session.options.count(key)) saved.options.erase(key);
    }
}
