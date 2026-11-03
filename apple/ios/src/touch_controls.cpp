// BlueWake iOS input and pause glue between the UIKit shell and the host.
//
// The on-screen controls, the three-dot menu and the layout editor are UIKit
// (BWGameOverlay.mm, adapted from SunPad). This file owns what they share with
// the running game:
//   - the touch pad state, published into Aurora's virtual pad for port 0,
//     where it merges with the keyboard and any controller;
//   - a minimum hold per button, so a tap shorter than one guest frame is
//     still seen by the game's once-per-frame pad read;
//   - the pause-reason set: the guest is held at a frame boundary while the
//     app is inactive or a menu, settings panel, layout editor or alert is
//     open, and resumes only when no reason remains (PRD FR-014/FR-015).
#include <SDL3/SDL_events.h>
#include <aurora/gfx.h>
#include <aurora/aurora.h>
#include <dolphin/pad.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <mutex>

#include "gxruntime/aurora_backend.h"
#include "controller_settings.h"
#include "touch_controls.h"
#include "../../../runtime/host/src/jump_button.h"
#include "../../../runtime/host/src/sprint.h"

extern "C" unsigned long long bluewake_host_retrace_count(void);  // runtime/host/src/main.c
extern "C" unsigned long long bluewake_host_thread_cpu_us(void);
extern "C" int bluewake_thermal_state(void);  // controller_settings.mm: 0 nominal .. 3 critical

namespace {

using Clock = std::chrono::steady_clock;
constexpr auto kMinHold = std::chrono::milliseconds(120);

std::mutex g_pad_mutex;
BlueWakeTouchPad g_pad{};         // what the controls report now
u16 g_latched = 0;                // buttons released before their minimum hold
Clock::time_point g_pressed_at[16];

std::atomic<unsigned> g_pause_reasons{0};

// Frames shown and why they were late. Measured once per presented frame from
// the overlay hook (which runs at the present), summarized once a second into
// the session log's [fps] line and the optional on-screen FPS display:
//   shown     frames presented in that second (the game draws 30)
//   worst     the longest gap between two presents
//   late      gaps over 50 ms (a frame the player saw missing)
//   speed     guest retraces against real time (100% is full game speed)
//   cpu       the emulation thread's own CPU time (waits excluded)
//   gpu_wait  main-thread time in aurora_end_frame: GPU submission and waiting
//             for a drawable; high values mean the GPU or display is behind
//   fifo_wait main-thread time waiting for the GX translation worker
//   draws     draw calls per presented frame
std::atomic<float> g_fps_shown{0.f};
std::atomic<float> g_fps_speed{0.f};
// The game's own frames a second, and whether Smooth Motion's in-between frames
// were paused (fewer than 90% of game frames interpolated): each game frame is
// then shown twice, so the shown count reads 60 while the picture moves at 30.
std::atomic<float> g_fps_game{0.f};
std::atomic<bool> g_fps_smooth_paused{false};
std::atomic<float> g_fps_worst_ms{0.f};

void fps_tick() {
    static bool log_enabled = [] {
        const char* env = std::getenv("BLUEWAKE_PERF_LOG");
        return env != nullptr && env[0] != '\0' && env[0] != '0';
    }();
    static Clock::time_point window_start, last;
    static unsigned frames, late;
    static double worst_ms;
    static bool paused_in_window;
    static unsigned long long retrace0, cpu0;
    static DolAuroraFrameTiming timing0;
    const auto now = Clock::now();
    const bool paused = g_pause_reasons.load() != 0;
    if (window_start == Clock::time_point{}) {
        window_start = last = now;
        retrace0 = bluewake_host_retrace_count();
        cpu0 = bluewake_host_thread_cpu_us();
        dol_aurora_frame_timing(&timing0);
        return;
    }
    const double gap_ms = std::chrono::duration<double, std::milli>(now - last).count();
    last = now;
    ++frames;
    worst_ms = std::max(worst_ms, gap_ms);
    if (gap_ms > 50.0) ++late;
    // Per-frame breakdown, logged for each late frame: how many guest
    // retraces passed (4 means the game itself took two frame times; 2 means
    // the game was on time and the delay was on the way to the screen), and
    // where the main thread's time in the gap went.
    {
        static unsigned long long f_retrace, f_cpu;
        static DolAuroraFrameTiming f_timing;
        static bool f_valid;
        DolAuroraFrameTiming t{};
        dol_aurora_frame_timing(&t);
        const unsigned long long r = bluewake_host_retrace_count();
        const unsigned long long c = bluewake_host_thread_cpu_us();
        if (f_valid && log_enabled && gap_ms > 50.0 && !paused) {
            std::fprintf(stderr,
                         "[late] gap_ms=%.0f retraces=%llu cpu_ms=%.1f present_ms=%.1f "
                         "gpu_ms=%.1f fifo_wait_ms=%.1f audio_wait_ms=%llu draws=%llu thermal=%d\n",
                         gap_ms, r - f_retrace, (c - f_cpu) / 1000.0,
                         (t.present_us - f_timing.present_us) / 1000.0,
                         (t.end_frame_us - f_timing.end_frame_us) / 1000.0,
                         (t.drain_us - f_timing.drain_us) / 1000.0,
                         t.audio_throttles - f_timing.audio_throttles,
                         t.draws - f_timing.draws, bluewake_thermal_state());
        }
        f_retrace = r;
        f_cpu = c;
        f_timing = t;
        f_valid = true;
    }
    paused_in_window |= paused;
    const double elapsed = std::chrono::duration<double>(now - window_start).count();
    if (elapsed < 1.0) return;
    const unsigned long long retrace = bluewake_host_retrace_count();
    const unsigned long long cpu = bluewake_host_thread_cpu_us();
    DolAuroraFrameTiming timing{};
    dol_aurora_frame_timing(&timing);
    const double shown = timing.shown > timing0.shown ? (timing.shown - timing0.shown) / elapsed
                                                    : frames / elapsed;
    const double speed = (retrace - retrace0) / elapsed / 60.0 * 100.0;
    g_fps_shown = static_cast<float>(shown);
    g_fps_speed = static_cast<float>(speed);
    g_fps_game = static_cast<float>((timing.display_copies - timing0.display_copies) / elapsed);
    {
        const unsigned long long frames = timing.interp_frames - timing0.interp_frames;
        const unsigned long long interpolated = timing.interp_interpolated - timing0.interp_interpolated;
        g_fps_smooth_paused = aurora_get_frame_interpolation() && frames > 0 && interpolated * 10 < frames * 9;
    }
    g_fps_worst_ms = static_cast<float>(worst_ms);
    if (log_enabled && !paused_in_window) {
        const unsigned long long presents = timing.presents - timing0.presents;
        std::fprintf(stderr,
                     "[fps] shown=%.1f game=%.1f worst_ms=%.0f late=%u speed=%.0f%% cpu=%.0f%% "
                     "gpu_wait_ms=%.0f fifo_wait_ms=%.0f draws=%llu retrace=%llu thermal=%d "
                     "audio_queue_ms=%d audio_waits=%llu audio_drops=%llu\n",
                     shown, (timing.display_copies - timing0.display_copies) / elapsed, worst_ms,
                     late, speed, (cpu - cpu0) / (elapsed * 1e4),
                     (timing.end_frame_us - timing0.end_frame_us) / 1000.0,
                     (timing.drain_us - timing0.drain_us) / 1000.0,
                     presents ? (timing.draws - timing0.draws) / presents : 0ull, retrace,
                     bluewake_thermal_state(), timing.audio_queued_ms,
                     timing.audio_throttles - timing0.audio_throttles,
                     timing.audio_dropped - timing0.audio_dropped);
    }
    window_start = now;
    frames = late = 0;
    worst_ms = 0.0;
    paused_in_window = false;
    retrace0 = retrace;
    cpu0 = cpu;
    timing0 = timing;
}

// BLUEWAKE_INPUT_LOG: one line per touch button press or release and per
// change of stick direction (eight directions and neutral), so the session
// log shows when the player acted; the host logs when the game read it.
bool input_log_enabled() {
    static const bool enabled = [] {
        const char* env = std::getenv("BLUEWAKE_INPUT_LOG");
        return env != nullptr && env[0] != '\0' && env[0] != '0';
    }();
    return enabled;
}

int stick_direction(int x, int y) {  // 0 neutral, 1..8 from east counterclockwise
    if (x * x + y * y < 40 * 40) return 0;
    static const int sectors[3][3] = {{6, 7, 8}, {5, 0, 1}, {4, 3, 2}};
    const int col = x > 2 * std::abs(y) / 5 ? 2 : (x < -2 * std::abs(y) / 5 ? 0 : 1);
    const int row = y > 2 * std::abs(x) / 5 ? 0 : (y < -2 * std::abs(x) / 5 ? 2 : 1);
    return sectors[row][col];
}

void log_touch_change(const BlueWakeTouchPad& before, const BlueWakeTouchPad& after) {
    static const char* const names[] = {"A", "B", "X", "Y", "Z", "L", "R", "START",
                                        "UP", "DOWN", "LEFT", "RIGHT"};
    static const unsigned bits[] = {BLUEWAKE_TOUCH_A, BLUEWAKE_TOUCH_B, BLUEWAKE_TOUCH_X,
                                    BLUEWAKE_TOUCH_Y, BLUEWAKE_TOUCH_Z, BLUEWAKE_TOUCH_L,
                                    BLUEWAKE_TOUCH_R, BLUEWAKE_TOUCH_START,
                                    BLUEWAKE_TOUCH_DPAD_UP, BLUEWAKE_TOUCH_DPAD_DOWN,
                                    BLUEWAKE_TOUCH_DPAD_LEFT, BLUEWAKE_TOUCH_DPAD_RIGHT};
    for (size_t i = 0; i < sizeof bits / sizeof bits[0]; ++i) {
        const bool was = (before.buttons & bits[i]) != 0, is = (after.buttons & bits[i]) != 0;
        if (was != is) std::fprintf(stderr, "[touch] %s %s\n", names[i], is ? "down" : "up");
    }
    const int d0 = stick_direction(before.stick_x, before.stick_y);
    const int d1 = stick_direction(after.stick_x, after.stick_y);
    if (d0 != d1) std::fprintf(stderr, "[touch] stick dir=%d (%d,%d)\n", d1, after.stick_x, after.stick_y);
    const int c0 = stick_direction(before.c_stick_x, before.c_stick_y);
    const int c1 = stick_direction(after.c_stick_x, after.c_stick_y);
    if (c0 != c1) std::fprintf(stderr, "[touch] cstick dir=%d\n", c1);
}

u16 to_pad_buttons(u16 bits) {
    u16 out = 0;
    if (bits & BLUEWAKE_TOUCH_DPAD_LEFT) out |= PAD_BUTTON_LEFT;
    if (bits & BLUEWAKE_TOUCH_DPAD_RIGHT) out |= PAD_BUTTON_RIGHT;
    if (bits & BLUEWAKE_TOUCH_DPAD_DOWN) out |= PAD_BUTTON_DOWN;
    if (bits & BLUEWAKE_TOUCH_DPAD_UP) out |= PAD_BUTTON_UP;
    if (bits & BLUEWAKE_TOUCH_Z) out |= PAD_TRIGGER_Z;
    if (bits & BLUEWAKE_TOUCH_R) out |= PAD_TRIGGER_R;
    if (bits & BLUEWAKE_TOUCH_L) out |= PAD_TRIGGER_L;
    if (bits & BLUEWAKE_TOUCH_A) out |= PAD_BUTTON_A;
    if (bits & BLUEWAKE_TOUCH_B) out |= PAD_BUTTON_B;
    if (bits & BLUEWAKE_TOUCH_X) out |= PAD_BUTTON_X;
    if (bits & BLUEWAKE_TOUCH_Y) out |= PAD_BUTTON_Y;
    if (bits & BLUEWAKE_TOUCH_START) out |= PAD_BUTTON_START;
    return out;
}

// Called with g_pad_mutex held.
void publish_locked() {
    const u16 bits = g_pad.buttons | g_latched;
    bluewake_jump_button_touch((bits & BLUEWAKE_TOUCH_JUMP) != 0);
    bluewake_sprint_touch((bits & BLUEWAKE_TOUCH_SPRINT) != 0);
    if (bits == 0 && g_pad.stick_x == 0 && g_pad.stick_y == 0 &&
        g_pad.c_stick_x == 0 && g_pad.c_stick_y == 0) {
        PADClearVirtualStatus(PAD_CHAN0);
        return;
    }
    PADStatus status{};
    status.button = to_pad_buttons(bits);
    // GameCube sticks read about +/-100 at full tilt.
    status.stickX = (s8)(g_pad.stick_x * 100 / 127);
    status.stickY = (s8)(g_pad.stick_y * 100 / 127);
    status.substickX = (s8)(g_pad.c_stick_x * 100 / 127);
    status.substickY = (s8)(g_pad.c_stick_y * 100 / 127);
    // Touch L/R are ordinary buttons: the digital click plus full pressure.
    if (bits & BLUEWAKE_TOUCH_L) status.triggerLeft = 255;
    if (bits & BLUEWAKE_TOUCH_R) status.triggerRight = 255;
    if (bits & BLUEWAKE_TOUCH_A) status.analogA = 255;
    if (bits & BLUEWAKE_TOUCH_B) status.analogB = 255;
    PADSetVirtualStatus(PAD_CHAN0, &status);
}

// iOS forbids GPU work from the background, and a game should stop when the
// app resigns active (app switcher, Control Center, a call). SDL reports those
// transitions synchronously to event watches from inside UIKit's callbacks.
// The memory card needs no flush: every guest card write replaces the file.
bool SDLCALL lifecycle_watch(void*, SDL_Event* e) {
    // iOS skips the desktop mouse observer; its existing event watch still
    // receives Space from a hardware keyboard.
    bluewake_jump_button_event(e);
    switch (e->type) {
    case SDL_EVENT_WILL_ENTER_BACKGROUND:
        bluewake_pause_set(BLUEWAKE_PAUSE_INACTIVE, true);
        bluewake_touch_clear();
        break;
    case SDL_EVENT_DID_ENTER_FOREGROUND:
        bluewake_pause_set(BLUEWAKE_PAUSE_INACTIVE, false);
        break;
    default:
        break;
    }
    return true;
}

bool hold_guest(void*) { return g_pause_reasons.load() != 0; }

// Runs once per presented frame on the main thread.
void frame_tick(void*) {
    static bool s_installed = false;
    if (!s_installed) {
        s_installed = true;
        SDL_AddEventWatch(lifecycle_watch, nullptr);
        bluewake_shell_install_overlay();
    }
    bluewake_settings_tick();
    fps_tick();
    std::lock_guard<std::mutex> lock(g_pad_mutex);
    if (g_latched == 0)
        return;
    const auto now = Clock::now();
    u16 expired = 0;
    for (int i = 0; i < 16; ++i)
        if ((g_latched & (1u << i)) && now - g_pressed_at[i] >= kMinHold)
            expired |= (u16)(1u << i);
    if (expired != 0) {
        g_latched &= (u16)~expired;
        publish_locked();
    }
}

}  // namespace

extern "C" void bluewake_touch_publish(const BlueWakeTouchPad* pad) {
    std::lock_guard<std::mutex> lock(g_pad_mutex);
    const auto now = Clock::now();
    const u16 pressed = (u16)(pad->buttons & ~g_pad.buttons);
    const u16 released = (u16)(g_pad.buttons & ~pad->buttons);
    for (int i = 0; i < 16; ++i)
        if (pressed & (1u << i))
            g_pressed_at[i] = now;
    for (int i = 0; i < 16; ++i)
        if ((released & (1u << i)) && now - g_pressed_at[i] < kMinHold)
            g_latched |= (u16)(1u << i);
    g_latched &= (u16)~pressed;
    if (input_log_enabled()) log_touch_change(g_pad, *pad);
    g_pad = *pad;
    publish_locked();
}

extern "C" void bluewake_touch_clear(void) {
    std::lock_guard<std::mutex> lock(g_pad_mutex);
    g_pad = BlueWakeTouchPad{};
    g_latched = 0;
    bluewake_jump_button_touch(false);
    bluewake_sprint_touch(false);
    PADClearVirtualStatus(PAD_CHAN0);
}

extern "C" void bluewake_pause_set(unsigned reason, bool on) {
    const unsigned before = on ? g_pause_reasons.fetch_or(reason)
                               : g_pause_reasons.fetch_and(~reason);
    const unsigned after = on ? (before | reason) : (before & ~reason);
    if ((before == 0) != (after == 0))
        std::fprintf(stderr, "[ios] %s (reasons 0x%x)\n",
                     after != 0 ? "pausing" : "resuming", after != 0 ? after : before);
}

extern "C" unsigned bluewake_pause_reasons(void) { return g_pause_reasons.load(); }

extern "C" float bluewake_fps_display(void) { return aurora_get_fps(); }
extern "C" float bluewake_fps_game(void) { return g_fps_game.load(); }
extern "C" bool bluewake_fps_smooth_paused(void) { return g_fps_smooth_paused.load(); }

extern "C" void bluewake_fps_read(float* shown, float* speed, float* worst_ms) {
    *shown = g_fps_shown.load();
    *speed = g_fps_speed.load();
    *worst_ms = g_fps_worst_ms.load();
}

extern "C" void bluewake_touch_controls_install(void) {
    dol_aurora_set_overlay(frame_tick, nullptr);
    dol_aurora_set_hold(hold_guest, nullptr);
    std::fprintf(stderr, "[ios] shell installed\n");
}
