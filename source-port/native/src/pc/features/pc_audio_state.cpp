// What JAudio is playing, for the Mac checks and the Switch debug server (pc_internal.h):
// - audioStateLine: one line with the sound effects JAudio has registered, the SE sequence's tracks
//   still sounding, the BGM handles and the two counters that tell a stalled audio thread apart;
// - audioSeRegistered: how many registered sound effects have a given id.
// Read on the game thread (the SE manager runs there, JAInter::SeMgr::processGFrameSe).
#include "pc_internal.h"

#include "JAZelAudio/JAIZelBasic.h"
#include "JSystem/JAudio/JAIGlobalParameter.h"
#include "JSystem/JAudio/JAISeMgr.h"
#include "JSystem/JAudio/JAISequenceMgr.h"
#include "JSystem/JAudio/JAISound.h"
#include "JSystem/JAudio/JASAiCtrl.h"
#include "JSystem/JAudio/JASDSPBuf.h"
#include "JSystem/JAudio/JASTrack.h"

#include <cstdio>

namespace pc {

int audioSeRegistered(u32 id) {
    if (JAInter::SeMgr::seRegist == nullptr) {
        return 0;
    }
    int n = 0;
    for (u32 c = 0; c < JAIGlobalParameter::getParamSeCategoryMax(); c++) {
        for (JAISound* snd = JAInter::SeMgr::seRegist[c].field_0x4; snd != nullptr; snd = snd->field_0x34) {
            if ((snd->mSoundID & 0xFFFF) == (id & 0xFFFF)) {
                n++;
            }
        }
    }
    return n;
}

// "dac-repeat <n> dsp-sync <n> bgm main <id> sub <id> stream <id> sailing <0|1> island <room>
//  mute <main>/<sub> n=<count> <id>:<state>[*] ... t<track>=<id>/<active><command> ..."
// dac-repeat: AI blocks that ended with no new mix (Kernel::updateDac replays the last block): it
// climbs when the audio thread stalls; dsp-sync: the DSP sub-frames mixed, it stops then. A BGM id with
// "(-)" has no handle. mute: the takt mode's factors of the main and sub BGM volume (0 while conducting).
// "*" marks the boat's cruise sounds (JAIZelBasic::shipCruiseSePlay's two handles). A track with
// "ORPHAN" and active 1 sounds with no sound in its slot: nothing would stop it.
int audioStateLine(char* out, size_t size) {
    const JAIZelBasic* z = JAIZelBasic::zel_basic;
    if (z == nullptr || JAInter::SeMgr::seRegist == nullptr || size == 0) {
        return snprintf(out, size, "JAudio not running");
    }
    const JAISound* go = (const JAISound*)z->field_0x1f44;
    const JAISound* wave = (const JAISound*)z->field_0x1f48;
    char list[1024];
    int len = 0;
    int n = 0;
    for (u32 c = 0; c < JAIGlobalParameter::getParamSeCategoryMax(); c++) {
        for (JAISound* snd = JAInter::SeMgr::seRegist[c].field_0x4; snd != nullptr; snd = snd->field_0x34) {
            n++;
            if (len < (int)sizeof(list) - 24) {
                len += snprintf(list + len, sizeof(list) - len, " %04x:%u%s", (unsigned)(snd->mSoundID & 0xFFFF),
                                (unsigned)snd->mState, snd == go || snd == wave ? "*" : "");
            }
        }
    }
    // the SE sequence's tracks, one per slot, numbered as JAInter::SeMgr::checkPlayingSe numbers them
    if (JAInter::SeMgr::seHandle != nullptr && JAInter::SeMgr::seHandle->mState >= 4) {
        JASystem::TTrack* root = JAInter::SeMgr::seHandle->getSeqParameter()->getRootTrackPointer();
        u8 track = 0;
        for (u32 c = 0; c < JAIGlobalParameter::getParamSeCategoryMax(); c++) {
            for (u8 slot = 0; slot < JAInter::SeMgr::categoryInfoTable[JAInter::SeMgr::seScene][c * 2]; slot++, track++) {
                u16 active = 0;
                u16 command = 0;
                const u32 port = 0x20000000 + (track >> 4) + ((track & 0xF) << 4);
                root->readPortApp(port + 0x20000, &active);
                root->readPortApp(port, &command);
                const JAISound* owner = JAInter::SeMgr::sePlaySound[c][slot];
                if (active != 0 && len < (int)sizeof(list) - 32) {
                    if (owner != nullptr) {
                        len += snprintf(list + len, sizeof(list) - len, " t%u=%04x/%u%u", (unsigned)track,
                                        (unsigned)(owner->mSoundID & 0xFFFF), (unsigned)active, (unsigned)command);
                    } else {
                        len += snprintf(list + len, sizeof(list) - len, " t%u=ORPHAN/%u%u", (unsigned)track,
                                        (unsigned)active, (unsigned)command);
                    }
                }
            }
        }
    }
    list[len] = '\0';
    return snprintf(out, size,
                    "dac-repeat %d dsp-sync %d bgm main %x%s sub %x%s stream %x%s sailing %u island %u mute %.2f/%.2f "
                    "n=%d%s",
                    JASystem::Kernel::JASUniversalDacCounter, JASystem::TDSP_DACBuffer::dacSyncCounter,
                    (unsigned)z->mMainBgmNum, z->mpMainBgmSound ? "" : "(-)", (unsigned)z->mSubBgmNum,
                    z->mpSubBgmSound ? "" : "(-)", (unsigned)z->mStreamBgmNum, z->mpStreamBgmSound ? "" : "(-)",
                    (unsigned)z->mIsSailing, (unsigned)z->mIslandRoomNo, z->field_0x0098, z->field_0x00a4, n, list);
}

} // namespace pc
