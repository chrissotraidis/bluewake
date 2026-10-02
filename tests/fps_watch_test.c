#include "fps_watch.h"
#include "gxruntime/aurora_backend.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <string.h>

bool bluewake_fast_load_fast_forward(void) { return false; }
bool aurora_get_frame_interpolation(void) { return false; }
int aurora_get_frame_interp_steps(void) { return 1; }
void dol_aurora_frame_timing(DolAuroraFrameTiming* out) { memset(out, 0, sizeof(*out)); }

int main(void) {
    assert(bluewake_fps_watch_cpu_percent(750000, 250000, 1000000) == 50.0);
    assert(bluewake_fps_watch_cpu_percent(250000, 250000, 1000000) == 0.0);
    assert(bluewake_fps_watch_cpu_percent(10, 250000, 1000000) == 0.0);
    assert(bluewake_fps_watch_cpu_percent(750000, 250000, 0) == 0.0);
    assert(bluewake_fps_watch_reason(29.3, 1.0, false, 3, 0, 0) == NULL);
    assert(bluewake_fps_watch_reason(59.0, 1.0, true, 1, 30, 30) == NULL);
    assert(bluewake_fps_watch_reason(118.0, 1.0, true, 3, 30, 30) == NULL);
    assert(strcmp(bluewake_fps_watch_reason(20, 0.7, false, 1, 0, 0),
                  "game below full speed") == 0);
    assert(strcmp(bluewake_fps_watch_reason(30, 1.0, true, 1, 30, 0),
                  "frames not interpolated") == 0);
    assert(strcmp(bluewake_fps_watch_reason(50, 1.0, true, 1, 30, 30),
                  "presents late") == 0);
    assert(strcmp(bluewake_fps_watch_reason(60, 1.0, true, 3, 30, 30),
                  "presents late") == 0); // does not confuse 60 with requested 120
    assert(bluewake_fps_watch_reason(59, 1.0, true, 0, 30, 30) == NULL);
    assert(bluewake_fps_watch_reason(118, 1.0, true, 99, 30, 30) == NULL);
    return 0;
}
