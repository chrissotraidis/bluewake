# Camera audio dependency reframe — 2026-09-06

Base `bab0895`; Apple Silicon macOS 26.6.2. BW-P4-0126 remains active.
State: REFRAME. No gameplay, audio playback or camera Run promotion.

## What eight unresolved symbols conceal

The Run census's eight symbols are not eight bounded implementation tasks.
Five are actor-state globals, one is assertion confirmation, and two are
JAIZelBasic interface/seStart. The latter includes missing behavioral source
below the public entry point, not just build composition.

The private GZLE01 DOL was revalidated against SHA1
8d28bab68bb5078c38e43f29206f0bd01f7e7a67. Direct PPC opcode inspection of
seStart's public symbol range 0x802A6720–0x802A8550 finds 7,728 bytes / 1,932
instructions, 81 direct linked branches to 28 distinct targets, and no
indirect linked branch within that function. This is static evidence, not an
executed call graph or proof that deeper callees have no indirect calls.

The body calls startSoundVec four times, itself twice, seStop fifteen times,
and also changes BGM/menu/event state, routes creature/level-object sounds,
transforms positions and adjusts sound parameters. The pinned source body
has only two diagnostic prints and no implementation/return. Replacing it
with a null return would discard gameplay-adjacent effects, not merely mute
an output device.

Further source inspection distinguishes useful source from empty bodies:

- JAIBasic::startSoundVec, startSoundDirectID, startSoundBasic and
  processFrameWork are empty.
- JAInter::SeMgr::storeSeBuffer, checkNextFrameSe and checkPlayingSe are empty.
- JAInter::SoundTable::getInfoPointer has real category/index lookup logic;
  it is NOT an empty function. Its initialization still overlays serialized
  u16 counts/offsets and needs native resource qualification.
- JAIAnimation has real animation logic, but its vector wrapper loses pointer
  width through the incorrectly typed Actor record.

Camera's three immediate sound IDs are manual-camera denial, switching to
manual camera, and pirate-ship creak. They enter the same broad seStart method;
their presence does not establish that all branches execute in Outset.

## Reproduced portability defect and repair

New optional target bluewake_route_b_audio_dispatch_census_objects compiles
four FULL source units: JAIBasic, JAISoundTable, JAISeMgr and JAIAnimation.
The initial compile fails with four diagnostics: two path-length size_t-to-u32
conversions in JAIBasic::initResourcePath and two pointer/integer conversions
in JAIAnimation::setAnimSoundVec. No warning is suppressed.

Patch 0158 repairs the latter at its native owner. Retail startSoundVec and
setAnimSoundVec both store the same position pointer in Actor's first three
slots and a separate u32 context in its fourth. Direct DOL instruction checks
verify those register/stack stores, independently of source type guesses.
The native Actor third field is now void*, its fourth u32; the native vector
wrapper passes those typed arguments directly. Retail types/casts remain in
their existing target branch, and the null descriptor uses equivalent zeros.

Verified retail wrapper SHA256s (no binary bytes are tracked):

- startSoundVec: ca5c7faf6b068ecd84e3d4b8e0977e3eb9e703cc114449afa3dc7f54c720dd76
- setAnimSoundVec: c614195bea00d5178c46cbdba0012499c1f469438acc252e5b982038e76fbf65

The new public audio-actor test uses real distinct native addresses, confirms
the identity address is above UINT32_MAX, dereferences the preserved identity,
checks four independent context values, vector-wrapper aliasing and null
records. It tests the original typed constructor, not sound dispatch/playback.
The full animation source now compiles; the census retains the two JAIBasic
path-length errors. Empty source bodies compiling is explicitly not evidence
that their behavior exists.

## Critical-path decision

Verification: 71 public CTests, full PLAYER compilation and all existing
private event/sea/attention/DZB/Toripost/McaMorf/BG/Stone2/Room44 and camera
constructor/matrix/mass regressions pass Debug, Release and strict ASan/UBSan.
The audio census reproduces exactly the two remaining path-length errors in
all modes; Run still has the same eight unresolved symbols. Five generator
tests, private asset regeneration, both preparers, shell syntax and whitespace
checks pass. Lock SHA256 remains
b9e74b327368a9bc6e3c994b6f8794dea92e06bf822512774fb42bf32a240aba; protected
runtime hashes are unchanged. Audit retains only the known 20 tracked
local-research files. No app window or Simulator was launched. Private logs
are ignored under `local-research/evidence/route-b-audio-actor-20260906`.

Keep continuous visible Outset control as the organizing milestone. Do not
turn every sound or actor into a separate proxy checkpoint, and do not infer
that all of audio must be finished before the first diagnostic camera update.

Next, compose the remaining source-authentic actor/assertion owners and
measure original camera/PLAYER execution requirements. If a diagnostic-only
audio fence is needed to retain the camera body, it must report a unique
failure and stop when reached; it must not return fabricated successful audio
or establish a normal-boot/playability gate. Record actual audio calls reached
before deciding which reconstruction blocks that milestone.

The eventual audio implementation must reconstruct sound dispatch/registration
and per-frame processing over qualified table/bank/sequence resources, then
restore seStart's effects and prove output. Fixing its link symbol alone does
not satisfy that requirement. No route pivot or product scope reduction has
been made. No external blocker.
