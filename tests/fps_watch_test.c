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
unsigned bluewake_host_pipelines_created(void) { return 0; }

static int cause_is(const char* cause, const char* expected) { return strcmp(cause, expected) == 0; }

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
    // The iPad with Smooth Motion dropped: 60 shown (each frame twice), none of the
    // 30 game frames interpolated. Previously no dip because shown looked fine.
    assert(strcmp(bluewake_fps_watch_reason(60, 1.0, true, 1, 30, 0), "frames not interpolated") == 0);
    assert(strcmp(bluewake_fps_watch_reason(118, 1.0, true, 3, 30, 0), "frames not interpolated") == 0);
    // ... and the same while the game itself was slow (the reason the frames were dropped).
    assert(strcmp(bluewake_fps_watch_reason(51, 0.86, true, 1, 26, 0), "game below full speed") == 0);
    // Smooth Motion off: repeated frames are expected, no dip.
    assert(bluewake_fps_watch_reason(30, 1.0, false, 1, 30, 0) == NULL);
    // The Forsaken Fortress dips in a player's 0.4.0 log: game at 75%, GX worker
    // 95%, game thread about half, 450 ms of each second waiting on the worker.
    assert(cause_is(bluewake_fps_watch_cause(0.75, 55, 95, 0, 20, 450, 3, 0), "gx-worker"));
    // The same wait while pipelines were made is a shader compile.
    assert(cause_is(bluewake_fps_watch_cause(0.75, 55, 95, 0, 20, 450, 3, 4), "shader-compile"));
    // Pipelines made in the background, the game not waiting: not a compile stall.
    assert(cause_is(bluewake_fps_watch_cause(0.80, 95, 30, 0, 20, 10, 3, 4), "game-thread"));
    // BLUEWAKE_TEST_STALL on a Mac: 150 ms and 900 ms holds on the game thread
    // (speed 86% and 47%, the thread 73% and 85% busy, almost no waiting).
    assert(cause_is(bluewake_fps_watch_cause(0.86, 73, 33, 0, 6, 5, 2, 0), "game-thread"));
    assert(cause_is(bluewake_fps_watch_cause(0.47, 85, 19, 0, 4, 22, 1, 0), "game-thread"));
    assert(cause_is(bluewake_fps_watch_cause(1.0, 40, 30, 0, 20, 5, 400, 0), "gpu-present"));
    assert(cause_is(bluewake_fps_watch_cause(1.0, 40, 30, 0, 90, 5, 40, 0), "render-worker"));
    assert(cause_is(bluewake_fps_watch_cause(1.0, 40, 30, 92, 20, 5, 40, 0), "interp-helper"));
    // Full speed, nothing saturated: presents late for no visible reason.
    assert(cause_is(bluewake_fps_watch_cause(1.0, 40, 30, 0, 20, 5, 40, 0), "unclear"));
    return 0;
}
