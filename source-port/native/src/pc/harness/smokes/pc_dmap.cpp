// COS_SMOKE=dmap (issue #6): the dungeon map screen (Up in a dungeon, dMenu_Dmap_c / dMap_Dmap_c) draws
// its rooms. Boot in a dungeon, e.g. COS_BOOT_STAGE=M_NewD2:0:0 (Dragon Roost Cavern's entrance):
//   1. A through any event until none runs, then kSettleFrames;
//   2. the dungeon map and the compass given to the stage (dComIfGs_onDungeonItemMap/Compass, as their
//      chests do), Up pressed: the dungeon map screen must open (MENU_STATUS_MAP) within kOpenFrames;
//   3. kDrawFrames later, a shot, and in the screen's map panel (kPanel*, fractions of the picture)
//      the rooms counted: the visited room the player stands in (color_on, green, flashing towards
//      0, 255, 180) and, with the map, the rooms not visited yet (color_off_map_possession, dark grey).
// The shot compiles each pipeline before its first draw (COS_SYNC_PIPELINES on for this smoke,
// pc_harness.cpp): with Aurora's default the panel's draws were skipped while their pipelines compiled,
// and the first uncapped run of a build directory (cold pipeline cache) read an empty panel.
// Exit 0 when the screen opened and both counts reach kMinPixels (per mille of the panel), else 1.
// Before the fix the floor TLUT stayed all zero (alpha 0: the palette read in host order matched no
// room colour) and the panel showed the grid alone.
#include "pc_internal.h"

#include "d/d_com_inf_game.h"
#include "d/d_meter.h"
#include "f_pc/f_pc_manager.h"
#include "f_pc/f_pc_name.h"

#include <dolphin/pad.h>

#include <cstring>
#include <unistd.h>

namespace pc {

namespace {

constexpr unsigned int kSettleFrames = 60;
constexpr unsigned int kOpenFrames = 60;
constexpr unsigned int kDrawFrames = 60;
// The map panel of the screen at 4:3 (the right page's grid, without its frame).
constexpr float kPanelX0 = 0.49f, kPanelX1 = 0.92f, kPanelY0 = 0.16f, kPanelY1 = 0.70f;
constexpr unsigned int kMinPixels = 2; // per mille of the panel, for each kind of room (0 before the fix;
                                       // Dragon Roost Cavern 1F: 23 and 169, Forbidden Woods 1F: 3 and 37)

enum State { kOff, kWait, kSettle, kOpen, kDraw, kDone };
State sState = kOff;
unsigned int sSince = 0;
unsigned int sEventFrames = 0;

struct Count {
    unsigned int panel = 0;
    unsigned int visited = 0;
    unsigned int unvisited = 0;
};

void* isPlayScene(void* proc, void*) {
    return fpcM_GetName(proc) == fpcNm_PLAY_SCENE_e ? proc : nullptr;
}

void countSink(unsigned int frame, const std::vector<uint8_t>& rgb, uint32_t w, uint32_t h, void* user) {
    saveFramePng(frame, rgb, w, h);
    Count* c = static_cast<Count*>(user);
    for (uint32_t y = (uint32_t)(kPanelY0 * h); y < (uint32_t)(kPanelY1 * h); y++) {
        for (uint32_t x = (uint32_t)(kPanelX0 * w); x < (uint32_t)(kPanelX1 * w); x++) {
            const uint8_t* p = &rgb[(y * w + x) * 3];
            const int r = p[0], g = p[1], b = p[2];
            c->panel++;
            if (g > r + 60 && g > b + 40) {
                c->visited++; // green: 0, 128, 0 .. 0, 255, 180
            } else if (r < 90 && g < 90 && b < 90 && r - b < 30 && g - b < 30) {
                c->unvisited++; // grey 30, 30, 30 at alpha 1 over the brown page (r > g > b, b = 0)
            }
        }
    }
}

void finish(unsigned int frames, bool ok) {
    setDrivenPad(true, 0, 0, 0);
    sState = kDone;
    writef(STDERR_FILENO, "[cos] dmap: frame %u: %s\n", frames, ok ? "PASS" : "FAIL");
    pc_exit(ok ? PC_EXIT_REACHED : PC_EXIT_CHECK_FAILED);
}

} // namespace

void dmapFrame(unsigned int frames) {
    if (sState == kOff) {
        if (gConfig.smoke == nullptr || strcmp(gConfig.smoke, "dmap") != 0) {
            sState = kDone;
            return;
        }
        setDrivenPad(true, 0, 0, 0);
        sState = kWait;
    }
    if (sState == kDone || dComIfGp_getPlayer(0) == nullptr || fpcM_Search(isPlayScene, nullptr) == nullptr) {
        return;
    }
    ++sSince;
    switch (sState) {
    case kOff:
    case kDone:
        return;
    case kWait:
        if (dComIfGp_event_runCheck()) {
            sEventFrames++;
            setDrivenPad(true, sEventFrames % 20 < 2 ? PAD_BUTTON_A : 0, 0, 0);
            sSince = 0;
            return;
        }
        setDrivenPad(true, 0, 0, 0);
        if (sSince >= kSettleFrames) {
            dComIfGs_onDungeonItemMap();
            dComIfGs_onDungeonItemCompass();
            writef(STDERR_FILENO, "[cos] dmap: frame %u: map %d, compass on; Up\n", frames,
                   (int)dComIfGs_isDungeonItemMap());
            sState = kOpen;
            sSince = 0;
        }
        return;
    case kOpen:
        setDrivenPad(true, sSince % 20 < 2 ? PAD_BUTTON_UP : 0, 0, 0);
        if (dMenu_flag() != 0 && dMenu_getMenuStatus() == MENU_STATUS_MAP) {
            setDrivenPad(true, 0, 0, 0);
            writef(STDERR_FILENO, "[cos] dmap: frame %u: the dungeon map screen opened\n", frames);
            sState = kDraw;
            sSince = 0;
        } else if (sSince >= kOpenFrames) {
            writef(STDERR_FILENO, "[cos] dmap: Up did not open the dungeon map screen (menu flag %u, status %u)\n",
                   (unsigned)dMenu_flag(), (unsigned)dMenu_getMenuStatus());
            finish(frames, false);
        }
        return;
    case kDraw: {
        if (sSince < kDrawFrames) {
            return;
        }
        Count c;
        if (!captureFrame(frames, countSink, &c) || c.panel == 0) {
            writef(STDERR_FILENO, "[cos] dmap: the frame could not be read back\n");
            finish(frames, false);
            return;
        }
        const unsigned int visited = c.visited * 1000 / c.panel;
        const unsigned int unvisited = c.unvisited * 1000 / c.panel;
        writef(STDERR_FILENO,
               "[cos] dmap: frame %u: map panel %u pixels: visited room %u (%u per mille), rooms not "
               "visited %u (%u per mille), at least %u per mille each\n",
               frames, c.panel, c.visited, visited, c.unvisited, unvisited, kMinPixels);
        finish(frames, visited >= kMinPixels && unvisited >= kMinPixels);
        return;
    }
    }
}

} // namespace pc
