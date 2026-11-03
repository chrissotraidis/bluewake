#include "fps_watch.h"

#include "fast_load.h"

#include "gxruntime/aurora_backend.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#if defined(__APPLE__)
#include <sys/sysctl.h>
#endif

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
// main.c: Aurora's created-pipeline count, so this file needs no Aurora headers.
unsigned bluewake_host_pipelines_created(void);

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
    // Smooth Motion on but most game frames shown without an in-between frame:
    // each is presented twice, so the shown count stays at the target while the
    // picture moves at 30. Counted even when shown looks fine.
    const bool uninterpolated = smooth && frames > 0u && (double)interpolated / (double)frames < 0.9;
    if (shown >= target_fps(smooth, steps) * 0.95 && !uninterpolated)
        return NULL;
    if (speed < 0.97)
        return "game below full speed";
    if (uninterpolated)
        return "frames not interpolated";
    return "presents late";
}

const char* bluewake_fps_watch_cause(double speed, double game_busy, double gx_worker, double interp_helper,
                                    double render_worker, double gx_wait_ms, double present_ms,
                                    unsigned pipelines) {
    if (pipelines > 0u && gx_wait_ms >= 100.0)
        return "shader-compile";
    if (gx_worker >= 85.0 || gx_wait_ms >= 300.0)
        return "gx-worker";
    if (present_ms >= 300.0)
        return "gpu-present";
    if (render_worker >= 85.0)
        return "render-worker";
    if (interp_helper >= 85.0)
        return "interp-helper";
    if (speed < 0.97 && (game_busy >= 70.0 || gx_wait_ms + present_ms < 150.0))
        return "game-thread";
    return "unclear";
}

static CPUState* g_cpu;
static bool g_enabled = true;
static DolAuroraFrameTiming g_last;
static unsigned long long g_last_wall_us, g_last_cpu_us, g_last_retrace;
static unsigned long long g_retrace;
static unsigned long long g_dips;
static unsigned g_last_pipelines;

// The session's slow seconds, summed for [perf-summary]: every ten minutes and at
// exit, by cause and by place, so a long log says where it was slow in one line.
enum { kCauses = 8, kPlaces = 12 };
static const char* const kCauseNames[kCauses] = {"gx-worker",     "game-thread",   "shader-compile",
                                                 "gpu-present",   "render-worker", "interp-helper",
                                                 "smooth-motion-paused", "unclear"};
static unsigned long long g_cause_seconds[kCauses];
static struct {
    char stage[9];
    int room;
    unsigned long long seconds;
} g_places[kPlaces];
static unsigned long long g_other_place_seconds, g_watched_seconds, g_start_us, g_summary_us;
static unsigned g_pipelines_made;
static double g_lowest_speed = 1.0;
static int g_logged_smooth = -1, g_logged_steps = -1;

static unsigned long long now_us(clockid_t clock) {
    struct timespec ts;
    clock_gettime(clock, &ts);
    return (unsigned long long)ts.tv_sec * 1000000ull + (unsigned long long)ts.tv_nsec / 1000ull;
}

static void count_place(const char* stage, int room) {
    for (unsigned i = 0; i < kPlaces; ++i) {
        if (g_places[i].seconds == 0u) {
            memcpy(g_places[i].stage, stage, sizeof g_places[i].stage);
            g_places[i].room = room;
        } else if (strcmp(g_places[i].stage, stage) != 0 || g_places[i].room != room) {
            continue;
        }
        ++g_places[i].seconds;
        return;
    }
    ++g_other_place_seconds;
}

static void print_summary(const char* when) {
    if (g_start_us == 0u)
        return;
    unsigned long long below = 0;
    for (unsigned i = 0; i < kCauses; ++i)
        below += g_cause_seconds[i];
    char causes[256] = "", places[512] = "";
    size_t n = 0;
    for (unsigned i = 0; i < kCauses; ++i)
        if (g_cause_seconds[i] != 0u && n < sizeof causes)
            n += (size_t)snprintf(causes + n, sizeof causes - n, " %s=%llu", kCauseNames[i], g_cause_seconds[i]);
    n = 0;
    for (unsigned i = 0; i < kPlaces && g_places[i].seconds != 0u && n < sizeof places; ++i)
        n += (size_t)snprintf(places + n, sizeof places - n, " %s/%d=%llu", g_places[i].stage, g_places[i].room,
                              g_places[i].seconds);
    if (g_other_place_seconds != 0u && n < sizeof places)
        snprintf(places + n, sizeof places - n, " other=%llu", g_other_place_seconds);
    fprintf(stderr,
            "[perf-summary] %s minutes=%.1f watched_s=%llu below_target_s=%llu lowest_speed=%.0f%% "
            "pipelines_made=%u causes:%s places:%s\n",
            when, (double)(now_us(CLOCK_MONOTONIC) - g_start_us) / 60e6, g_watched_seconds, below,
            g_lowest_speed * 100.0, g_pipelines_made, below ? causes : " none", below ? places : " none");
}

static void print_summary_at_exit(void) { print_summary("exit"); }

static void log_settings(bool smooth, int steps) {
    if ((int)smooth == g_logged_smooth && steps == g_logged_steps)
        return;
    g_logged_smooth = smooth;
    g_logged_steps = steps;
    fprintf(stderr, "[smooth-motion] %s in_between=%d target_fps=%.0f\n", smooth ? "on" : "off",
            smooth ? steps : 0, target_fps(smooth, steps));
}

static void log_device(void) {
#if defined(__APPLE__)
    // Aurora reports Apple systems as "macOS" with an unknown CPU; this tells an
    // iPad from a Mac and gives the OS version.
    char model[64] = "?", machine[64] = "?", os[32] = "?";
    size_t size = sizeof model;
    sysctlbyname("hw.model", model, &size, NULL, 0);
    size = sizeof machine;
    sysctlbyname("hw.machine", machine, &size, NULL, 0);
    size = sizeof os;
    sysctlbyname("kern.osproductversion", os, &size, NULL, 0);
    fprintf(stderr, "[device] model=%s machine=%s os=%s\n", model, machine, os);
#endif
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

// BLUEWAKE_TEST_STALL=retrace:ms[,retrace:ms...] (testing only): the game thread
// held that long at those retraces, a hitch like a pipeline compiled or a file
// read, to check how frame interpolation's pacing takes one.
static unsigned long long g_stall_retrace[16];
static unsigned g_stall_ms[16];
static unsigned g_stall_count;

void bluewake_fps_watch_attach(CPUState* cpu) {
    g_cpu = cpu;
    const char* on = getenv("BLUEWAKE_FPS_WATCH");
    g_enabled = on == NULL || on[0] != '0';
    log_device();
    if (g_enabled)
        atexit(print_summary_at_exit);
    const char* place = getenv("BLUEWAKE_TEST_PLACE");
    if (place != NULL &&
        sscanf(place, "%llu:%f:%f:%f", &g_place_retrace, &g_place[0], &g_place[1], &g_place[2]) != 4)
        g_place_retrace = 0;
    const char* stall = getenv("BLUEWAKE_TEST_STALL");
    for (const char* at = stall; at != NULL && *at != '\0' && g_stall_count < 16u;) {
        unsigned long long retrace = 0;
        unsigned ms = 0;
        if (sscanf(at, "%llu:%u", &retrace, &ms) != 2)
            break;
        g_stall_retrace[g_stall_count] = retrace;
        g_stall_ms[g_stall_count++] = ms;
        at = strchr(at, ',');
        if (at != NULL)
            ++at;
    }
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
    for (unsigned i = 0; i < g_stall_count; ++i) {
        if (g_stall_retrace[i] != g_retrace)
            continue;
        const unsigned long long until = now_us(CLOCK_MONOTONIC) + 1000ull * g_stall_ms[i];
        while (now_us(CLOCK_MONOTONIC) < until) {
        }
        fprintf(stderr, "[test-stall] retrace=%llu held %u ms\n", g_retrace, g_stall_ms[i]);
    }
    if (!g_enabled || g_cpu == NULL)
        return;
    const unsigned long long wall = now_us(CLOCK_MONOTONIC);
    if (g_last_wall_us == 0u) {
        g_last_wall_us = wall;
        g_last_cpu_us = now_us(CLOCK_THREAD_CPUTIME_ID);
        g_last_retrace = g_retrace;
        dol_aurora_frame_timing(&g_last);
        g_last_pipelines = bluewake_host_pipelines_created();
        return;
    }
    if (wall - g_last_wall_us < 1000000ull)
        return;
    DolAuroraFrameTiming now;
    dol_aurora_frame_timing(&now);
    const unsigned pipelines_total = bluewake_host_pipelines_created();
    const unsigned pipelines = pipelines_total >= g_last_pipelines ? pipelines_total - g_last_pipelines : 0u;
    g_last_pipelines = pipelines_total;
    g_pipelines_made += pipelines;
    if (g_start_us == 0u)
        g_start_us = g_summary_us = wall;
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
    // ... nor one in which the host held the guest (a menu, the background).
    const bool held = now.held_us - g_last.held_us > 100000ull;
    const bool skip = bluewake_fast_load_fast_forward() || now.shown == g_last.shown || speed > 1.05 || held ||
                      link < 0x80000000u || link >= 0x81800000u;
    const bool smooth = aurora_get_frame_interpolation();
    const int steps = aurora_get_frame_interp_steps();
    log_settings(smooth, steps);
    if (!skip)
        ++g_watched_seconds;
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
        const double gx_worker = bluewake_fps_watch_cpu_percent(now.gx_worker_cpu_us, g_last.gx_worker_cpu_us, wall_us);
        const double interp_helper =
            bluewake_fps_watch_cpu_percent(now.interp_helper_cpu_us, g_last.interp_helper_cpu_us, wall_us);
        const double render_worker =
            bluewake_fps_watch_cpu_percent(now.render_worker_cpu_us, g_last.render_worker_cpu_us, wall_us);
        // In-between frames turned off by Smooth Motion's pacing (none at all at
        // full speed): what the player sees is the pause, after an earlier slowdown.
        const char* cause = strcmp(reason, "frames not interpolated") == 0 && interpolated == 0u
                                ? "smooth-motion-paused"
                                : bluewake_fps_watch_cause(speed, busy, gx_worker, interp_helper, render_worker,
                                                           gx_ms, present_ms, pipelines);
        for (unsigned i = 0; i < kCauses; ++i)
            if (strcmp(kCauseNames[i], cause) == 0)
                ++g_cause_seconds[i];
        const int room = (int)(signed char)mem_read8(cpu, kStayRoom);
        count_place(stage, room);
        if (speed < g_lowest_speed)
            g_lowest_speed = speed;
        ++g_dips;
        fprintf(stderr,
                "[fps-dip] retrace=%llu shown=%.1f game=%llu speed=%.0f%% interpolated=%llu/%llu "
                "draws/frame=%llu rejected=%.1f%% unmatched=%.1f%% busy=%.0f%% waits: gx=%.0fms present=%.0fms "
                "gpu=%.0fms workers: gx=%.0f%% interp=%.0f%% render=%.0f%% "
                "stage=%s room=%d event=%u pos=%.0f,%.0f,%.0f target=%.0f pipelines=%u cause=%s reason=%s\n",
                g_retrace, shown, game, speed * 100.0, interpolated, frames, frames ? draws / frames : 0ull,
                draws ? 100.0 * (double)rejected / (double)draws : 0.0,
                draws ? 100.0 * (double)unmatched / (double)draws : 0.0, busy, gx_ms, present_ms, gpu_ms,
                gx_worker, interp_helper, render_worker, stage, room, mem_read8(cpu, kEventMode), x, y, z,
                target_fps(smooth, steps), pipelines, cause, reason);
    }
    if (wall - g_summary_us >= 600000000ull) {
        g_summary_us = wall;
        print_summary("periodic");
    }
    g_last = now;
    g_last_wall_us = wall;
    g_last_cpu_us = cpu_us;
    g_last_retrace = g_retrace;
}
