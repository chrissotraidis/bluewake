// COS_SMOKE=sun-glare (issue #7): the sun's lens flare and glare, which the game shows only where
// GXPeekZ reads the sky's depth (the far plane, 0xFFFFFF) at the sun's five check points
// (d_kankyo_rain.cpp dKyr_sun_move peeks them through dComIfGd_peekZ at the end of each frame and
// reads them in the next). Run with the sailing preset at 17:30 (COS_BOOT_TIME=17.5) and
// native/check/input/sun-glare.txt, which turns the camera until the low sun is near the top
// centre of the view and lets it settle. At kCheckFrame the sun packet must have its centre point
// and at least four of the five on the sky, a visibility near 1 and the flare drawn over the scene
// (mbDrawLenzInSky false). With GXPeekZ reading 0 (the deko3d renderer before the fix) every point
// reads occluded and the flare and glare never draw. Exit 0 when all held, else 1. A shot of it.
//
// COS_SUN_GLARE_CHECK=sail (issue #7, second report: a brown/orange tint over the whole screen with
// the low sun "behind the sail"): with native/check/input/sun-glare-sail.txt at 17:00, the sail
// raised and the boat turning, the low sun crosses the view from left to right and the sail sweeps
// over it. The glare (dKy_set_actcol/bgcol_ratio darkening the scene, the flare packet) is the
// original game's whenever the check points read the sky, and the stronger the nearer the sun is to
// the screen's centre. The boat's sail (d_a_grid.cpp, daGrid_packet_c::draw; d_a_sail.cpp is the pirate
// ship's) is drawn see-through (blended with its texture's alpha) but writes depth over all its cloth: its
// l_matDL sets ZMode (on, LEQUAL, update on), Z before texturing and an alpha test that always passes. So
// in the original the check points read the sail, the sun counts as hidden and the glare fades, even
// though the sun shows faintly through the cloth.
// Checked: the sun on the sky with the glare before the sail reaches it (kSailBefore), then while the
// sail is over it (kSailFrom..kSailTo) at least three points reading the sail (nearer than the
// clouds, < 0xFF8000) in some frame and the visibility below 0.1 in some frame (the peeks arrive one to
// three frames late, more on a loaded machine: a window, not one frame). Fails if GXPeekZ reads through
// the sail.
//
// COS_SUN_GLARE_TRACE=N (any run, also the console's [dev] section; no check, no exit): a "[cos]
// sun-glare trace:" line every N frames while the sun is near the view: the five peeked values, the
// visibility, staringAtSunAmount (from mDistFalloff), the colour ratios it set and the sun on the
// 640x480 screen. With COS_SMOKE=sun-glare also COS_SUN_GLARE_SHOTS=f1,f2,... (shots of those
// frames) and COS_SUN_GLARE_END=f (exit 0 at that frame) instead of the check.
#include "pc_internal.h"

#include "d/d_com_inf_game.h"
#include "d/d_kankyo.h"
#include "d/d_kankyo_wether.h"
#include "m_Do/m_Do_lib.h"

#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <vector>

namespace pc {

namespace {

constexpr unsigned int kCheckFrame = 1100; // the script stops the camera at 983
constexpr unsigned int kSailBefore = 935;  // sun-glare-sail.txt: the sun near the centre, on the sky
constexpr unsigned int kSailFrom = 940;    // the sail over the sun
constexpr unsigned int kSailTo = 965;
bool sChecked = false;
bool sOn = false;
bool sSail = false;
unsigned int sTrace = 0;
unsigned int sEnd = 0;
std::vector<unsigned int> sShots;
bool sSailBeforeOk = false;
int sSailMaxOnSail = 0;
float sSailMinVis = 1.0f;

void shotSink(unsigned int frame, const std::vector<uint8_t>& rgb, uint32_t w, uint32_t h, void*) {
    saveFramePng(frame, rgb, w, h);
}

unsigned int envUint(const char* name) {
    const char* v = getenv(name);
    return v != nullptr ? (unsigned int)strtoul(v, nullptr, 10) : 0;
}

int onSky(const dKankyo_sun_Packet* sun) {
    int n = 0;
    for (int i = 0; i < 5; i++) {
        n += sun->mVizChkData[i] >= 0xFFFFFF;
    }
    return n;
}

// dKyr_sun_move: f = 1 - (the sun's distance from the screen centre) / 450, mDistFalloff = 1 - f^2 and
// staringAtSunAmount = f^4 = (1 - mDistFalloff)^2
float staring(const dKankyo_sunlenz_Packet* lenz) {
    const float s = 1.0f - lenz->mDistFalloff;
    return s * s;
}

void traceLine(unsigned int frames, bool always) {
    const dKankyo_sun_Packet* sun = dKy_getEnvlight().mpSunPacket;
    const dKankyo_sunlenz_Packet* lenz = dKy_getEnvlight().mpSunlenzPacket;
    if (sun == nullptr || lenz == nullptr) {
        return;
    }
    cXyz projected;
    mDoLib_project(const_cast<cXyz*>(&sun->mPos[0]), &projected);
    const bool near = projected.x > -100.0f && projected.x < 740.0f && projected.y > -100.0f && projected.y < 580.0f;
    if (!always && !near && sun->mVisibility <= 0.0f) {
        return;
    }
    const dScnKy_env_light_c& env = dKy_getEnvlight();
    writef(STDERR_FILENO,
           "[cos] sun-glare trace: frame %u time %.2f sun (%.0f, %.0f) peek %06x %06x %06x %06x %06x vis %.3f "
           "staring %.3f lenz %s act %.3f bg %.3f fog %.3f sky %.3f\n",
           frames, env.mCurTime, near ? projected.x : -1.0f, near ? projected.y : -1.0f,
           (unsigned)sun->mVizChkData[0], (unsigned)sun->mVizChkData[1], (unsigned)sun->mVizChkData[2],
           (unsigned)sun->mVizChkData[3], (unsigned)sun->mVizChkData[4], sun->mVisibility, staring(lenz),
           lenz->mbDrawLenzInSky ? "sky" : "over", env.mActColGatherRatio, env.mBgColGatherRatio,
           env.mFogColGatherRatio, env.mVrboxSoraColRatio);
}

void sailCheck(unsigned int frames, const dKankyo_sun_Packet* sun, const dKankyo_sunlenz_Packet* lenz) {
    if (frames == kSailBefore) {
        sSailBeforeOk = onSky(sun) >= 4 && sun->mVisibility > 0.9f && !lenz->mbDrawLenzInSky && staring(lenz) > 0.1f;
        traceLine(frames, true);
        captureFrame(frames, shotSink, nullptr);
    }
    if (frames >= kSailFrom && frames <= kSailTo) {
        int onSail = 0;
        for (int i = 0; i < 5; i++) {
            onSail += sun->mVizChkData[i] < 0xFF8000;
        }
        sSailMaxOnSail = onSail > sSailMaxOnSail ? onSail : sSailMaxOnSail;
        sSailMinVis = sun->mVisibility < sSailMinVis ? sun->mVisibility : sSailMinVis;
        traceLine(frames, true);
    }
    if (frames == kSailTo) {
        captureFrame(frames, shotSink, nullptr);
        const bool ok = sSailBeforeOk && sSailMaxOnSail >= 3 && sSailMinVis < 0.1f;
        writef(STDERR_FILENO,
               "[cos] sun-glare: sail: before it the sun %s; over the sun at most %d of 5 points on the sail; "
               "lowest visibility %.3f (frames %u-%u)\n[cos] sun-glare: %s\n",
               sSailBeforeOk ? "on the sky with the glare" : "NOT on the sky with the glare", sSailMaxOnSail,
               sSailMinVis, kSailFrom, kSailTo, ok ? "PASS" : "FAIL");
        pc_exit(ok ? PC_EXIT_REACHED : PC_EXIT_CHECK_FAILED);
    }
}

} // namespace

void sunGlareFrame(unsigned int frames) {
    if (!sChecked) {
        sChecked = true;
        sOn = gConfig.smoke != nullptr && strcmp(gConfig.smoke, "sun-glare") == 0;
        const char* check = getenv("COS_SUN_GLARE_CHECK");
        sSail = sOn && check != nullptr && strcmp(check, "sail") == 0;
        sTrace = envUint("COS_SUN_GLARE_TRACE");
        if (sOn) {
            sEnd = envUint("COS_SUN_GLARE_END");
            const char* sh = getenv("COS_SUN_GLARE_SHOTS");
            while (sh != nullptr && *sh != '\0') {
                char* end = nullptr;
                const unsigned long f = strtoul(sh, &end, 10);
                if (end == sh) {
                    break;
                }
                sShots.push_back((unsigned int)f);
                sh = *end == ',' ? end + 1 : end;
            }
        }
    }
    if (sTrace != 0 && frames % sTrace == 0) {
        traceLine(frames, false);
    }
    if (!sOn) {
        return;
    }
    if (sEnd != 0 || !sShots.empty()) { // the investigation mode: shots and an end, no check
        for (unsigned int f : sShots) {
            if (f == frames) {
                traceLine(frames, true);
                captureFrame(frames, shotSink, nullptr);
            }
        }
        if (sEnd != 0 && frames >= sEnd) {
            writef(STDERR_FILENO, "[cos] sun-glare: trace run ended at frame %u\n", frames);
            pc_exit(PC_EXIT_REACHED);
        }
        return;
    }
    const unsigned int first = sSail ? kSailBefore : kCheckFrame;
    if (frames < first) {
        return;
    }
    const dKankyo_sun_Packet* sun = dKy_getEnvlight().mpSunPacket;
    const dKankyo_sunlenz_Packet* lenz = dKy_getEnvlight().mpSunlenzPacket;
    if (sun == nullptr || lenz == nullptr) {
        writef(STDERR_FILENO, "[cos] sun-glare: frame %u: no sun packet (not in play at sea in daytime?)\nFAIL\n",
               frames);
        pc_exit(PC_EXIT_CHECK_FAILED);
    }
    if (sSail) {
        sailCheck(frames, sun, lenz);
        return;
    }
    const int sky = onSky(sun);
    const bool centre = sun->mVizChkData[0] >= 0xFFFFFF;
    writef(STDERR_FILENO,
           "[cos] sun-glare: frame %u: time %.1f, peeked depth %06x %06x %06x %06x %06x (%d of 5 on the sky), "
           "visibility %.2f, flare %s\n",
           frames, dKy_getEnvlight().mCurTime, (unsigned)sun->mVizChkData[0], (unsigned)sun->mVizChkData[1],
           (unsigned)sun->mVizChkData[2], (unsigned)sun->mVizChkData[3], (unsigned)sun->mVizChkData[4], sky,
           sun->mVisibility, lenz->mbDrawLenzInSky ? "behind the scene (sun hidden)" : "over the scene");
    captureFrame(frames, shotSink, nullptr);
    const bool ok = centre && sky >= 4 && sun->mVisibility > 0.9f && !lenz->mbDrawLenzInSky;
    writef(STDERR_FILENO, "[cos] sun-glare: %s\n", ok ? "PASS" : "FAIL");
    pc_exit(ok ? PC_EXIT_REACHED : PC_EXIT_CHECK_FAILED);
}

} // namespace pc
