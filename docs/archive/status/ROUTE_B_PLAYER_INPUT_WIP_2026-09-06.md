# Original PAD polling / dependency ABI WIP — 2026-09-06

Stable checkpoint `d766d29`; BW-P4-0126 remains active. No P4 promotion.

**Superseded investigation:** the stationary gait below was traced to zero
actor scale from bypassing original actor initialization. The verified
input-driven locomotion result, full regression and remaining product limits
are recorded in `ROUTE_B_PLAYER_LOCOMOTION_2026-09-06.md`. This document keeps
the earlier failures and disproved hypotheses as chronological evidence.

## Latest continuation — initialization passes, stationary gait frontier

0198 shares existing original JAIZelBasic makeSound/checkStreamPlaying,
JAIBasic initCamera/setCameraInfo and StreamMgr init/status with the PLAYER
composition. Real JAIZelSound source supplies derived allocation behavior.
The diagnostic publishes a 2 MiB JAI heap before original construction,
parses the real JaiInit sound table through the validated parser, copies it
into retained heap storage, then runs original table, SE and stream init.
Original default listener initialization follows. No full audio startup,
PLAYER listener, active stream/DSP or sound-output acceptance is implied.

The initial link needed a closed setSeInterDolby backend method because the
real derived vtable now retains its distance method. Other backend fences
remain explicit aborts. The first parser integration linked the aggregate
static-REL library and introduced duplicate process globals; strict ASan
reported an ODR violation for cMl::Heap before main. This was corrected by
composing only the pure parser source, not disabling ODR detection. Sharing
listener init initially added unresolved dependencies to the isolated SE
test; the PLAYER-specific registration object composition now enables it
without altering that test's fixture contract. A missing Camera definition
include was also corrected before runtime replay.

Eight input-fed updates then passed strict checks. Extending to 120 exposed
hat animation's float-to-s16 phase conversion at update 17: -35976 was out
of range. Retail 8011C9CC–8011C9D8 and 8011CA40–8011CA4C truncate to an
integer then mask the low halfword before cosine lookup. 0199 preserves
that conversion for the two bounded halfword-derived phase expressions.
Console code is unchanged. No general floating-angle audit is claimed.

The final strict run completes all 120 original input-fed updates and
collision Move with exit 0. Input magnitude remains 0.518519 in CPAD and
Link; position remains (-192701,550,318904) and velocity is zero. This is a
stationary diagnostic replay, not locomotion or sustained gameplay.

Locomotion investigation: a function-entry LLDB breakpoint read the `this`
parameter before the prologue had established its location; its zero/null
field readings were invalid, and evaluating a method on that apparent null
pointer triggered a debugger-induced sanitizer abort. This is not a game
failure. Repeating at source line 2353 (after the prologue) showed a valid
actor/history pointer. At approximately input update 98, original state is:

- mCurProc = 6 (MOVE), mNormalSpeed = 4.57064438;
- mOldFrameFlg = true, mOldFrameRate = 0;
- m3598 = 1 (full foot-motion weighting).

Thus the missing old-history hypothesis is disproved. Source posMoveFromFootPos
weights normal speed by (1-m3598) and relies on animated foot displacement
at this blend. Next audit real BCK samples, changing joint transforms and
foot displacement as a coherent animation/locomotion owner. Do not force
old-frame flags, normal speed, blend or position. The exact cause of absent
foot displacement remains unproven.

Verification: strict final PLAYER run exit 0; focused public SE-registration
and player-audio-feedback tests 2/2 pass after composition repair. Both
preparers passed for 0198; the 0199 build's source preparation and patch
numstat pass. Diff check and protected core hashes pass. Full Debug/Release/
public/private matrix and independent angle controls are still pending;
no stable checkpoint/push. All confirmed tool sessions have completed.

Logs: ignored `/tmp/bluewake-player-audio-init-*` and
`/tmp/bluewake-player-locomotion-{lldb,line-lldb,history-lldb}.log`.
Alternate patch baseline index:
`/tmp/bluewake-player-audio-init.VjcE8O/index`. This index contains selected
pre-0198/0199 source versions; export only the relevant files.

## Latest continuation — six input-fed updates, audio initialization open

PLAYER replaces animation-state-only objects with the qualified playback
objects and adds existing SE registration objects. Duplicate direct Basic/
solid-heap objects are removed from this target, and the Basic makeSound
diagnostic fence is excluded only for PLAYER execute. Original JAIConst
supplies its real globals/constructors. This composes existing source-backed
behavior; it does not initialize the whole audio runtime.

First link exposed eight missing services. Registration composition exposed
20 missing entries including virtual sound methods and sequence/stream
branches. A dedicated PLAYER backend-fence file supplies uniquely logged
aborts, not the isolated SE test's recording/success fixtures. Its sequence
pointer and stream flags are explicitly link-only storage, not qualified
runtime owners. Original checkStreamPlaying is not yet composed.

Native PSMTXMultVec delegates to Aurora's scalar C_MTXMultVec. Non-identity
integer-coordinate and in-place alias controls pass; bit-exact paired-single
rounding for arbitrary floats has not been qualified. No matrix identity
stub is used in PLAYER.

Strict composed replay now completes six original input-fed updates/Move.
Both CPAD and Link's stick magnitude are 0.518519. Link is observed through
the original public getAnmSpeedStickRate(0,1) expression; direct diagnostic
access to private stick/normal-speed members failed compilation and was
removed rather than widening their visibility. Position remains
(-192701,550,318904), all reported velocity components zero. This proves
input consumption and bounded update execution, not locomotion.

The seventh input-fed update aborts with
`BW-PLAYER-AUDIO-BACKEND-UNQUALIFIED JAIZelBasic::checkStreamPlaying`.
The planned eight-update loop still fails overall (exit 134). The old
setAnimSound fence is no longer the reached blocker.

Next work is the audio initialization ownership as a whole: original stream
initialization/status, JAIZelBasic's real JAIZelSound allocation, real sound
table/category pool setup and listener state. Existing private registration
tests are useful regression evidence but their fake stream status, identity
matrix and sequence responses are not game runtime implementations. Keep
backend fences until genuine behavior is composed and observed.

Latest build and matrix controls pass under strict ASan/UBSan; final runtime
stops at the named fence, without a sanitizer diagnostic. Diff check passes.
No broad configuration replay, stable checkpoint or P4 promotion. Logs:
`/tmp/bluewake-player-input-playback-build.log`,
`/tmp/bluewake-player-input-registration-build.log`,
`/tmp/bluewake-player-input-audio-composition-build.log`,
`/tmp/bluewake-player-input-composed-*`, and
`/tmp/bluewake-player-input-movement-state-*`. All confirmed build/probe
sessions have completed; no GUI or Simulator launched.

## Latest continuation — input reaches PLAYER animation

The dependency hypothesis is confirmed. Explicit CMake reconfiguration was
required before building the new target (the first build attempt reported
no rule for that target). Source-built Abseil 20240722.0 passes strict empty
iteration, mutation, copy/move and reuse. Original neutral polling and two
PLAYER updates/Move then pass with exit 0. No sanitizer checks were disabled.

Provider provenance: public tag commit
`4447c7562e3bc702ade25105912dce503f0c4010`, Apache-2.0 license inspected in
the fetched source, archive SHA256
`f50e5ac311a81382da7fa75b97310e4b9006474f9560ac46f54a9967f07d4ae3`.
`cmake/RouteBAbseil.cmake` selects it before Aurora imports another provider;
the dependency lock records WIP qualification, not full promotion.

Original PAD/JUTGamePad/CPAD main X, camera Y, A rising/held/release and L
trigger transitions pass. Forward Y exposed float-to-s16 overflow at
JUTGamePad.cpp:328. Retail 802C434C–802C4358 uses fctiwz/stfd/lwz/sth,
truncating to an integer then storing the low halfword. Patch 0197 explicitly
preserves that wrap without a floating conversion outside s16 range. All
65,536 signed-byte input pairs pass strict finite/range checks and cardinal
angle controls. This is not an exhaustive retail angle-value differential.

The forward-input execution now aborts at the existing animation fence:
`JAIZelAnime::setAnimSound -> daPy_lk_c::execute:11514`, confirmed in LLDB.
The fence now prints `BW-PLAYER-ANIMATION-PLAYBACK-UNQUALIFIED` rather than
silently aborting. No movement frame completes; the planned eight-update
loop does not pass. Next compose the existing qualified original scheduler
and registration owners into PLAYER, not synthetic voices or a skipped call.

Latest strict build passes; reset-switch host test passes; final execute
probe intentionally exits 134 at that named fence after angle/transport
controls. Source preparer and git diff checks pass, protected core hashes
unchanged. Full Debug/Release/public/private regression matrix remains
pending. No stable commit/push or gameplay claim.

Evidence logs are ignored `/tmp/bluewake-player-input-source-absl-*`,
`/tmp/bluewake-player-input-controls-*`, `/tmp/bluewake-player-input-angle-*`,
`/tmp/bluewake-player-input-final-*` and
`/tmp/bluewake-player-input-host-test.log`. The angle backtrace log uses
LLDB's crash-command option; ordinary post-run commands did not execute
after the signal in batch mode. The first 0197 export omitted a trailing
context line, was rejected by the preparer, and was corrected/rechecked.

The sections below describe the preceding dependency-blocked state.

## Progress and current failure

The private PLAYER execute composition adds full original JUTGamePad,
JUTGba, GBA communication, reset and f_ap_game sources, plus Aurora PAD/OS
libraries. Original gamepad construction, initialization, mDoCPd_Read and
PADRead are now reached. Non-neutral input has not been delivered to Link.

Full GBA communication initially failed compilation at two receive-buffer
casts. Source inspection showed these are local buffer-registration addresses:
four AGB actor assignments publish addresses into TestDataManager. 0195 gives
the native slot `void*` storage and removes those four narrowing publications
under TARGET_PC, preserving the console branch. All communication methods now
compile without an initialization-only replacement tier. Transfer decoding,
bounds, endian handling and the AGB actor remain unqualified. No GBA transfer
is attempted; original mDoGaC_Initial leaves communication disabled.

PAD bridge review before execution found an ABI mismatch: Aurora appends a
native extButton field to PADStatus, while TWW still declared 12 bytes. 0196
matches the 16-byte native ABI and preserves console layout. The probe checks
every field offset. The extension declaration uses the TWW PADStatus type to
avoid defining both SDK structs. The first patch export was empty because the
tracked path is `include/dolphin/pad/Pad.h`, not lowercase `pad.h`; corrected
export now contains the actual change. Never replay an empty patch.

The diagnostic constructs one original JUTGamePad, publishes its pointer to
the CPAD table, supplies reset-data storage through the original setter, calls
original GBA initialization, and submits neutral Aurora virtual PAD status.
This is not full mDoCPd_Create/normal controller startup; no GBA worker threads
are started. The host reports a uniquely logged absent physical reset switch.
Original fapGm_HIO construction supplies trigger thresholds. No stick fields
inside Link or the CPAD output are directly assigned.

## Earliest current boundary

Strict execution aborts in Aurora PADRead before game input conversion:

`get_controller_for_player -> absl::flat_hash_map::begin/end ->
HashSetIteratorGenerationInfoEnabled`: null GenerationType pointer at
`raw_hash_set.h:645`.

Standalone reproducer with no Aurora/game code:

```cpp
#include "absl/container/flat_hash_map.h"
int main() {
  absl::flat_hash_map<int,int> map;
  return map.begin()==map.end() ? 0 : 1;
}
```

Compiling with `-std=c++20 -fsanitize=address,undefined` and pkg-config's
`absl_flat_hash_map` flags reproduces exit 134 and the identical null-generation
failure using Homebrew Abseil 20260107.1. Both Aurora core and PAD compile with
the same sanitizer flags; the external library is unsanitized. Its headers
enable SwissTable generations when ASan is enabled. This is a dependency
configuration boundary, not a Link/GBA/controller-map initialization defect.
Do not define NDEBUG_SANITIZER or otherwise disable the failing checks.

## Next action

Build Abseil from pinned source with matching sanitizer settings and consistent
headers/libraries, qualify the standalone reproducer, then rerun original
PADRead. Aurora already has a pinned standalone fallback (20240722.0), but
currently prefers the system package. Review that provider selection and
record any dependency/configuration change before promotion. Preserve the
stable checkpoint and avoid treating old-system build results as fresh.

Then qualify neutral/axis/button-edge/release transport across Aurora PAD,
original JUTGamePad and CPAD, and execute actual Link behavior with delivered
input. Scene-request progression, valid path metadata, PLAYER camera/draw,
normal boot and the full PRD remain open.

## Verification / resume handles

- Current private target builds, but neutral strict poll fails as above.
- Public/regression/Debug/Release validation of 0195–0196 remains pending.
- No stable commit/push this increment; all changes are WIP.
- No app or Simulator launched, no game/save data changed.
- No confirmed live tool/build sessions remain at this handoff.
- Logs: ignored `/tmp/bluewake-player-input-build.log`,
  `/tmp/bluewake-player-input-run.log`,
  `/tmp/bluewake-player-input-absl-reproducer.log`.
- Standalone source/binary and alternate patch-baseline index:
  ignored `/tmp/bluewake-player-pad.sYux6z/`.
- No external blocker; continue dependency-owner repair.
