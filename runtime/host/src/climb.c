#include "climb.h"

#include <math.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// GZLE01. The player (daPy_lk_c) is at kPlayerPointer; its fields:
enum {
    kPlayerPointer = 0x803CA74Cu,
    kCurrentPos = 0x1F8u,     // fopAc_ac_c::current.pos
    kNoResetFlg0 = 0x29Cu,    // daPyFlg0_UNK100 (0x100): the game looks for ivy at all
    kNoResetFlg1 = 0x2A0u,    // daPyFlg1_VINE_CATCH (0x02000000): just let go of a wall
    kLinkLinChkPoly = 0x644u, // mLinkLinChk (0x630)'s cBgS_PolyInfo data (+0x14, 12 bytes)
    kPolyInfo = 0x940u,       // mPolyInfo's data (12 bytes; its vtable follows)
    kCurProc = 0x31D8u,       // mCurProc
    kFrontWallType = 0x34B9u, // mFrontWallType (u8): 1 a plain wall or none, 3 ivy
    kStickDistance = 0x35B0u, // mStickDistance (0..1)
    kLavaHeight = 0x35D4u,    // m35D4: lava or water under him (-inf when none)
    kModeFlg = 0x3618u,       // mModeFlg: MIDAIR 0x2, SWIM 0x40000
};
enum {
    kFlg0LooksForIvy = 0x100u,
    kFlg1VineCatch = 0x02000000u,
    kModeMidair = 0x2u,
    kModeSwim = 0x40000u,
    kProcClimbFirst = 0x3D, // CLIMB_UP_START, CLIMB_DOWN_START, CLIMB_MOVE_UP_DOWN,
    kProcClimbLast = 0x40,  // CLIMB_MOVE_SIDE
    kWallTypePlain = 1,
    kWallTypeIvy = 3,
};
// Where the hooks are (see climb.h's dispatch). Only calls into another
// translation unit come back through the dispatcher, so the hooks are the
// return sites of setFrontWallType's and setMoveBGCorrectClimb's collision
// queries (dBgS / cBgS), not their own entries.
enum {
    kFrontWallCode = 0x8010F0DCu,  // setFrontWallType's GetWallCode (0x8010F0D8) returns: r3 the code
    kFrontWallAbove = 0x8010F554u, // its LineCross at grabbing height (0x8010F550) returns: r3 a hit
    kClimbWallCode = 0x80135FE4u,  // setMoveBGCorrectClimb's GetWallCode (0x80135FE0) returns
    kPlayerExecute = 0x80122D30u,  // daPy_Execute__FP9daPy_lk_c: r3 the player
    kCameraDraw = 0x8017C350u,     // camera_draw: r3 the camera process (a view_class)
};
// view_class: fovy, aspect, then the eye and centre it is drawn from.
enum { kViewFovy = 0xD0u, kViewAspect = 0xD4u, kViewEye = 0xD8u, kViewCenter = 0xE4u };

static const double kFrameSeconds = 1.0 / 30.0; // daPy_Execute runs once a game frame
static const double kHangShare = 0.4;           // holding still costs this share
static const double kRefillSeconds = 3.0;       // empty to full on the ground
static const unsigned kRefillDelayFrames = 15;  // on the ground this long first

bool bluewake_climb_on;
static double g_seconds = 12.0; // a full wheel
static double g_stamina = 1.0;  // 0..1
static bool g_exhausted;        // ran out: no grabbing until full again
static bool g_candidate;        // this frame's classification found a plain wall
static u8 g_candidate_poly[12]; // ... and the collision it hit
static unsigned long long g_frame, g_plain_climb_frame;
static unsigned g_ground_frames;
static u32 g_last_proc;
static bool g_trace;

// The wheel, for the overlay's thread.
static _Atomic float g_hud_stamina = 1.0f, g_hud_x, g_hud_y, g_hud_aspect = 4.0f / 3.0f, g_hud_alpha;
static _Atomic bool g_hud_exhausted, g_hud_in_view;
static float g_alpha;

static float read_f32(CPUState* cpu, u32 address) {
    const u32 bits = mem_read32(cpu, address);
    float value;
    memcpy(&value, &bits, sizeof value);
    return value;
}

static bool guest_pointer(u32 address) { return address >= 0x80000000u && address < 0x81800000u; }

static void read_settings(void) {
    const char* on = getenv("BLUEWAKE_CLIMB");
    bluewake_climb_on = on != NULL && on[0] == '1';
    const char* seconds = getenv("BLUEWAKE_CLIMB_STAMINA");
    g_seconds = seconds != NULL && atof(seconds) >= 1.0 ? atof(seconds) : 12.0;
}

void bluewake_climb_reload(void) {
    read_settings();
    if (!bluewake_climb_on) {
        g_stamina = 1.0;
        g_exhausted = false;
        g_alpha = 0.0f;
        atomic_store(&g_hud_alpha, 0.0f);
    }
}

void bluewake_climb_attach(CPUState* cpu) {
    (void)cpu;
    read_settings();
    const char* trace = getenv("BLUEWAKE_CLIMB_TRACE");
    g_trace = trace != NULL && trace[0] == '1';
    if (bluewake_climb_on)
        fprintf(stderr, "[climb] Link climbs any wall, with %.0f seconds of stamina\n", g_seconds);
}

static bool can_grab(void) { return !g_exhausted && g_stamina > 0.0; }

// Once a game frame, at Link's update: the wheel drains while he climbs a
// plain wall and refills once he has stood on the ground a moment.
static void player_frame(CPUState* cpu, u32 player) {
    g_frame++;
    const u32 proc = mem_read32(cpu, player + kCurProc);
    const bool climbing = proc >= kProcClimbFirst && proc <= kProcClimbLast;
    // setMoveBGCorrectClimb kept him on a plain wall in his last update.
    const bool plain = climbing && g_frame - g_plain_climb_frame <= 2u;
    const u32 mode = mem_read32(cpu, player + kModeFlg);
    const double before = g_stamina;
    if (plain) {
        const bool still = read_f32(cpu, player + kStickDistance) <= 0.05f;
        g_stamina -= (still ? kHangShare : 1.0) * kFrameSeconds / g_seconds;
        g_ground_frames = 0;
        if (g_stamina <= 0.0) {
            g_stamina = 0.0;
            g_exhausted = true;
            if (g_trace)
                fprintf(stderr, "[climb] frame=%llu out of stamina: letting go\n", g_frame);
        }
    } else if (!climbing && (mode & (kModeMidair | kModeSwim)) == 0u) {
        if (++g_ground_frames > kRefillDelayFrames && g_stamina < 1.0) {
            g_stamina += kFrameSeconds / kRefillSeconds;
            if (g_stamina >= 1.0) {
                g_stamina = 1.0;
                if (g_exhausted && g_trace)
                    fprintf(stderr, "[climb] frame=%llu stamina full again\n", g_frame);
                g_exhausted = false;
            }
        }
    } else {
        g_ground_frames = 0;
    }
    if (g_trace && (proc != g_last_proc || (plain && (int)(before * 10.0) != (int)(g_stamina * 10.0))))
        fprintf(stderr, "[climb] frame=%llu proc=0x%X plain=%d stamina=%.2f%s wheel=%.2f,%.2f%s\n", g_frame, proc,
                plain ? 1 : 0, g_stamina, g_exhausted ? " (exhausted)" : "", atomic_load(&g_hud_x),
                atomic_load(&g_hud_y), atomic_load(&g_hud_in_view) ? "" : " (out of view)");
    g_last_proc = proc;

    // The wheel shows while it is not full, and fades a second after.
    const float target = plain || g_stamina < 1.0 ? 1.0f : 0.0f;
    g_alpha += (target - g_alpha) * (target > g_alpha ? 0.5f : 0.06f);
    if (g_alpha < 0.01f)
        g_alpha = 0.0f;
    atomic_store(&g_hud_stamina, (float)g_stamina);
    atomic_store(&g_hud_exhausted, g_exhausted);
    atomic_store(&g_hud_alpha, g_alpha);
}

// At camera_draw: where Link's shoulder is in this frame's picture.
static void camera_frame(CPUState* cpu, u32 view) {
    const u32 player = mem_read32(cpu, kPlayerPointer);
    if (!guest_pointer(player) || g_alpha == 0.0f)
        return;
    const float fovy = read_f32(cpu, view + kViewFovy), aspect = read_f32(cpu, view + kViewAspect);
    if (!(fovy > 1.0f && fovy < 179.0f && aspect > 0.5f && aspect < 4.0f))
        return;
    float eye[3], center[3], pos[3];
    for (int i = 0; i < 3; ++i) {
        eye[i] = read_f32(cpu, view + kViewEye + 4u * (u32)i);
        center[i] = read_f32(cpu, view + kViewCenter + 4u * (u32)i);
        pos[i] = read_f32(cpu, player + kCurrentPos + 4u * (u32)i);
    }
    pos[1] += 90.0f;
    // A look-at frame: forward, right (y up), up.
    float f[3] = {center[0] - eye[0], center[1] - eye[1], center[2] - eye[2]};
    float n = sqrtf(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]);
    if (n < 1e-3f)
        return;
    for (int i = 0; i < 3; ++i)
        f[i] /= n;
    float r[3] = {f[1] * 0.0f - f[2] * 1.0f, f[2] * 0.0f - f[0] * 0.0f, f[0] * 1.0f - f[1] * 0.0f};
    n = sqrtf(r[0] * r[0] + r[1] * r[1] + r[2] * r[2]);
    if (n < 1e-3f)
        return;
    for (int i = 0; i < 3; ++i)
        r[i] /= n;
    const float u[3] = {r[1] * f[2] - r[2] * f[1], r[2] * f[0] - r[0] * f[2], r[0] * f[1] - r[1] * f[0]};
    const float d[3] = {pos[0] - eye[0], pos[1] - eye[1], pos[2] - eye[2]};
    const float z = d[0] * f[0] + d[1] * f[1] + d[2] * f[2];
    if (z < 10.0f) {
        atomic_store(&g_hud_in_view, false);
        return;
    }
    const float t = tanf(fovy * 0.5f * 3.14159265f / 180.0f);
    const float x = (d[0] * r[0] + d[1] * r[1] + d[2] * r[2]) / (z * t * aspect);
    const float y = (d[0] * u[0] + d[1] * u[1] + d[2] * u[2]) / (z * t);
    atomic_store(&g_hud_x, 0.5f + 0.5f * x);
    atomic_store(&g_hud_y, 0.5f - 0.5f * y);
    atomic_store(&g_hud_aspect, aspect);
    atomic_store(&g_hud_in_view, fabsf(x) < 1.2f && fabsf(y) < 1.2f);
}

bool bluewake_climb_hud(float* fraction, bool* exhausted, float* x, float* y, float* aspect, float* alpha) {
    if (!bluewake_climb_on)
        return false;
    *alpha = atomic_load(&g_hud_alpha);
    if (*alpha <= 0.0f)
        return false;
    *fraction = atomic_load(&g_hud_stamina);
    *exhausted = atomic_load(&g_hud_exhausted);
    *aspect = atomic_load(&g_hud_aspect);
    if (atomic_load(&g_hud_in_view)) {
        *x = atomic_load(&g_hud_x);
        *y = atomic_load(&g_hud_y);
    } else {
        *x = 0.5f; // Link out of the picture: its middle
        *y = 0.45f;
    }
    return true;
}

void bluewake_climb_hook(CPUState* cpu, u32 address) {
    if (cpu == NULL)
        return;
    switch (address) {
    case kFrontWallCode: {
        // The game found a steep wall Link faces and asks what it is: a plain
        // wall is a candidate, with the collision it hit (the checks after
        // this reuse the line check).
        const u32 player = mem_read32(cpu, kPlayerPointer);
        g_candidate = cpu->gpr[3] == 0u && guest_pointer(player);
        if (g_candidate)
            for (u32 i = 0; i < sizeof g_candidate_poly; ++i)
                g_candidate_poly[i] = mem_read8(cpu, player + kLinkLinChkPoly + i);
        break;
    }
    case kFrontWallAbove: {
        // A plain wall that also stands at the height Link grabs ledges at:
        // not a ledge to pull himself onto. The game would leave it a plain
        // wall, or one to sidle along (which it still decides after this, and
        // which then wins): it becomes ivy, under the conditions the game puts
        // on ivy, and in the air only while he is steered at it.
        const bool candidate = g_candidate;
        g_candidate = false;
        if (!candidate || cpu->gpr[3] == 0u || !can_grab())
            break;
        const u32 player = mem_read32(cpu, kPlayerPointer);
        if (!guest_pointer(player) || mem_read8(cpu, player + kFrontWallType) != kWallTypePlain)
            break;
        const u32 mode = mem_read32(cpu, player + kModeFlg);
        if ((mem_read32(cpu, player + kNoResetFlg0) & kFlg0LooksForIvy) == 0u ||
            (mem_read32(cpu, player + kNoResetFlg1) & kFlg1VineCatch) != 0u)
            break;
        if (read_f32(cpu, player + kCurrentPos + 4u) - read_f32(cpu, player + kLavaHeight) < 125.0f)
            break;
        if ((mode & kModeMidair) != 0u && read_f32(cpu, player + kStickDistance) <= 0.05f)
            break;
        for (u32 i = 0; i < sizeof g_candidate_poly; ++i)
            mem_write8(cpu, player + kPolyInfo + i, g_candidate_poly[i]);
        mem_write8(cpu, player + kFrontWallType, kWallTypeIvy);
        if (g_trace)
            fprintf(stderr, "[climb] frame=%llu grabs a wall%s (stamina %.2f)\n", g_frame,
                    (mode & kModeMidair) != 0u ? " in the air" : "", g_stamina);
        break;
    }
    case kClimbWallCode:
        // Each climbing frame: a plain wall stays ivy while there is stamina.
        if (cpu->gpr[3] == 0u && can_grab()) {
            cpu->gpr[3] = 1u;
            g_plain_climb_frame = g_frame;
        }
        break;
    case kPlayerExecute: {
        const u32 player = cpu->gpr[3];
        g_candidate = false;
        if (player == mem_read32(cpu, kPlayerPointer) && guest_pointer(player))
            player_frame(cpu, player);
        break;
    }
    case kCameraDraw:
        if (guest_pointer(cpu->gpr[3]))
            camera_frame(cpu, cpu->gpr[3]);
        break;
    default:
        break;
    }
}
