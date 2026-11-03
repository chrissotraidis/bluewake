# Original PLAYER particle services — 2026-09-06

Base `78110c6`; BW-P4-0126 remains active. No actual PLAYER construction,
continuous control, particle rendering or P4 promotion.

## Original owners and reproduced native gap

The creation census now composes the full original d_particle unit against
the native game headers, rather than its earlier construction-only include
boundary. Original JPAMath, emitter, particle, draw, draw visitors, TEV setup,
random and GFPixel units supply the retained callback dependencies. No new
successful substitute callback was introduced. Existing demo-construction
fences omit their duplicate particle-manager global when these owners link.

GFSetBlendModeEtc exposed a native FIFO gap: original GFWriteBPCmd still
wrote the GameCube GXFIFO address. Patch 0160 routes its native branch through
Aurora GXCmd1u8/GXCmd1u32; the retail writes remain unchanged. The two original
mDoExt color/alpha-update packet draw methods now emit the independently
expected bytes `61 41 00 00 14` and `61 41 00 00 0c`, inside a 32-byte padded
Aurora display list. This checks command transport and blend bits, not pixels.

Patch 0161 makes the existing original double-precision normalization helpers
in d_kankyo_rain independently composable without retaining the entire rain/
weather unit or redefining already-owned wave initialization. No helper body
changes. Tests cover the 3-4-5 displacement and coincident-point zero result.
Original black/white game-color globals are defined with their original RGBA
values for the raw-singleton PLAYER composition.

## Executed callback contracts

The existing private_player_model_probe now also checks eight typed emitter
fixture lifetimes, each with 32 original follow callback updates:

- setup publishes the emitter and establishes immortal/continuous flags;
- translation follows all three components of the changing position exactly;
- removal clears callback ownership, stops emission and removes immortality;
- repeated removal remains bounded;
- original smoke construction/setup/end preserves emitter identity and the
  configured wind-off user-data value.

Emitter storage is zero-initialized explicitly before its original base
constructor. This is a typed input fixture, not JPA resource-backed emitter
admission, a simulated successful particle manager, or proof of spawn/draw.
Smoke lighting/draw, ripple water queries and particle field processing do
not execute in these cases. The probe supplies one explicit aborting
fopAcM_getWaterY boundary so retained, unexecuted water callbacks cannot silently
return invented heights. The separate PLAYER censuses still leave that
original function unresolved; the runtime-only boundary does not lower their
counts. All prior model/frame/heap and 120-camera-update assertions still run
in the same combined executable.

## Remaining creation dependencies

The phase-two-only census retains 12 symbols, down from 19. Creation plus
execute retains 141, previously 156. The creation-only set is:

- fopKyM_create and fopAcM_getWaterY;
- dSv_info_c::isSwitch/onSwitch and dSv_event_c::isEventBit;
- JAIZelBasic::linkVoiceStart, getLinkVoiceVowel, checkPlayingSubBgmFlag and
  checkPlayingMainBgmFlag;
- JPAFieldManager::init;
- dDlst_mirrorPacket::init and its vtable.

These counts describe static retention, not executed startup calls or a count
of small implementation tasks. Mirror init is directly needed by playerInit;
several particle/audio methods are retained through virtual callbacks. The
next action is original mirror construction and its graphics/resource owners,
then the remaining actual phase-two requirements and real Link initialization.
No fake Link object or fabricated-success audio/water behavior may replace it.

## Verification

Public 72/72 tests, full PLAYER/model/particle source builds and the combined
private particle/model/camera probe pass Debug, Release and strict ASan/UBSan.
Existing private event/sea/attention/DZB/Toripost/McaMorf/BG/Stone2/Room44 and
camera constructor/matrix/mass regressions pass all three modes. Both link
censuses reproduce the counts above in every mode and remain deliberate
failures. Both preparers (TWW through 0161), eight generator tests, asset
regeneration, shell syntax and whitespace checks pass. Protected hashes and
dependency lock remain unchanged. Audit retains only its known 20 tracked
local-research files. No app window or Simulator is launched.

Private evidence is ignored under
`local-research/evidence/route-b-player-particles-20260906`. Reproduce the
combined probe by building bluewake_route_b_private_player_model_probe in an
Aurora-GX/DVD tree and passing the validated GZLE01 disc as its sole argument.
Strict runs use ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 and
UBSAN_OPTIONS=halt_on_error=1. Global fixture teardown remains outside this
probe's acceptance, as before.
