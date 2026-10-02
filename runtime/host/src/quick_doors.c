#include "quick_doors.h"

#include "fast_load.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Doors with a knob (d_a_knob00: houses, shops and the other interiors). Every
// one leads to another stage, and all 149 stages with one use the same two
// events for it. Going through one takes about 5 seconds from the press of A
// to Link moving again (Link's house, with fast_load.c's short fades):
//
// 1. The door's event (DEFAULT_KNOB_DOOR_F_OPEN, or _B_OPEN from the other
//    side): Link puts his sword away and turns to the door, then opens it and
//    walks through (daPy_lk_c::dProcDoorOpen, proc 0xC1: his door animation
//    moves him, root motion) while the door plays its own. At the door's frame
//    15 it fades the screen to white or black itself (mDoGph_gInf_c::fadeOut
//    at 0.05 a frame, so fully covered 20 frames later, at Link's frame 34).
// 2. Covered, Link keeps walking until he steps onto the exit polygon behind
//    the door, 27 frames later (his frame 61). daPy_lk_c::checkFallCode then
//    asks for the scene change with the exit's wipe, 10 or 11: OVERLAP8, a
//    dOvlpFd that does not fade anything itself but counts 26 frames (the
//    door's fade covers the screen) before the old scene goes.
// 3. The black while the new scene is made (fast_load.c runs it fast).
// 4. The new stage's start point for the door has start mode 0xA (or 0xB), so
//    daPy_lk_c::playerInit starts KNOB_START (or KNOB_START_B): the door is
//    open, Link walks in (his door animation from frame 35) and it closes
//    behind him, about 50 frames before he can move.
//
// BLUEWAKE_QUICK_DOORS (on unless 0) keeps 1 as it is and cuts the rest:
//   - once the door's fade covers the whole screen, Link's door animation runs
//     kWalkRate times as fast, so he reaches the exit polygon in a few frames
//     (his path is the same, in bigger steps; the exit polygons behind knob
//     doors are about 150 units deep and where his animation ends is on them);
//   - once the door's fade covers the screen and the scene change is asked
//     for, OVERLAP8's 26-frame count is ended, so the black starts at once;
//   - the new stage's player is made as at an ordinary start point (start
//     mode 0, the stage's DEFAULT_START, as when a save is loaded) where
//     KNOB_START would have left him: the same way round, kArriveFront (0xA)
//     or kArriveBack (0xB) units on from the point, which is in the doorway.
//     The door was never opened, so it stands closed (and solid). He can move
//     as the picture comes back, and his restart point is where he stands.
// That takes about 2 seconds: 1.5 of the door opening as before, then 0.1 of
// the covered screen settling, the black (fast-forwarded), and the picture
// coming back with Link free to move.
// Only knob doors do any of this: other doors (dungeon shutters, boss doors,
// sliding doors) have no fade and no scene change of this kind, and a door that
// talks or is locked never starts the fade.
//
//   BLUEWAKE_QUICK_DOORS=0              the game's doors
//   BLUEWAKE_DOOR_TRACE=1               log what is cut, and each door's timing
//   BLUEWAKE_DOOR_TRACE=2               also log state every retrace
//   BLUEWAKE_DOOR_TEST_FACE=r:x:z,...   testing: turn Link to face (x, z) at retrace r
enum {
    kPlayerPointer = 0x803CA74Cu,
    kEventMode = 0x803C9EA2u,    // g_dComIfG_gameInfo.play.mEvtCtrl.mMode
    kEventId = 0x803C9EB8u,      // mEvtCtrl.mEventId (s16)
    kEventList = 0x803C9ED4u,    // play.mEvtManager.mList (dEvDtBase_c)
    kExceptionIdx = 0x803C9EF8u, // mEvtManager.mException.mEventInfoIdx
    kExceptionState = 0x803C9F00u,
    kStartStage = 0x803C9D3Cu, // play.mCurStage (dStage_startStage_c): name[8], s16 point
    kNextStage = 0x803C9D48u,  // play.mNextStage: +0xC enabled, +0xD wipe
    kOverlap = 0x803F6160u,    // l_fopOvlpM_overlap[0] (overlap_request_class*)
    kFader = 0x803F6898u,      // mDoGph_gInf_c::mFader (JUTFader*)
    kGphFade = 0x803F68ABu,      // mDoGph_gInf_c::mFade (the door's fade is on)
    kGphFadeRate = 0x803F68ACu,  // mDoGph_gInf_c::mFadeRate (1: covered)
    kGphFadeSpeed = 0x803F68B0u, // mDoGph_gInf_c::mFadeSpeed
    kGphFadeColor = 0x803F6104u, // mDoGph_gInf_c::mFadeColor (RGBA)
    kRestart = 0x803C5D30u,      // g_dComIfG_gameInfo.info.mRestart (dSv_restart_c)
    kCamera = 0x803CA718u,       // the player's camera (camera_class*): mLookat eye 0xD8, center 0xE4
    kKeptAngleY = 0x803F6F12u,   // l_debug_current_angle.y (daPy_lk_c::execute starts from these)
    kKeptShapeY = 0x803F6F1Au,   // l_debug_shape_angle.y

    // dStage_playerInit (0x80041AF4-0x80041E84) makes the player with
    // dStage_actorCreate (BLUEWAKE_QUICK_DOORS_ACTOR_CREATE)(player_data,
    // append): r4 is the fopAcM_prm_class (parameters, position, angle) the
    // player is made from.
    kPlayerInit = 0x80041AF4u,
    kPlayerInitEnd = 0x80041E84u,
    kAppendParams = 0x00u,
    kAppendPos = 0x04u,
    kAppendAngleY = 0x12u,

    // overlap_request_class / overlap1_class (dOvlpFd)
    kRequestTask = 0x20u,
    kProcName = 0x08u,
    kFadeInTime = 0xD0u, // the frames left going black
    kFaderStatus = 0x04u,
    kFaderTimer = 0x0Au,       // JUTFader::mTimer (u16)
    kFaderDelayTimer = 0x20u,  // JUTFader::mDelayTimer: controls until mDelayStatus
    kFaderDelayStatus = 0x24u, // JUTFader::mDelayStatus
    kFaderWaitOut = 0,    // JUTFader: covering the picture
    kFaderWaitIn = 1,     // JUTFader: the fader is not covering the picture
    kFaderFadeIn = 2,     // the picture coming back
    kLastFadeOverlap = 4, // fpcNm_OVERLAP0/1/6/7/8 are 0-4, all dOvlpFd

    // fopAc_ac_c / daPy_lk_c
    kPos = 0x1F8u,
    kAngleY = 0x206u,       // current.angle.y
    kShapeAngleY = 0x20Eu,  // shape_angle.y
    kDemoMode = 0x314u,
    kUnderRate = 0x3038u,   // mFrameCtrlUnder[UNDER_MOVE0].mRate (his root motion)
    kUnderFrame = 0x303Cu,  // mFrameCtrlUnder[UNDER_MOVE0].mFrame
    kUpperRate = 0x3060u,   // mFrameCtrlUpper[UPPER_MOVE0].mRate
    kCurProc = 0x31D8u,
    kTargetAngle = 0x34DEu, // m34DE
    kProcDoorOpen = 0xC1u,  // daPyProc_DEMO_DOOR_OPEN_e
    kProcWait = 0x04u, kProcFreeWait = 0x05u, kProcMove = 0x06u,
};

// How much faster Link's door animation runs under the covered screen: from
// his frame 34 to its end (frame 78) in under 3 frames instead of 44. Across
// every stage's knob doors the end of his walk is on the exit polygon behind
// the door, so the exit is taken however big the steps (at worst where the
// animation ends, which is where the game's walk ends too).
static const float kWalkRate = 16.0f;
// Where KNOB_START leaves Link: straight on from the start point (the doorway),
// facing the way it faces (measured at Link's house, both sides: 74.8 and 84.2
// units, and within 2 units of the line).
static const float kArriveFront = 75.0f; // start mode 0xA, KNOB_START
static const float kArriveBack = 84.0f;  // start mode 0xB, KNOB_START_B
// The picture comes back this many frames later after such a start. The
// player's camera is made behind Link, and the door's wall is right behind
// him: outside Link's house its first frame still had the eye inside the wall
// and only its second was in front of it (KNOB_START's camera is a fixed one
// out in the room, so this never shows in the game's own arrival).
static const int kArriveHold = 2;

bool bluewake_quick_doors_armed;
static bool g_enabled = true;
static int g_trace;
static CPUState* g_cpu;
static unsigned long long g_retrace;

// BLUEWAKE_DOOR_TRACE's timing of each door: the retrace its event started,
// the one the screen was covered and the one Link could move again.
static unsigned long long g_door_start, g_door_covered, g_door_ff;
static bool g_door_swapped;
static bool g_walk_sped; // this door's walk already sped up
static bool g_wait_cut;  // this door's overlap count already ended
// From the walk sped up until the new scene takes the door's fade away: the
// screen is covered, and fast_load.c may run the game as fast as it can.
static bool g_exiting;
// A start point made ordinary, until the picture starts coming back.
static bool g_arrived;

bool bluewake_quick_doors_covered(void) { return g_exiting; }

// A door under way keeps its progress here (and may have changed guest state it
// changes back later), so a save state waits until none is.
bool bluewake_quick_doors_busy(void) {
    return g_door_start != 0u || g_exiting || g_arrived || bluewake_quick_doors_armed;
}

// BLUEWAKE_DOOR_TEST_FACE=retrace:x:z,... (testing only): at that retrace Link
// is turned to face the point (x, z), so a press of A opens the door there.
static struct {
    unsigned long long retrace;
    float x, z;
} g_face[8];
static unsigned g_face_count;

static bool guest_pointer(u32 address) { return address >= 0x80000000u && address < 0x81800000u; }

static float read_f32(CPUState* cpu, u32 address) {
    const u32 bits = mem_read32(cpu, address);
    float value;
    memcpy(&value, &bits, sizeof value);
    return value;
}

static void write_f32(CPUState* cpu, u32 address, float value) {
    u32 bits;
    memcpy(&bits, &value, sizeof bits);
    mem_write32(cpu, address, bits);
}

static void read_string(CPUState* cpu, u32 address, char* out, unsigned size) {
    unsigned i = 0;
    for (; i + 1u < size; ++i) {
        const char c = (char)mem_read8(cpu, address + i);
        if (c == '\0')
            break;
        out[i] = c;
    }
    out[i] = '\0';
}

// The running event's name, or "-".
static void event_name(CPUState* cpu, char* out, unsigned size) {
    snprintf(out, size, "-");
    const s16 id = (s16)mem_read16(cpu, kEventId);
    const u32 events = mem_read32(cpu, kEventList + 4u);
    if (mem_read8(cpu, kEventMode) != 0u && id >= 0 && guest_pointer(events))
        read_string(cpu, events + (u32)id * 0xB0u, out, size);
}

void bluewake_quick_doors_reload(void) {
    const char* on = getenv("BLUEWAKE_QUICK_DOORS");
    g_enabled = on == NULL || on[0] != '0';
}

void bluewake_quick_doors_attach(CPUState* cpu) {
    g_cpu = cpu;
    bluewake_quick_doors_reload();
    const char* trace = getenv("BLUEWAKE_DOOR_TRACE");
    g_trace = trace != NULL ? atoi(trace) : 0;
    const char* face = getenv("BLUEWAKE_DOOR_TEST_FACE");
    for (const char* p = face; p != NULL && *p != '\0' && g_face_count < 8u;) {
        unsigned long long retrace;
        float x, z;
        int used = 0;
        if (sscanf(p, "%llu:%f:%f%n", &retrace, &x, &z, &used) != 3)
            break;
        g_face[g_face_count].retrace = retrace;
        g_face[g_face_count].x = x;
        g_face[g_face_count].z = z;
        ++g_face_count;
        p += used;
        if (*p == ',')
            ++p;
    }
    if (g_face_count > 0u && g_trace == 0)
        g_trace = 1;
    if (g_enabled)
        fprintf(stderr, "[quick-door] doors with a knob: no walk under the fade, no door closing behind Link\n");
}

// The new stage's player, made from a knob door's start point: made as from an
// ordinary one where the door's arrival would have left him.
void bluewake_quick_doors_enter(CPUState* cpu) {
    if (cpu == NULL || cpu->lr < kPlayerInit || cpu->lr >= kPlayerInitEnd || !g_enabled)
        return;
    const u32 append = cpu->gpr[4];
    if (!guest_pointer(append) || !guest_pointer(cpu->gpr[3]))
        return;
    char name[9];
    read_string(cpu, cpu->gpr[3], name, sizeof name);
    const u32 params = mem_read32(cpu, append + kAppendParams);
    const u32 mode = (params >> 12) & 0xFu;
    // A start point of the stage (not a restart: those points are negative).
    if (strcmp(name, "Link") != 0 || (s16)mem_read16(cpu, kStartStage + 8u) < 0 || (mode != 0xAu && mode != 0xBu))
        return;
    const s16 angle = (s16)mem_read16(cpu, append + kAppendAngleY);
    const float on = mode == 0xAu ? kArriveFront : kArriveBack;
    const float radians = (float)angle * (3.14159265f / 32768.0f);
    const float x = read_f32(cpu, append + kAppendPos), z = read_f32(cpu, append + kAppendPos + 8u);
    write_f32(cpu, append + kAppendPos, x + on * sinf(radians));
    write_f32(cpu, append + kAppendPos + 8u, z + on * cosf(radians));
    // Start mode 0 and no event of the point's own: DEFAULT_START.
    mem_write32(cpu, append + kAppendParams, (params & 0x00FF0FFFu) | 0xFF000000u);
    g_arrived = true;
    if (g_trace > 0) {
        char stage[9];
        read_string(cpu, kStartStage, stage, sizeof stage);
        fprintf(stderr,
                "[quick-door] retrace=%llu arrive %s point %d: start mode 0x%X at %.1f,%.1f becomes an ordinary "
                "start %.0f units on (angle 0x%04X), params 0x%08X -> 0x%08X\n",
                g_retrace, stage, (s16)mem_read16(cpu, kStartStage + 8u), mode, x, z, on, (u16)angle, params,
                mem_read32(cpu, append + kAppendParams));
    }
}

// The door's own fade covers the whole screen (and stays so until the new
// scene is made: dScnPly_Create's offFade).
static bool door_covered(CPUState* cpu) {
    return mem_read8(cpu, kGphFade) != 0u && read_f32(cpu, kGphFadeSpeed) > 0.0f &&
           read_f32(cpu, kGphFadeRate) >= 1.0f && mem_read8(cpu, kGphFadeColor + 3u) == 0xFFu;
}

// A knob door's own event (every stage has the same two).
static bool door_event(CPUState* cpu) {
    char name[33];
    event_name(cpu, name, sizeof name);
    return strcmp(name, "DEFAULT_KNOB_DOOR_F_OPEN") == 0 || strcmp(name, "DEFAULT_KNOB_DOOR_B_OPEN") == 0;
}

// Once the door's fade covers the screen: the rest of Link's walk to the exit
// polygon, in bigger steps.
static void speed_walk(CPUState* cpu, u32 player) {
    const float rate = read_f32(cpu, player + kUnderRate);
    if (g_walk_sped || mem_read32(cpu, player + kCurProc) != kProcDoorOpen || rate <= 0.5f || rate >= kWalkRate ||
        !door_event(cpu))
        return;
    write_f32(cpu, player + kUnderRate, rate * kWalkRate);
    const float upper = read_f32(cpu, player + kUpperRate);
    if (upper > 0.5f && upper < kWalkRate)
        write_f32(cpu, player + kUpperRate, upper * kWalkRate);
    g_walk_sped = true;
    g_exiting = true;
    if (g_trace > 0)
        fprintf(stderr, "[quick-door] retrace=%llu covered: Link's walk to the exit at %.0fx from frame %.1f\n",
                g_retrace, kWalkRate, read_f32(cpu, player + kUnderFrame));
}

// The scene change the door leads to is asked for: OVERLAP8's count of the
// frames the door's fade takes ends at once (the fade already covers).
static void cut_wait(CPUState* cpu) {
    const u32 overlap = mem_read32(cpu, kOverlap);
    const u32 task = guest_pointer(overlap) ? mem_read32(cpu, overlap + kRequestTask) : 0u;
    const u32 fader = mem_read32(cpu, kFader);
    if (g_wait_cut || !guest_pointer(task) || !guest_pointer(fader) ||
        mem_read16(cpu, task + kProcName) > kLastFadeOverlap ||
        mem_read32(cpu, fader + kFaderStatus) != (u32)kFaderWaitIn || !door_event(cpu))
        return;
    const s32 left = (s32)mem_read32(cpu, task + kFadeInTime);
    if (left <= 1)
        return;
    mem_write32(cpu, task + kFadeInTime, 1u);
    g_wait_cut = true;
    if (g_trace > 0)
        fprintf(stderr, "[quick-door] retrace=%llu scene change: the overlap's %d frames of waiting cut\n", g_retrace,
                left);
}

// After a start point made ordinary, the picture starts coming back
// kArriveHold frames later: the fader stays covering (its own delayed status
// change, JUTFader::control's mDelayTimer) and then fades in as it would have.
static void hold_picture(CPUState* cpu) {
    const u32 fader = mem_read32(cpu, kFader);
    if (!guest_pointer(fader) || mem_read32(cpu, fader + kFaderStatus) != (u32)kFaderFadeIn)
        return;
    g_arrived = false;
    const u16 timer = mem_read16(cpu, fader + kFaderTimer);
    if (timer > 1u)
        return; // too late to hold it
    mem_write32(cpu, fader + kFaderStatus, (u32)kFaderWaitOut);
    mem_write16(cpu, fader + kFaderTimer, 0u);
    mem_write32(cpu, fader + kFaderDelayStatus, (u32)kFaderFadeIn);
    mem_write32(cpu, fader + kFaderDelayTimer, (u32)kArriveHold);
    if (g_trace > 0)
        fprintf(stderr, "[quick-door] retrace=%llu the picture comes back %d frames later\n", g_retrace, kArriveHold);
}

static void face(CPUState* cpu, float x, float z) {
    const u32 player = mem_read32(cpu, kPlayerPointer);
    if (!guest_pointer(player))
        return;
    const float dx = x - read_f32(cpu, player + kPos), dz = z - read_f32(cpu, player + kPos + 8u);
    const s16 angle = (s16)(atan2f(dx, dz) * (32768.0f / 3.14159265f));
    // As daPy_lk_c::setPlayerPosAndAngle does: execute starts from the kept copies.
    mem_write16(cpu, player + kAngleY, (u16)angle);
    mem_write16(cpu, player + kShapeAngleY, (u16)angle);
    mem_write16(cpu, player + kTargetAngle, (u16)angle);
    mem_write16(cpu, kKeptAngleY, (u16)angle);
    mem_write16(cpu, kKeptShapeY, (u16)angle);
    fprintf(stderr, "[quick-door] test face retrace=%llu angle=0x%04X\n", g_retrace, (u16)angle);
}

static void trace_state(CPUState* cpu) {
    const u32 player = mem_read32(cpu, kPlayerPointer);
    char name[33], stage[9];
    event_name(cpu, name, sizeof name);
    read_string(cpu, kStartStage, stage, sizeof stage);
    const u32 fader = mem_read32(cpu, kFader);
    const int fstatus = guest_pointer(fader) ? (int)mem_read32(cpu, fader + kFaderStatus) : -1;
    const int falpha = guest_pointer(fader) ? mem_read8(cpu, fader + 0x0Fu) : -1;
    const u32 overlap = mem_read32(cpu, kOverlap);
    const u32 task = guest_pointer(overlap) ? mem_read32(cpu, overlap + kRequestTask) : 0u;
    const int ovname = guest_pointer(task) ? (int)(s16)mem_read16(cpu, task + kProcName) : -1;
    const s32 ovin = guest_pointer(task) ? (s32)mem_read32(cpu, task + kFadeInTime) : -1;
    const s32 ovout = guest_pointer(task) ? (s32)mem_read32(cpu, task + 0xCCu) : -1;
    float x = 0, y = 0, z = 0, rate = 0, frame = 0;
    u32 proc = 0, demo = 0;
    u16 angle = 0;
    float cam[6] = {0};
    const u32 camera = mem_read32(cpu, kCamera);
    for (u32 i = 0; guest_pointer(camera) && i < 6u; ++i)
        cam[i] = read_f32(cpu, camera + 0xD8u + i * 4u);
    if (guest_pointer(player)) {
        x = read_f32(cpu, player + kPos);
        y = read_f32(cpu, player + kPos + 4u);
        z = read_f32(cpu, player + kPos + 8u);
        proc = mem_read32(cpu, player + kCurProc);
        demo = mem_read32(cpu, player + kDemoMode);
        rate = read_f32(cpu, player + kUnderRate);
        frame = read_f32(cpu, player + kUnderFrame);
        angle = mem_read16(cpu, player + kShapeAngleY);
    }
    fprintf(stderr,
            "[door-trace] retrace=%llu stage=%s ev=%u/%d %s exc=%d/%d next=%u/%u proc=%u demo=%u anim=%.1f@%.2f "
            "pos=%.1f,%.1f,%.1f ang=0x%04X gph=%u/%.2f/%.3f a=%u fader=%d/%d ovl=%d in=%d out=%d "
            "eye=%.0f,%.0f,%.0f center=%.0f,%.0f,%.0f\n",
            g_retrace, stage, mem_read8(cpu, kEventMode), (s16)mem_read16(cpu, kEventId), name,
            (int)mem_read32(cpu, kExceptionIdx), (int)mem_read32(cpu, kExceptionState),
            mem_read8(cpu, kNextStage + 0xCu), mem_read8(cpu, kNextStage + 0xDu), proc, demo, frame, rate, x, y, z,
            angle, mem_read8(cpu, kGphFade), read_f32(cpu, kGphFadeRate), read_f32(cpu, kGphFadeSpeed),
            mem_read8(cpu, kGphFadeColor + 3u), fstatus, falpha, ovname, ovin, ovout, cam[0], cam[1], cam[2],
            cam[3], cam[4], cam[5]);
}

// BLUEWAKE_DOOR_TRACE's timing: from the door's event starting to Link
// standing, walking or running under the player's control in the next scene.
// Retraces are game time; the black between (fast_load.c's fast-forward) runs
// as fast as the host can, so its retraces are counted apart.
static void time_door(CPUState* cpu, u32 player) {
    char name[33];
    event_name(cpu, name, sizeof name);
    if (strncmp(name, "DEFAULT_KNOB_DOOR_", 18) == 0 && g_door_start == 0u) {
        g_door_start = g_retrace;
        g_door_swapped = false;
        g_door_ff = 0u;
        fprintf(stderr, "[quick-door] retrace=%llu door: %s\n", g_retrace, name);
    }
    if (g_door_start == 0u)
        return;
    if (bluewake_fast_load_fast_forward())
        ++g_door_ff;
    if (g_door_covered == 0u && door_covered(cpu))
        g_door_covered = g_retrace;
    if (!guest_pointer(player)) {
        g_door_swapped = true;
        return;
    }
    const u32 fader = mem_read32(cpu, kFader);
    if (!g_door_swapped || mem_read8(cpu, kEventMode) != 0u || bluewake_fast_load_fast_forward() ||
        !guest_pointer(fader) || mem_read32(cpu, fader + kFaderStatus) == 0u)
        return;
    const u32 proc = mem_read32(cpu, player + kCurProc);
    if (proc != kProcWait && proc != kProcFreeWait && proc != kProcMove)
        return;
    char stage[9];
    read_string(cpu, kStartStage, stage, sizeof stage);
    const unsigned long long total = g_retrace - g_door_start;
    const u32 camera = mem_read32(cpu, kCamera);
    fprintf(stderr,
            "[quick-door] retrace=%llu control in %s at %.1f,%.1f,%.1f angle 0x%04X proc %u: %llu retraces from "
            "the door's event (%llu of them fast-forwarded black, so %.2f s at 60 Hz for the rest), %llu from the "
            "screen covered; restart room %d at %.1f,%.1f,%.1f angle 0x%04X; camera eye %.0f,%.0f,%.0f\n",
            g_retrace, stage, read_f32(cpu, player + kPos), read_f32(cpu, player + kPos + 4u),
            read_f32(cpu, player + kPos + 8u), mem_read16(cpu, player + kShapeAngleY), proc, total, g_door_ff,
            (double)(total - g_door_ff) / 60.0, g_door_covered != 0u ? g_retrace - g_door_covered : 0ull,
            (s8)mem_read8(cpu, kRestart), read_f32(cpu, kRestart + 0x18u), read_f32(cpu, kRestart + 0x1Cu),
            read_f32(cpu, kRestart + 0x20u), mem_read16(cpu, kRestart + 0x16u),
            guest_pointer(camera) ? read_f32(cpu, camera + 0xD8u) : 0.0f,
            guest_pointer(camera) ? read_f32(cpu, camera + 0xDCu) : 0.0f,
            guest_pointer(camera) ? read_f32(cpu, camera + 0xE0u) : 0.0f);
    g_door_start = g_door_covered = 0u;
}

void bluewake_quick_doors_retrace(void) {
    ++g_retrace;
    CPUState* cpu = g_cpu;
    if (cpu == NULL)
        return;
    for (unsigned i = 0; i < g_face_count; ++i)
        if (g_face[i].retrace == g_retrace)
            face(cpu, g_face[i].x, g_face[i].z);
    const u32 player = mem_read32(cpu, kPlayerPointer);
    const bool changing = guest_pointer(mem_read32(cpu, kOverlap));
    // The player of the next scene is made while its overlap runs.
    bluewake_quick_doors_armed = g_enabled && changing;
    if (g_enabled && door_covered(cpu)) {
        if (guest_pointer(player))
            speed_walk(cpu, player);
        if (g_exiting)
            cut_wait(cpu);
    } else {
        // The new scene takes the door's fade away (dScnPly_Create's offFade),
        // and from then the fader covers the screen until the picture comes back.
        g_walk_sped = g_wait_cut = g_exiting = false;
    }
    if (g_arrived)
        hold_picture(cpu);
    if (g_trace > 0)
        time_door(cpu, player);
    if (g_trace > 1)
        trace_state(cpu);
}
