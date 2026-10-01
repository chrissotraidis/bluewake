#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include "../windows/src/settings_state.h"
int main() {
    Settings saved;
    saved.options["old"] = true;
    Settings session = saved;
    session.fullscreen = true; session.render_scale = 4; session.lle_audio = true;
    session.smooth_motion = true; session.options["session_only"] = true;
    Settings before = session;
    session.show_fps = true;
    bw_settings_keep_edits(saved, before, session);
    assert(saved.show_fps && !saved.fullscreen && saved.render_scale == 0);
    assert(!saved.lle_audio && !saved.smooth_motion && saved.options.count("session_only") == 0);
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
}
