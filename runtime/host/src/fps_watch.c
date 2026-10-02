#include "fps_watch.h"

#include "fast_load.h"

#include "gxruntime/aurora_backend.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Once a second of wall time, when fewer than 95% of the selected FPS reached the
// screen: what the game and the in-between frames did in that second, and
// where Link was, so the places where Smooth Motion does not hold 60 can be
// found and fixed. A scene change's fast-forward (nothing to see) is skipped.
enum {
    kCurStage = 0x803C9D3Cu,      // g_dComIfG_gameInfo.play.mCurStage (name[8], point, room, layer)
    kStayRoom = 0x803F6A78u,      // dStage_roomControl_c::mStayNo
    kPlayerPointer = 0x803CA74Cu, // dComIfGp_getPlayer(0)
    kPos = 0x1F8u,                // fopAc_ac_c::current.pos
    kOldPos = 0x1E4u,             // fopAc_ac_c::old.pos
    kEventMode = 0x803C9EA2u,     // g_dComIfG_gameInfo.play.mEvtCtrl's mode
};

// Read the live setting, not getenv: desktop/mobile menus can change it at runtime.
bool aurora_get_frame_interpolation(void);
int aurora_get_frame_interp_steps(void);

static double target_fps(bool smooth, int steps) {
    if (!smooth)
        return 30.0; // the retail simulation without in-between frames
    if (steps < 1) steps = 1;
    if (steps > 3) steps = 3;
    return 30.0 * (steps + 1);
}

double bluewake_fps_watch_cpu_percent(unsigned long long current,
                                     unsigned long long previous,
                                     unsigned long long wall_us) {
    if (wall_us == 0u || current < previous)
        return 0.0;
    return 100.0 * (double)(current - previous) / (double)wall_us;
}

const char* bluewake_fps_watch_reason(double shown, double speed, bool smooth,
                                    int steps, unsigned long long frames,
                                    unsigned long long interpolated) {
    if (shown >= target_fps(smooth, steps) * 0.95)
        return NULL;
    if (speed < 0.97)
        return "game below full speed";
    if (smooth && frames > 0u && (double)interpolated / (double)frames < 0.9)
        return "frames not interpolated";
    return "presents late";
}

static CPUState* g_cpu;
static bool g_enabled = true;
static DolAuroraFrameTiming g_last;
static unsigned long long g_last_wall_us, g_last_cpu_us, g_last_retrace;
static unsigned long long g_retrace;
static unsigned long long g_dips;

static unsigned long long now_us(clockid_t clock) {
    struct timespec ts;
    clock_gettime(clock, &ts);
    return (unsigned long long)ts.tv_sec * 1000000ull + (unsigned long long)ts.tv_nsec / 1000ull;
}

static float read_f32(CPUState* cpu, u32 address) {
    const u32 bits = mem_read32(cpu, address);
    float value;
    memcpy(&value, &bits, sizeof value);
    return value;
}

// BLUEWAKE_TEST_PLACE=retrace:x:y:z (testing only): Link stood at x, y, z of
// the current stage from that retrace, held there for a few, so a view can be
// reached without a route to it.
static unsigned long long g_place_retrace;
static float g_place[3];

void bluewake_fps_watch_attach(CPUState* cpu) {
    g_cpu = cpu;
    const char* on = getenv("BLUEWAKE_FPS_WATCH");
    g_enabled = on == NULL || on[0] != '0';
    const char* place = getenv("BLUEWAKE_TEST_PLACE");
    if (place != NULL &&
        sscanf(place, "%llu:%f:%f:%f", &g_place_retrace, &g_place[0], &g_place[1], &g_place[2]) != 4)
        g_place_retrace = 0;
}

static void write_f32(CPUState* cpu, u32 address, float value) {
    u32 bits;
    memcpy(&bits, &value, sizeof bits);
    mem_write32(cpu, address, bits);
}

static void place_player(void) {
    const u32 player = mem_read32(g_cpu, kPlayerPointer);
    if (player < 0x80000000u || player >= 0x81800000u)
        return;
    for (u32 i = 0; i < 3u; ++i) {
        write_f32(g_cpu, player + kPos + i * 4u, g_place[i]);
        write_f32(g_cpu, player + kOldPos + i * 4u, g_place[i]);
    }
    fprintf(stderr, "[test-place] retrace=%llu player=0x%08X now %.0f,%.0f,%.0f\n", g_retrace, player,
            read_f32(g_cpu, player + kPos), read_f32(g_cpu, player + kPos + 4u), read_f32(g_cpu, player + kPos + 8u));
}

void bluewake_fps_watch_retrace(void) {
    ++g_retrace;
    if (g_place_retrace != 0 && g_cpu != NULL && g_retrace >= g_place_retrace && g_retrace < g_place_retrace + 8)
        place_player();
    if (!g_enabled || g_cpu == NULL)
        return;
    const unsigned long long wall = now_us(CLOCK_MONOTONIC);
    if (g_last_wall_us == 0u) {
        g_last_wall_us = wall;
        g_last_cpu_us = now_us(CLOCK_THREAD_CPUTIME_ID);
        g_last_retrace = g_retrace;
        dol_aurora_frame_timing(&g_last);
        return;
    }
    if (wall - g_last_wall_us < 1000000ull)
        return;
    DolAuroraFrameTiming now;
    dol_aurora_frame_timing(&now);
    const unsigned long long cpu_us = now_us(CLOCK_THREAD_CPUTIME_ID);
    const double seconds = (double)(wall - g_last_wall_us) / 1e6;
    const double shown = (double)(now.shown - g_last.shown) / seconds;
    const double speed = (double)(g_retrace - g_last_retrace) / seconds / 59.94;
    const unsigned long long game = now.presents - g_last.presents;
    const unsigned long long frames = now.interp_frames - g_last.interp_frames;
    const unsigned long long interpolated = now.interp_interpolated - g_last.interp_interpolated;
    const unsigned long long draws = now.interp_draws - g_last.interp_draws;
    const unsigned long long rejected = now.interp_rejected - g_last.interp_rejected;
    const unsigned long long unmatched = now.interp_unmatched - g_last.interp_unmatched;
    const unsigned long long wall_us = wall - g_last_wall_us;
    const double busy = bluewake_fps_watch_cpu_percent(cpu_us, g_last_cpu_us, wall_us);
    // Where the emulation thread waited, in milliseconds of this second: for
    // the GX translation worker at the game's draw barriers, in presents, and
    // the part of those in the GPU submission and waiting for a drawable.
    const double gx_ms = (double)(now.drain_us - g_last.drain_us) / 1000.0 / seconds;
    const double present_ms = (double)(now.present_us - g_last.present_us) / 1000.0 / seconds;
    const double gpu_ms = (double)(now.end_frame_us - g_last.end_frame_us) / 1000.0 / seconds;
    // Not a second with a scene change's fast-forward in it (the game ran
    // faster than real time), nor the title and file screens (no Link).
    const u32 link = mem_read32(g_cpu, kPlayerPointer);
    const bool skip = bluewake_fast_load_fast_forward() || now.shown == g_last.shown || speed > 1.05 ||
                      link < 0x80000000u || link >= 0x81800000u;
    const bool smooth = aurora_get_frame_interpolation();
    const int steps = aurora_get_frame_interp_steps();
    const char* reason = bluewake_fps_watch_reason(shown, speed, smooth, steps, frames, interpolated);
    if (!skip && reason != NULL) {
        CPUState* cpu = g_cpu;
        char stage[9] = {0};
        for (u32 i = 0; i < 8u; ++i)
            stage[i] = (char)mem_read8(cpu, kCurStage + i);
        const u32 player = mem_read32(cpu, kPlayerPointer);
        float x = 0.f, y = 0.f, z = 0.f;
        if (player >= 0x80000000u && player < 0x81800000u) {
            x = read_f32(cpu, player + kPos);
            y = read_f32(cpu, player + kPos + 4u);
            z = read_f32(cpu, player + kPos + 8u);
        }
        ++g_dips;
        fprintf(stderr,
                "[fps-dip] retrace=%llu shown=%.1f game=%llu speed=%.0f%% interpolated=%llu/%llu "
                "draws/frame=%llu rejected=%.1f%% unmatched=%.1f%% busy=%.0f%% waits: gx=%.0fms present=%.0fms "
                "gpu=%.0fms workers: gx=%.0f%% interp=%.0f%% render=%.0f%% "
                "stage=%s room=%d event=%u pos=%.0f,%.0f,%.0f target=%.0f reason=%s\n",
                g_retrace, shown, game, speed * 100.0, interpolated, frames, frames ? draws / frames : 0ull,
                draws ? 100.0 * (double)rejected / (double)draws : 0.0,
                draws ? 100.0 * (double)unmatched / (double)draws : 0.0, busy, gx_ms, present_ms, gpu_ms,
                bluewake_fps_watch_cpu_percent(now.gx_worker_cpu_us, g_last.gx_worker_cpu_us, wall_us),
                bluewake_fps_watch_cpu_percent(now.interp_helper_cpu_us, g_last.interp_helper_cpu_us, wall_us),
                bluewake_fps_watch_cpu_percent(now.render_worker_cpu_us, g_last.render_worker_cpu_us, wall_us), stage,
                (int)(signed char)mem_read8(cpu, kStayRoom), mem_read8(cpu, kEventMode), x, y, z,
                target_fps(smooth, steps), reason);
    }
    g_last = now;
    g_last_wall_us = wall;
    g_last_cpu_us = cpu_us;
    g_last_retrace = g_retrace;
}
