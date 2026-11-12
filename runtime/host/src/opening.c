#include "opening.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// GZLE01. The current 2D port, g_dComIfG_gameInfo.play.mCurrentGrafPort
// (0x803C4C08 + 0x12A0 + 0x4A60): the port proc_draw itself set up, which the
// widescreen code widens with the screen (mDoGph_Painter).
enum { kCurrentGrafPort = 0x803CA908u };
// The camera aspect constant (.sdata2): 4:3, or what the widescreen code wrote.
enum { kCameraAspect = 0x803FA998u };
static const float kAspect43 = 4.0f / 3.0f;

bool bluewake_opening_armed;
static bool g_wide;
static bool g_trace;

static float read_f32(CPUState* cpu, u32 address) {
    const u32 bits = mem_read32(cpu, address);
    float value;
    memcpy(&value, &bits, sizeof value);
    return value;
}

void bluewake_opening_reload(void) {
    const char* on = getenv("BLUEWAKE_NATIVE_CUTSCENE_FRAMING");
    bluewake_opening_armed = g_wide && on != NULL && on[0] == '1';
}

void bluewake_opening_attach(CPUState* cpu) {
    const char* trace = getenv("BLUEWAKE_LEGEND_TRACE");
    g_trace = trace != NULL && trace[0] == '1';
    const float aspect = read_f32(cpu, kCameraAspect);
    // Only the widescreen codes take the port away.
    g_wide = aspect > kAspect43 + 0.01f && aspect < 4.0f;
    bluewake_opening_reload();
    if (g_trace)
        fprintf(stderr, "[legend] aspect %.3f: %s\n", aspect,
                bluewake_opening_armed ? "drawn with the game's port" : "as the game draws it");
}

void bluewake_opening_hook(CPUState* cpu) {
    if (cpu == NULL || cpu->lr != BLUEWAKE_OPENING_DRAW_RETURN || cpu->gpr[4] != 0u)
        return;
    const u32 port = mem_read32(cpu, kCurrentGrafPort);
    if (port < 0x80000000u || port >= 0x81800000u)
        return;
    cpu->gpr[4] = port;
    static bool logged;
    if (g_trace && !logged) {
        logged = true;
        fprintf(stderr, "[legend] draws with port %08X, box x %.1f..%.1f\n", port, read_f32(cpu, port + 0xBCu),
                read_f32(cpu, port + 0xC4u));
    }
}
