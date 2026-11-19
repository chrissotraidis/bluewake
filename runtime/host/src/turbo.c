#include "turbo.h"

#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_keyboard.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The guest's clock is two clocks, and the slower one decides the speed.
//
// host_wall_pace (main.c) holds each retrace to 1001/60000 s; dividing that is
// how the pace itself changes, so the retraces stay in sequence and every
// per-retrace host feature keeps running. The other clock is the sound: every
// chunk the guest's DMA hands over is pushed into a queue that sleeps while it
// is fuller than DOL_AUDIO_MAX_QUEUE_MS, so pushing faster than realtime pins
// the game at realtime whatever the pacer does.
//
// The DMA's cadence and its AID interrupt are not in the push, so while the key
// is held this module's caller advances the DMA itself and skips the push:
// guest timing stays exact, the sound is simply not made. The device queue
// drains while that happens, so the first moment after the key is let go plays
// from an empty queue — one audible seam, not a bug.
//
// A factor of 1 (or a switch that is off) leaves every path exactly as it was.
enum {
    kDefaultPercent = 300, // three times realtime, when the file says nothing
    kMaxPercent = 500,
};

static bool g_on;      // BLUEWAKE_TURBO
static double g_factor = 3.0;
// BLUEWAKE_TURBO_TEST=retrace:length (testing only): held meanwhile, so the
// speed-up can be timed without a keyboard.
static unsigned long long g_test_start, g_test_length;
static bool g_held;    // Tab, or a controller's right stick, down this retrace
static bool g_was_held;
static unsigned long long g_retrace;

static bool env_on(const char* name) {
    const char* value = getenv(name);
    return value != NULL && value[0] != '\0' && strcmp(value, "0") != 0;
}

void bluewake_turbo_reload(void) {
    g_on = env_on("BLUEWAKE_TURBO");
    const char* speed = getenv("BLUEWAKE_TURBO_SPEED");
    g_factor = speed != NULL && speed[0] != '\0' ? strtod(speed, NULL) : 3.0;
    if (!(g_factor >= 1.0)) // also catches the NaN a bad value would give
        g_factor = 1.0;
    if (g_factor * 100.0 > (double)kMaxPercent)
        g_factor = (double)kMaxPercent / 100.0;
    if (!g_on || g_factor <= 1.0)
        g_held = false;
    const char* test = getenv("BLUEWAKE_TURBO_TEST");
    if (test != NULL && sscanf(test, "%llu:%llu", &g_test_start, &g_test_length) != 2)
        g_test_length = 0u;
}

void bluewake_turbo_attach(void) {
    bluewake_turbo_reload();
    if (g_on && g_factor > 1.0)
        fprintf(stderr,
                "[turbo] hold Tab (or the right stick's click) to run %.2fx faster; "
                "the sound is skipped while it is held\n",
                g_factor);
}

static bool tab_held(void) {
    int count = 0;
    const bool* keys = SDL_GetKeyboardState(&count);
    return keys != NULL && count > SDL_SCANCODE_TAB && keys[SDL_SCANCODE_TAB];
}

// Any connected controller's right stick click. A GameCube pad has no such
// button, so it collides with nothing the game reads.
static bool pad_held(void) {
    int count = 0;
    SDL_JoystickID* ids = SDL_GetGamepads(&count);
    bool held = false;
    for (int i = 0; ids != NULL && i < count && !held; ++i) {
        SDL_Gamepad* pad = SDL_GetGamepadFromID(ids[i]);
        if (pad != NULL && SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_RIGHT_STICK))
            held = true;
    }
    SDL_free(ids);
    return held;
}

void bluewake_turbo_retrace(void) {
    ++g_retrace;
    if (!g_on || g_factor <= 1.0) {
        g_held = false;
        return;
    }
    g_held = tab_held() || pad_held() ||
             (g_retrace >= g_test_start && g_retrace < g_test_start + g_test_length);
    if (g_held != g_was_held) {
        g_was_held = g_held;
        fprintf(stderr, "[turbo] %s retrace=%llu %.2fx\n", g_held ? "on" : "off", g_retrace, g_factor);
    }
}

bool bluewake_turbo_active(void) {
    return g_on && g_factor > 1.0 && g_held;
}

u32 bluewake_turbo_percent(void) {
    if (!bluewake_turbo_active())
        return 100u;
    const u32 percent = (u32)(g_factor * 100.0 + 0.5);
    return percent > 100u ? percent : 100u;
}
