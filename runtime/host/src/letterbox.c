#include "letterbox.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// GZLE01. The camera process (camera_process_class): its view_class's fovy
// (degrees) and aspect, and its dCamera_c (at 0x244)'s trim.
enum {
    kViewFovy = 0xD0u,
    kViewAspect = 0xD4u,
    kTrimHeight = 0x244u + 0x5F8u, // mCamera.mTrimHeight (f32, picture lines at top and bottom)
    kTrimSize = 0x244u + 0x5FCu,   // mCamera.mTrimSize: 0 none, 1 vista, 2 cinema, 3 the telescope's, 4 none at once
};
enum { kTrimTelescope = 3 };
static const float kEfbLines = 480.0f; // camera_draw's scissor: 0..getEfbHeight(), 480 lines
static const float kPi = 3.14159265f;

bool bluewake_letterbox_armed;
static bool g_on;
static bool g_trace;
// What camera_draw's entry changed, to put back at the next camera_execute.
static bool g_held;
static u32 g_camera;
static u32 g_trim_bits, g_fovy_bits, g_fovy_written;

static float read_f32(CPUState* cpu, u32 address) {
    const u32 bits = mem_read32(cpu, address);
    float value;
    memcpy(&value, &bits, sizeof value);
    return value;
}

static u32 f32_bits(float value) {
    u32 bits;
    memcpy(&bits, &value, sizeof bits);
    return bits;
}

static void rearm(void) { bluewake_letterbox_armed = g_on || g_held; }

void bluewake_letterbox_reload(void) {
    const char* on = getenv("BLUEWAKE_NATIVE_CUTSCENE_FRAMING");
    g_on = on != NULL && on[0] == '1';
    rearm();
}

void bluewake_letterbox_attach(CPUState* cpu) {
    (void)cpu;
    const char* trace = getenv("BLUEWAKE_LETTERBOX_TRACE");
    g_trace = trace != NULL && trace[0] == '1';
    bluewake_letterbox_reload();
    if (g_on)
        fprintf(stderr, "[letterbox] off: cutscenes fill the screen\n");
}

// The state's memory has its own camera: nothing of the old one goes back.
void bluewake_letterbox_state_loaded(void) {
    g_held = false;
    rearm();
}

static void put_back(CPUState* cpu) {
    mem_write32(cpu, g_camera + kTrimHeight, g_trim_bits);
    // Only if nothing else has set the view's fovy since.
    if (mem_read32(cpu, g_camera + kViewFovy) == g_fovy_written)
        mem_write32(cpu, g_camera + kViewFovy, g_fovy_bits);
    g_held = false;
    rearm();
}

// A save state is about to be written: it keeps the game's own values, so it
// loads the same with the switch either way (the next camera_draw takes the
// bars away again).
void bluewake_letterbox_state_saving(CPUState* cpu) {
    if (cpu != NULL && g_held)
        put_back(cpu);
}

static void take_away(CPUState* cpu, u32 camera) {
    const float trim = read_f32(cpu, camera + kTrimHeight);
    if (!(trim > 0.5f && trim < kEfbLines * 0.5f) || (int)mem_read32(cpu, camera + kTrimSize) == kTrimTelescope)
        return;
    const float fovy = read_f32(cpu, camera + kViewFovy), aspect = read_f32(cpu, camera + kViewAspect);
    // Only in widescreen: at 4:3 the game's bars stay.
    if (!(fovy > 1.0f && fovy < 179.0f && aspect > 4.0f / 3.0f + 0.01f && aspect < 4.0f))
        return;
    // The band between the bars, as tall as the screen; but never so tall a
    // view that the picture is narrower than 4:3's.
    const float band = (kEfbLines - 2.0f * trim) / kEfbLines;
    const float scale = fmaxf(band, (4.0f / 3.0f) / aspect);
    const float narrowed = 2.0f * atanf(tanf(fovy * 0.5f * kPi / 180.0f) * fminf(scale, 1.0f)) * 180.0f / kPi;
    g_camera = camera;
    g_trim_bits = f32_bits(trim);
    g_fovy_bits = f32_bits(fovy);
    g_fovy_written = f32_bits(narrowed);
    g_held = true;
    rearm();
    mem_write32(cpu, camera + kTrimHeight, f32_bits(0.0f));
    mem_write32(cpu, camera + kViewFovy, g_fovy_written);
    if (g_trace)
        fprintf(stderr, "[letterbox] trim=%.1f size=%d aspect=%.3f fovy %.2f -> %.2f\n", trim,
                (int)mem_read32(cpu, camera + kTrimSize), aspect, fovy, narrowed);
}

void bluewake_letterbox_hook(CPUState* cpu, u32 address) {
    if (cpu == NULL)
        return;
    if (address == BLUEWAKE_LETTERBOX_CAMERA_EXECUTE) {
        if (g_held)
            put_back(cpu);
        return;
    }
    // camera_draw: a frame drawn twice without an update between keeps the
    // values held first.
    if (g_held)
        put_back(cpu);
    const u32 camera = cpu->gpr[3];
    if (g_on && camera >= 0x80000000u && camera < 0x81800000u)
        take_away(cpu, camera);
}
