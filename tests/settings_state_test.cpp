// The desktop settings' defaults, what the menu's core keeps (a launch choice is
// the session's, a player's edit is saved), Smooth Motion's rate, the frame
// rate's place, and the right stick's zoom.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <limits>
#include <string>
#include "../runtime/host/src/smooth_rate.h"
#include "../runtime/host/src/fps_position.h"
#include "../runtime/host/src/stick_zoom.h"

// The settings core as it is (no ImGui, SDL or Aurora), with the game's
// Better Wind Waker options and the platform's pack folder left empty.
extern "C" const char* bluewake_game_options_describe(uint32_t, const char**, bool*, bool*) { return nullptr; }
#include "../runtime/host/src/desktop_settings.cpp"
namespace bw_platform {
std::string default_pack_folder() { return ""; }
}  // namespace bw_platform

static const char* fallback(const char* key) { return bw_settings::fallback_for(*bw_settings::find(key)); }

static void launch_option(const char* key, const char* value) {
#if defined(_WIN32)
    _putenv_s(key, value);
#else
    setenv(key, value, 1);
#endif
}

static std::string read_file(const std::string& path) {
    std::ifstream file(path);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

static bool has(const std::string& text, const char* line) { return text.find(line) != std::string::npos; }

int main() {
    // The defaults, on every platform.
    assert(std::string(fallback("DOL_AURORA_FRAME_INTERP")) == "0" &&
           bw_smooth_requested(fallback("DOL_AURORA_FRAME_INTERP_STEPS")) == 1);
    assert(std::string(fallback("BLUEWAKE_STICK_ZOOM")) == "0" &&
           std::string(fallback("BLUEWAKE_STICK_ZOOM_SPEED")) == "1.275");
    assert(std::string(fallback("BLUEWAKE_HAPTICS")) == "enhanced" &&
           std::string(fallback("BLUEWAKE_HAPTICS_STRENGTH")) == "80" &&
           std::string(fallback("BLUEWAKE_HAPTICS_TRIGGERS")) == "1");
    // Jump & Sprint's sprint: 1.5 until the player moves the slider.
    assert(std::string(fallback("BLUEWAKE_SPRINT_CHOICE")) == "1.5");
    // The frame rate's place: top center unless a corner is named.
    assert(bw_fps_position(fallback("DOL_AURORA_FPS_POSITION")) == FPS_OVERLAY_TOP_CENTER);

    // What the core keeps: a launch option wins for this session and the file
    // keeps its own value; a player's edit is saved; choosing the frame rate's
    // place in the menu is saved, though the launch set one.
    const char* temp = std::getenv("TEMP");
    if (temp == nullptr)
        temp = std::getenv("TMPDIR");
    const std::string path = std::string(temp != nullptr ? temp : "/tmp") + "/bluewake_settings_state_test.ini";
    {
        std::ofstream file(path);
        file << "DOL_AURORA_RENDER_SCALE=0\nDOL_AURORA_FPS_POSITION=top-left\nDOL_AURORA_SHOW_FPS=0\nUNRELATED=kept\n"
             // Two old pairs, each held half on.
             << "BLUEWAKE_FAST_FORWARD=0\nBLUEWAKE_FADE_FRAMES=6\nBLUEWAKE_JUMP_BUTTON=0\nBLUEWAKE_SPRINT_SPEED=1.84\n";
    }
    launch_option("DOL_AURORA_RENDER_SCALE", "4");
    launch_option("DOL_AURORA_FPS_POSITION", "bottom-right");
    bw_desktop::load(path);
    assert(bw_desktop::env("DOL_AURORA_RENDER_SCALE") == "4");
    assert(bw_fps_position(bw_desktop::env("DOL_AURORA_FPS_POSITION").c_str()) == FPS_OVERLAY_BOTTOM_RIGHT);
    assert(bw_desktop::env("DOL_AURORA_SHOW_FPS") == "0");
    // The pairs: as one switch where the table reads them whole (Windows), as
    // each half says elsewhere.
    if (bw_settings::includes(bw_settings::find("BLUEWAKE_FAST_FORWARD")->whole, bw_settings::kThisOs))
        assert(bw_desktop::env("BLUEWAKE_FADE_FRAMES") == "0");
    else
        assert(bw_desktop::env("BLUEWAKE_FADE_FRAMES") == "6");
    if (bw_settings::includes(bw_settings::find("BLUEWAKE_JUMP_BUTTON")->whole, bw_settings::kThisOs))
        assert(bw_desktop::env("BLUEWAKE_JUMP_BUTTON") == "1" && bw_desktop::env("BLUEWAKE_SPRINT_SPEED") == "1.84");
    else
        assert(bw_desktop::env("BLUEWAKE_JUMP_BUTTON") == "0" && bw_desktop::env("BLUEWAKE_SPRINT_SPEED") == "1.84");
    bw_desktop::set_env("DOL_AURORA_SHOW_FPS", "1");
    bw_desktop::save();
    std::string saved = read_file(path);
    assert(has(saved, "DOL_AURORA_SHOW_FPS=1\n") && has(saved, "DOL_AURORA_RENDER_SCALE=0\n"));
    assert(has(saved, "DOL_AURORA_FPS_POSITION=top-left\n") && has(saved, "UNRELATED=kept\n"));
    bw_desktop::set_env("DOL_AURORA_FPS_POSITION", "top-right");
    bw_desktop::save();
    saved = read_file(path);
    assert(has(saved, "DOL_AURORA_FPS_POSITION=top-right\n") && has(saved, "DOL_AURORA_RENDER_SCALE=0\n"));
    std::remove(path.c_str());

    // Smooth Motion's rate.
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
    // Falling back on a slower or unknown display never edits the preference.
    assert(bw_smooth_steps(3, 60) == 1);

    // The frame rate's place.
    for (int i = 0; i < BW_FPS_POSITIONS; ++i)
        assert(bw_fps_position(kBwFpsPositionValues[i]) == i);
    assert(bw_fps_position("bottom-right") == FPS_OVERLAY_BOTTOM_RIGHT);
    assert(bw_fps_position(nullptr) == 0 && bw_fps_position("") == 0);
    assert(bw_fps_position("middle") == 0 && bw_fps_position("Top-Left") == 0 && bw_fps_position("top-left ") == 0);

    // The right stick's zoom.
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
