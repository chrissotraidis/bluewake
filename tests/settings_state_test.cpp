#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include "../windows/src/settings_state.h"
#include "../runtime/host/src/smooth_rate.h"
#include "../runtime/host/src/fps_position.h"
#include "../runtime/host/src/stick_zoom.h"
#include <limits>
int main() {
    Settings saved;
    assert(!saved.smooth_motion && saved.smooth_steps == 1);
    assert(!saved.stick_zoom && saved.stick_zoom_speed == 1.275);
    assert(saved.haptics == 2 && saved.haptics_strength == 80 && saved.haptics_triggers);
    saved.haptics = 0; saved.haptics_strength = 25; saved.haptics_triggers = false;
    saved.options["old"] = true;
    Settings session = saved;
    session.fullscreen = true; session.render_scale = 4; session.lle_audio = true;
    session.smooth_motion = true; session.smooth_steps = -1; session.options["session_only"] = true;
    session.haptics = 2; session.haptics_strength = 90; session.haptics_triggers = true;
    Settings before = session;
    session.show_fps = true;
    bw_settings_keep_edits(saved, before, session);
    assert(saved.show_fps && !saved.fullscreen && saved.render_scale == 0);
    assert(!saved.lle_audio && !saved.smooth_motion && saved.options.count("session_only") == 0);
    assert(saved.smooth_steps == 1); // A session override is not a stored edit.
    assert(saved.haptics == 0 && saved.haptics_strength == 25 && !saved.haptics_triggers);
    before = session;
    session.haptics = 1; session.haptics_strength = 60; session.haptics_triggers = false;
    bw_settings_keep_edits(saved, before, session);
    assert(saved.haptics == 1 && saved.haptics_strength == 60 && !saved.haptics_triggers);
    before = session;
    session.render_scale = 2; session.options["edited"] = false;
    bw_settings_keep_edits(saved, before, session);
    assert(saved.render_scale == 2 && saved.options["edited"] == false);
    assert(saved.options.count("session_only") == 0 && saved.options["old"]);
    before = session;
    session.options.erase("old"); // Player restores this option to its default.
    bw_settings_keep_edits(saved, before, session);
    assert(saved.options.count("old") == 0);
    session.option_defaults_off = true;
    assert(!bw_settings_option_value(session, "default_on", true));
    before = session;
    session.options["default_on"] = true;
    bw_settings_keep_edits(saved, before, session);
    assert(bw_settings_option_value(saved, "default_on", false));
    assert(!saved.option_defaults_off);
    before = session; session.smooth_steps = 3;
    bw_settings_keep_edits(saved, before, session);
    assert(saved.smooth_steps == 3 && !saved.smooth_motion);
    before = session; session.stick_zoom = true; session.stick_zoom_speed = 1.25;
    bw_settings_keep_edits(saved, before, session);
    assert(saved.stick_zoom && saved.stick_zoom_speed == 1.25);
    assert(bw_smooth_requested("display") == -1 && bw_smooth_requested("3") == 3);
    assert(bw_smooth_requested(nullptr) == 1 && bw_smooth_requested("invalid") == 1);
    const float rates[] = {59.94f, 60.f, 90.f, 100.f, 119.88f, 120.f, 144.f, 165.f, 240.f, 360.f};
    const int steps[] = {1, 1, 2, 2, 3, 3, 3, 4, 7, 7};
    for (int i = 0; i < 10; ++i) {
        assert(bw_smooth_steps(-1, rates[i]) == steps[i]);
        assert(bw_smooth_steps(1, rates[i]) == 1);
        assert(bw_smooth_steps(3, rates[i]) == (rates[i] >= 119.f ? 3 : 1));
    }
    assert(bw_smooth_steps(-1, 0) == 1 && bw_smooth_steps(-1, -1) == 1);
    assert(bw_smooth_steps(-1, std::numeric_limits<float>::infinity()) == 1);
    assert(bw_smooth_steps(-1, std::numeric_limits<float>::quiet_NaN()) == 1);
    // Falling back on a slower/unknown display never edits the preference.
    assert(bw_smooth_steps(saved.smooth_steps, 60) == 1 && saved.smooth_steps == 3);

    // The frame rate's place: top center unless a corner is named.
    assert(Settings{}.fps_position == FPS_OVERLAY_TOP_CENTER);
    for (int i = 0; i < BW_FPS_POSITIONS; ++i)
        assert(bw_fps_position(kBwFpsPositionValues[i]) == i);
    assert(bw_fps_position("bottom-right") == FPS_OVERLAY_BOTTOM_RIGHT);
    assert(bw_fps_position(nullptr) == 0 && bw_fps_position("") == 0);
    assert(bw_fps_position("middle") == 0 && bw_fps_position("Top-Left") == 0 && bw_fps_position("top-left ") == 0);
    // DOL_AURORA_FPS_POSITION for one session is not saved when another
    // setting changes; choosing a place in the menu is.
    Settings file;
    file.fps_position = FPS_OVERLAY_TOP_LEFT;
    Settings launch = file;
    launch.fps_position = bw_fps_position("bottom-right");
    Settings edit = launch;
    launch.show_fps = true;
    bw_settings_keep_edits(file, edit, launch);
    assert(file.show_fps && file.fps_position == FPS_OVERLAY_TOP_LEFT);
    edit = launch;
    launch.fps_position = FPS_OVERLAY_TOP_RIGHT;
    bw_settings_keep_edits(file, edit, launch);
    assert(file.fps_position == FPS_OVERLAY_TOP_RIGHT);

    BwStickZoomGesture gesture{};
    double axis = 0.0;
    assert(bw_stick_zoom_update(&gesture, true, true, 0.0, 0.0, 0.12, &axis) == BW_STICK_ZOOM_IDLE);
    assert(bw_stick_zoom_update(&gesture, true, false, 0.0, 0.0, 0.12, &axis) == BW_STICK_ZOOM_TAP);
    assert(bw_stick_zoom_update(&gesture, true, true, 0.0, 0.0, 0.12, &axis) == BW_STICK_ZOOM_IDLE);
    assert(bw_stick_zoom_update(&gesture, true, true, 0.0, 1.0, 0.12, &axis) == BW_STICK_ZOOM_ACTIVE);
    assert(axis == 1.0);
    assert(bw_stick_zoom_update(&gesture, true, false, 0.4, 0.5, 0.12, &axis) == BW_STICK_ZOOM_FINISHED);
    assert(gesture.block_camera);
    assert(bw_stick_zoom_update(&gesture, true, false, 0.4, 0.0, 0.12, &axis) == BW_STICK_ZOOM_IDLE);
    assert(gesture.block_camera);
    assert(bw_stick_zoom_update(&gesture, true, false, 0.0, 0.4, 0.12, &axis) == BW_STICK_ZOOM_IDLE);
    assert(gesture.block_camera);
    assert(bw_stick_zoom_update(&gesture, true, false, 0.0, 0.0, 0.12, &axis) == BW_STICK_ZOOM_IDLE);
    assert(!gesture.block_camera);
    assert(bw_stick_zoom_update(&gesture, true, true, 1.0, 0.0, 0.12, &axis) == BW_STICK_ZOOM_ACTIVE);
    assert(axis == 0.0);
    assert(bw_stick_zoom_update(&gesture, true, false, 0.0, 0.0, 0.12, &axis) == BW_STICK_ZOOM_FINISHED);
    assert(gesture.block_camera);
    assert(bw_stick_zoom_update(&gesture, true, false, 0.0, 0.0, 0.12, &axis) == BW_STICK_ZOOM_IDLE);
    assert(!gesture.block_camera);
    assert(bw_stick_zoom_update(&gesture, false, true, 0.0, 1.0, 0.12, &axis) == BW_STICK_ZOOM_IDLE);
    assert(bw_stick_zoom_advance(1.0, 1.0, 0.75, 1.0, 0.5, 2.0) == 1.75);
    assert(bw_stick_zoom_advance(1.8, 1.0, 0.75, 1.0, 0.5, 2.0) == 2.0);
    assert(bw_stick_zoom_advance(0.6, -1.0, 0.75, 1.0, 0.5, 2.0) == 0.5);

    unsigned long long tap_started = 0;
    assert(bw_stick_tap_update(&tap_started, 100, true, false, 30) == BW_STICK_TAP_HOLD);
    assert(bw_stick_tap_update(&tap_started, 110, false, false, 30) == BW_STICK_TAP_HOLD);
    assert(bw_stick_tap_update(&tap_started, 111, false, true, 30) == BW_STICK_TAP_ACCEPTED);
    assert(tap_started == 0);
    assert(bw_stick_tap_update(&tap_started, 200, true, false, 30) == BW_STICK_TAP_HOLD);
    assert(bw_stick_tap_update(&tap_started, 230, false, false, 30) == BW_STICK_TAP_TIMED_OUT);
    assert(tap_started == 0);
}
