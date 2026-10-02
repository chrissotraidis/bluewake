#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include "../windows/src/settings_state.h"
#include "../runtime/host/src/smooth_rate.h"
#include <limits>
int main() {
    Settings saved;
    assert(!saved.smooth_motion && saved.smooth_steps == 1);
    saved.options["old"] = true;
    Settings session = saved;
    session.fullscreen = true; session.render_scale = 4; session.lle_audio = true;
    session.smooth_motion = true; session.smooth_steps = -1; session.options["session_only"] = true;
    Settings before = session;
    session.show_fps = true;
    bw_settings_keep_edits(saved, before, session);
    assert(saved.show_fps && !saved.fullscreen && saved.render_scale == 0);
    assert(!saved.lle_audio && !saved.smooth_motion && saved.options.count("session_only") == 0);
    assert(saved.smooth_steps == 1); // A session override is not a stored edit.
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
}
