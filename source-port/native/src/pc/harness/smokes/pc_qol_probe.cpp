// COS_TRACE=qol: one line per game frame with what the quality-of-life options (pc_qol.h,
// docs/QOL_PLAN.md) change, for their Mac checks: run the same input with an option off and on and
// compare. Not a test by itself (no exit code); any boot or input script.
//   [cos] qol-probe f=<frame> <stage>:<room> pos=(x,y,z) spd=<speedF> ang=<shape_angle.y>
//     roll=<checkFrontRoll> ev=<event id> "<event name>" msg=<message number>:<status> eye=(x,y,z)
//     ship=<boat speedF, -1 without boat>@(<boat position>) do=<the A button's action, dActStts_*> sw=<switch, -1>
//     xyz=<the items on X, Y and Z (dComIfGp_getSelectItem)>
//     [act=<name>:(x,y,z)]
// Variables:
//   COS_QOL_PROBE_FROM=<frame>, COS_QOL_PROBE_TO=<frame>   only the lines of those frames;
//   COS_QOL_PROBE_ACTOR=<process name>   the first actor of that name (a block, the grappling hook);
//   COS_QOL_PROBE_SWITCH=<n>             the player's room's switch n;
//   COS_QOL_PROBE_SPAWN=<frame>:<process name>:<parameters>[:<distance>[:<angle z>]][;...] (up to 4)
//       creates that actor <distance> (150) in front of the player (angle z default 0xFFFF) at that
//       frame, from the player's layer; the first is the probed actor unless COS_QOL_PROBE_ACTOR says
//       otherwise;
//   COS_QOL_PROBE_MSG=<frame>:<message number>   shows that message (fopMsgM_messageSet) at that frame;
//   COS_QOL_PROBE_ROPE=<frame>   the player grabs the nearest hanging rope (d_a_himo3) as the rope smoke
//       does (pc_rope.cpp);
//   COS_QOL_PROBE_BALLAD=<frame>:<destination>   starts the boat's Ballad of Gales warp (a cyclone and
//       daShip_c::setTactWarp, as the song does) at that frame; the player must be on the boat;
//   COS_QOL_PROBE_WARP=<frame>:<stage>:<room>:<point>[;...] (up to 4)   the options menu's travel at that frame
//       (pc_menu.cpp doWarp: dComIfGp_setNextStage with the game's own layer);
//   COS_QOL_PROBE_GIVE=<frame>:<name>[,<name>...]   the debug server's "give" at that frame (pc_debug_give,
//       pc_menu.cpp: the same queue, run by the game thread at the frame's end);
//   COS_QOL_PROBE_SE=<n>   every n frames (also outside the PLAY scene) one line with what JAudio plays:
//       "[cos] qol-probe se f=<frame> <pc_audio_state.cpp's line>" (the registered sound effects, the
//       SE tracks still sounding, the BGM, the audio thread's counters).
//   COS_QOL_PROBE_SAVE=<n>   n frames after the player first exists, the save data in memory
//       (dSv_save_c: items, event bits, stage memories) as "[cos] qol-probe save <offset> <hex>" lines
//       of 64 bytes (all-zero lines left out), to compare a run with an option off and on;
//   COS_QOL_PROBE_ITEM=<frame>:<item>   gives that item (execItemGet, as a chest does) at that frame;
//   COS_QOL_PROBE_LIGHT=<frame>   a Light Arrow's hit on the probed actor if it is a Miniblin (d_a_pt:
//       its enemy_ice light timer started, as its damage check does), for the bug fixes' check;
//   COS_QOL_PROBE_PATHS=1   once the boat exists, the paths daShip_c::checkOutRange keeps the boat in
//       (its room's paths: index, points, XZ bounds), and daShip_c::checkOutRange at a few points
//       of the sea (the boat put there for the call, then back): 1 = the boat would be turned back,
//       for the unrestricted boat's check.
#include "pc_internal.h"

#include "d/actor/d_a_player.h"
#include "d/actor/d_a_ship.h"
#include "d/d_com_inf_game.h"
#include "d/d_event_data.h"
#include "d/actor/d_a_pt.h"
#include "d/d_event_manager.h"
#include "d/d_item.h"
#include "d/d_path.h"
#include "f_op/f_op_actor_mng.h"
#include "f_op/f_op_camera.h"
#include "f_op/f_op_msg.h"
#include "f_op/f_op_msg_mng.h"
#include "f_pc/f_pc_layer.h"
#include "f_pc/f_pc_manager.h"
#include "f_pc/f_pc_name.h"

#include <cstdlib>
#include <cstring>
#include <unistd.h>

namespace pc {

namespace {

int sOn = -1;
int sActorName = -1;
unsigned int sFrom = 0;
unsigned int sTo = ~0u;
unsigned int sMsgFrame = 0;
unsigned int sMsgNo = 0;
fpc_ProcID sMsgId = fpcM_ERROR_PROCESS_ID_e;
unsigned int sRopeFrame = 0;
unsigned int sBalladFrame = 0;
int sBalladTo = 0;
struct Spawn {
    unsigned int frame;
    int name;
    u32 param;
    float distance;
    s16 angleZ;
};
Spawn sSpawns[4];
int sSpawnCount = 0;
int sSwitch = -1;
struct ProbeWarp {
    unsigned int frame;
    char stage[8];
    int room;
    int point;
};
ProbeWarp sWarps[4];
int sWarpCount = 0;
unsigned int sSeEvery = 0;
unsigned int sGiveFrame = 0;
char sGive[256] = {};

// What JAudio plays (pc_audio_state.cpp).
void seLine(unsigned int frames) {
    char line[1536];
    audioStateLine(line, sizeof(line));
    writef(STDERR_FILENO, "[cos] qol-probe se f=%u %s\n", frames, line);
}
int sSaveAfter = -1;
unsigned int sItemFrame = 0;
int sItem = -1;
unsigned int sLightFrame = 0;
bool sPaths = false;
bool sPathsDone = false;
unsigned int sFirstPlayerFrame = 0;

void* isPlayScene(void* proc, void*) {
    return fpcM_GetName(proc) == fpcNm_PLAY_SCENE_e ? proc : nullptr;
}

void* isActorName(void* proc, void*) {
    return fpcM_IsCreating(fpcM_GetID(proc)) == FALSE && fpcM_GetName(proc) == sActorName ? proc : nullptr;
}

void* nearestRope(void* proc, void* data) {
    fopAc_ac_c** best = (fopAc_ac_c**)data;
    fopAc_ac_c* player = dComIfGp_getPlayer(0);
    if (fopAcM_GetName((fopAc_ac_c*)proc) == fpcNm_HIMO3_e) {
        fopAc_ac_c* rope = (fopAc_ac_c*)proc;
        if (*best == nullptr ||
            (rope->current.pos - player->current.pos).absXZ() < ((*best)->current.pos - player->current.pos).absXZ()) {
            *best = rope;
        }
    }
    return nullptr;
}

void* isMessage(void* proc, void*) {
    const s16 name = fpcM_GetName(proc);
    return name == fpcNm_MSG_e || name == fpcNm_MSG2_e || name == fpcNm_MSG3_e ? proc : nullptr;
}

} // namespace

} // namespace pc

extern "C" int pc_debug_grab_rope(fopAc_ac_c* rope, const cXyz* at); // pc_rope.cpp
extern "C" int pc_debug_give(const char* const* names, int count, char* done, size_t size); // pc_menu.cpp

namespace pc {

void qolProbeFrame(unsigned int frames) {
    if (sOn < 0) {
        sOn = pc_trace_enabled("qol");
        if (const char* v = getenv("COS_QOL_PROBE_ACTOR"); v != nullptr) {
            sActorName = (int)strtol(v, nullptr, 0);
        }
        if (const char* v = getenv("COS_QOL_PROBE_FROM"); v != nullptr) {
            sFrom = (unsigned int)strtoul(v, nullptr, 0);
        }
        if (const char* v = getenv("COS_QOL_PROBE_TO"); v != nullptr) {
            sTo = (unsigned int)strtoul(v, nullptr, 0);
        }
        for (const char* v = getenv("COS_QOL_PROBE_SPAWN"); v != nullptr && *v != '\0' && sSpawnCount < 4;) {
            Spawn s = {0, -1, 0, 150.0f, (s16)0xFFFF};
            char* end = nullptr;
            s.frame = (unsigned int)strtoul(v, &end, 0);
            if (*end == ':') {
                s.name = (int)strtol(end + 1, &end, 0);
            }
            if (*end == ':') {
                s.param = (u32)strtoul(end + 1, &end, 0);
            }
            if (*end == ':') {
                s.distance = strtof(end + 1, &end);
            }
            if (*end == ':') {
                s.angleZ = (s16)strtol(end + 1, &end, 0);
            }
            sSpawns[sSpawnCount++] = s;
            if (sActorName < 0) {
                sActorName = s.name;
            }
            v = *end == ';' ? end + 1 : nullptr;
        }
        if (const char* v = getenv("COS_QOL_PROBE_SWITCH"); v != nullptr) {
            sSwitch = (int)strtol(v, nullptr, 0);
        }
        if (const char* v = getenv("COS_QOL_PROBE_ROPE"); v != nullptr) {
            sRopeFrame = (unsigned int)strtoul(v, nullptr, 0);
        }
        if (const char* v = getenv("COS_QOL_PROBE_BALLAD"); v != nullptr) {
            char* end = nullptr;
            sBalladFrame = (unsigned int)strtoul(v, &end, 0);
            sBalladTo = end != nullptr && *end == ':' ? (int)strtol(end + 1, nullptr, 0) : 0;
        }
        if (const char* v = getenv("COS_QOL_PROBE_WARP"); v != nullptr) {
            for (const char* p = v; p != nullptr && *p != '\0' && sWarpCount < 4;) {
                ProbeWarp& w = sWarps[sWarpCount];
                char stage[16] = {};
                if (sscanf(p, "%u:%7[^:]:%d:%d", &w.frame, stage, &w.room, &w.point) != 4) {
                    break;
                }
                snprintf(w.stage, sizeof(w.stage), "%s", stage);
                sWarpCount++;
                p = strchr(p, ';');
                p = p != nullptr ? p + 1 : nullptr;
            }
        }
        if (const char* v = getenv("COS_QOL_PROBE_GIVE"); v != nullptr) {
            char* end = nullptr;
            sGiveFrame = (unsigned int)strtoul(v, &end, 0);
            if (end != nullptr && *end == ':') {
                snprintf(sGive, sizeof(sGive), "%s", end + 1);
            }
        }
        if (const char* v = getenv("COS_QOL_PROBE_SE"); v != nullptr) {
            sSeEvery = (unsigned int)strtoul(v, nullptr, 0);
        }
        sPaths = getenv("COS_QOL_PROBE_PATHS") != nullptr;
        if (const char* v = getenv("COS_QOL_PROBE_ITEM"); v != nullptr) {
            char* end = nullptr;
            sItemFrame = (unsigned int)strtoul(v, &end, 0);
            sItem = end != nullptr && *end == ':' ? (int)strtol(end + 1, nullptr, 0) : -1;
        }
        if (const char* v = getenv("COS_QOL_PROBE_LIGHT"); v != nullptr) {
            sLightFrame = (unsigned int)strtoul(v, nullptr, 0);
        }
        if (const char* v = getenv("COS_QOL_PROBE_SAVE"); v != nullptr) {
            sSaveAfter = (int)strtol(v, nullptr, 0);
        }
        if (const char* v = getenv("COS_QOL_PROBE_MSG"); v != nullptr) {
            char* end = nullptr;
            sMsgFrame = (unsigned int)strtoul(v, &end, 0);
            sMsgNo = end != nullptr && *end == ':' ? (unsigned int)strtoul(end + 1, nullptr, 0) : 0;
        }
    }
    if (sOn == 0) {
        return;
    }
    if (sSeEvery != 0 && frames >= sFrom && frames <= sTo && frames % sSeEvery == 0) {
        seLine(frames);
    }
    if (fpcM_Search(isPlayScene, nullptr) == nullptr) {
        return;
    }
    daPy_py_c* player = (daPy_py_c*)dComIfGp_getPlayer(0);
    if (player == nullptr || fopAcM_GetName(player) != fpcNm_PLAYER_e || fpcM_IsCreating(fopAcM_GetID(player))) {
        return;
    }
    if (sFirstPlayerFrame == 0) {
        sFirstPlayerFrame = frames;
        writef(STDERR_FILENO, "[cos] qol-probe: the player first exists at frame %u (%s room %d)\n", frames,
               dComIfGp_getStartStageName(), (int)fopAcM_GetRoomNo(player));
    }
    if (sSaveAfter >= 0 && frames == sFirstPlayerFrame + (unsigned int)sSaveAfter) {
        const u8* save = (const u8*)&g_dComIfG_gameInfo.save.getSavedata();
        const size_t size = sizeof(dSv_save_c);
        writef(STDERR_FILENO, "[cos] qol-probe: save data at frame %u (%zu bytes)\n", frames, size);
        for (size_t at = 0; at < size; at += 64) {
            char hex[64 * 2 + 1];
            bool any = false;
            size_t n = size - at < 64 ? size - at : 64;
            for (size_t i = 0; i < n; i++) {
                snprintf(hex + i * 2, 3, "%02x", save[at + i]);
                any = any || save[at + i] != 0;
            }
            if (any) {
                writef(STDERR_FILENO, "[cos] qol-probe save %05zx %s\n", at, hex);
            }
        }
    }
    if (sPaths && !sPathsDone) {
        if (daShip_c* boat = dComIfGp_getShipActor(); boat != nullptr && boat->m034B != 0xFF) {
            sPathsDone = true;
            int index = 0;
            for (dPath* path = dPath_GetRoomPath(boat->m034B, -1); path != nullptr;
                 path = dPath_GetNextRoomPath(path, -1), index++) {
                float x0 = 1e9f, x1 = -1e9f, z0 = 1e9f, z1 = -1e9f;
                for (int i = 0; i < path->m_num; i++) {
                    const float x = path->m_points[i].m_position.x, z = path->m_points[i].m_position.z;
                    x0 = x < x0 ? x : x0;
                    x1 = x > x1 ? x : x1;
                    z0 = z < z0 ? z : z0;
                    z1 = z > z1 ? z : z1;
                }
                writef(STDERR_FILENO, "[cos] qol-probe: boat path %d (room %d): %d points, x %.0f..%.0f, z %.0f..%.0f\n",
                       index, (int)boat->m034B, (int)path->m_num, x0, x1, z0, z1);
            }
            struct Point {
                const char* name;
                float x, z;
            };
            const Point points[] = {
                {"Windfall's corner (the preset's start)", -22000.0f, -175000.0f},
                {"the middle of the sea (outside path 0)", 0.0f, 0.0f},
                {"the fortress corner (inside path 2)", -300000.0f, -300000.0f},
                {"past the north edge (outside path 3)", 0.0f, -355000.0f},
                {"past the west edge (outside path 3)", -355000.0f, 0.0f},
            };
            const cXyz saved = boat->current.pos;
            const s16 savedAngle = boat->m038C;
            for (const Point& p : points) {
                boat->current.pos.x = p.x;
                boat->current.pos.z = p.z;
                const BOOL out = boat->checkOutRange();
                writef(STDERR_FILENO, "[cos] qol-probe: checkOutRange at (%.0f, %.0f), %s: %d\n", p.x, p.z, p.name,
                       (int)out);
            }
            boat->current.pos = saved;
            boat->m038C = savedAngle;
        }
    }
    if (frames < sFrom || frames > sTo) {
        return;
    }

    // Made from the player's layer, as the player's own requests are (pc_actor_sweep.cpp).
    layer_class* savedLayer = fpcLy_CurrentLayer();
    fpcLy_SetCurrentLayer(static_cast<base_process_class*>(player)->mLyTg.mpLayer);
    for (int i = 0; i < sSpawnCount; i++) {
        const Spawn& s = sSpawns[i];
        if (s.name < 0 || frames != s.frame) {
            continue;
        }
        const s16 yaw = player->shape_angle.y;
        cXyz pos = player->current.pos;
        pos.x += s.distance * cM_ssin(yaw);
        pos.z += s.distance * cM_scos(yaw);
        csXyz angle(0, (s16)(yaw + 0x8000), s.angleZ);
        fpc_ProcID id = fopAcM_create((s16)s.name, s.param, &pos, fopAcM_GetRoomNo(player), &angle);
        writef(STDERR_FILENO, "[cos] qol-probe: actor %d (0x%08X) spawned at frame %u at (%.1f,%.1f,%.1f) (process %d)\n",
               s.name, s.param, frames, pos.x, pos.y, pos.z, (int)id);
    }
    if (sRopeFrame != 0 && frames == sRopeFrame) {
        // The rope smoke's grab (pc_rope.cpp): the nearest hanging rope, 200 below its top.
        fopAc_ac_c* rope = nullptr;
        fpcM_Search(nearestRope, &rope);
        if (rope != nullptr) {
            cXyz at(rope->current.pos.x + 20.0f, rope->current.pos.y - 200.0f, rope->current.pos.z);
            pc_debug_grab_rope(rope, &at);
            writef(STDERR_FILENO, "[cos] qol-probe: rope grabbed at frame %u\n", frames);
        }
    }
    if (sBalladFrame != 0 && frames == sBalladFrame) {
        // What the Ballad of Gales does on the boat once a destination is chosen
        // (daPy_lk_c::procTactPlay_init, procTactPlayEnd): the cyclone, then the boat's warp.
        if (daShip_c* boat = dComIfGp_getShipActor(); boat != nullptr) {
            fpc_ProcID warpID = fopAcM_create(fpcNm_TORNADO_e, 1, &player->current.pos, fopAcM_GetRoomNo(player));
            boat->setTactWarpID(warpID);
            boat->setTactWarpPosNum(sBalladTo);
            boat->setTactWarp();
            writef(STDERR_FILENO, "[cos] qol-probe: Ballad of Gales warp %d ordered at frame %u\n", sBalladTo, frames);
        }
    }
    if (sGiveFrame != 0 && frames == sGiveFrame && sGive[0] != '\0') {
        char buf[256];
        snprintf(buf, sizeof(buf), "%s", sGive);
        const char* names[16];
        int count = 0;
        for (char* t = strtok(buf, ","); t != nullptr && count < 16; t = strtok(nullptr, ",")) {
            names[count++] = t;
        }
        char done[512];
        const int ok = pc_debug_give(names, count, done, sizeof(done));
        writef(STDERR_FILENO, "[cos] qol-probe: give %s at frame %u: %s\n", sGive, frames, ok ? done : "unknown item");
    }
    for (int i = 0; i < sWarpCount; i++) {
        const ProbeWarp& w = sWarps[i];
        if (frames == w.frame) {
            writef(STDERR_FILENO, "[cos] qol-probe: warp to %s room %d point %d at frame %u\n", w.stage, w.room,
                   w.point, frames);
            dComIfGp_setNextStage(w.stage, (s16)w.point, (s8)w.room, -1);
        }
    }
    if (sItem >= 0 && frames == sItemFrame) {
        execItemGet((u8)sItem);
        writef(STDERR_FILENO, "[cos] qol-probe: item 0x%02X given at frame %u\n", sItem, frames);
    }
    if (sLightFrame != 0 && frames == sLightFrame && sActorName == fpcNm_PT_e) {
        if (pt_class* pt = (pt_class*)fpcM_Search(isActorName, nullptr); pt != nullptr) {
            // What d_a_pt's damage check does for a Light Arrow (AT_TYPE_LIGHT_ARROW).
            pt->mEnemyIce.mLightShrinkTimer = 1;
            writef(STDERR_FILENO, "[cos] qol-probe: Light Arrow hit on the Miniblin at frame %u\n", frames);
        }
    }
    if (sMsgNo != 0 && frames == sMsgFrame) {
        sMsgId = fopMsgM_messageSet(sMsgNo, &player->eyePos);
        writef(STDERR_FILENO, "[cos] qol-probe: message %u shown at frame %u (process %d)\n", sMsgNo, frames,
               (int)sMsgId);
    }
    fpcLy_SetCurrentLayer(savedLayer);

    dEvt_control_c* ev = g_dComIfG_gameInfo.play.getEvent();
    const char* evName = "";
    if (ev->mEventId != -1) {
        dEvDtEvent_c* data = dComIfGp_getPEvtManager()->getEventData(ev->mEventId);
        if (data != nullptr) {
            evName = data->getName();
        }
    }
    // The first message process (d_msg.cpp and the others): its message number and status (fopMsgStts_*), 0 without one.
    unsigned int msgNo = 0, msgStatus = 0;
    msg_class* msg = sMsgId != fpcM_ERROR_PROCESS_ID_e ? fopMsgM_SearchByID(sMsgId) : nullptr;
    if (msg == nullptr) {
        msg = (msg_class*)fpcM_Search(isMessage, nullptr);
    }
    if (msg != nullptr) {
        msgNo = msg->mMsgNo;
        msgStatus = msg->mStatus;
    }
    cXyz eye(0.0f, 0.0f, 0.0f);
    if (camera_process_class* cam = dComIfGp_getCamera(0); cam != nullptr) {
        eye = cam->view.mLookat.mEye;
    }
    daShip_c* ship = dComIfGp_getShipActor();
    char act[96] = "";
    if (sActorName >= 0) {
        if (fopAc_ac_c* a = (fopAc_ac_c*)fpcM_Search(isActorName, nullptr); a != nullptr) {
            snprintf(act, sizeof(act), " act=%d:(%.1f,%.1f,%.1f)", sActorName, a->current.pos.x, a->current.pos.y,
                     a->current.pos.z);
        } else {
            snprintf(act, sizeof(act), " act=%d:-", sActorName);
        }
    }
    writef(STDERR_FILENO,
           "[cos] qol-probe f=%u %s:%d pos=(%.1f,%.1f,%.1f) spd=%.2f ang=%04x roll=%d ev=%d \"%s\" msg=%u:%u "
           "eye=(%.1f,%.1f,%.1f) ship=%.2f@(%.0f,%.0f,%.0f) do=%u sw=%d xyz=%02x,%02x,%02x%s\n",
           frames, dComIfGp_getStartStageName(), (int)fopAcM_GetRoomNo(player), player->current.pos.x,
           player->current.pos.y, player->current.pos.z, player->speedF, (unsigned)(u16)player->shape_angle.y,
           player->checkFrontRoll() ? 1 : 0, ev->mEventId, evName, msgNo, msgStatus, eye.x, eye.y,
           eye.z, ship != nullptr ? ship->speedF : -1.0f, ship != nullptr ? ship->current.pos.x : 0.0f,
           ship != nullptr ? ship->current.pos.y : 0.0f, ship != nullptr ? ship->current.pos.z : 0.0f,
           (unsigned)dComIfGp_getDoStatus(),
           sSwitch >= 0 ? (int)dComIfGs_isSwitch(sSwitch, fopAcM_GetRoomNo(player)) : -1,
           (unsigned)dComIfGp_getSelectItem(0), (unsigned)dComIfGp_getSelectItem(1), (unsigned)dComIfGp_getSelectItem(2),
           act);
}

} // namespace pc
