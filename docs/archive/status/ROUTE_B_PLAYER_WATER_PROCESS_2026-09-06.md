# Original PLAYER water and process support — 2026-09-06

Base `fad7a67`; BW-P4-0126 remains active. No actual PLAYER, continuous
control or P4 promotion.

## Composition and reached requirements

Patch 0164 gives original fopAcM_getWaterY a native independently composable
source branch, preserving the original function body. The combined probe's
aborting replacement is removed. Original dBgS::WaterChk/SplGrpChk and sea
query owners now supply its real behavior. Original cM2dGBox is also included
because executing sea updates in this combined executable retains it.

Full original f_op_kankyo_mng supplies environment creation. This exposes
process allocation, static module lookup and assertion dependencies rather
than completing actor admission. The composition now uses original c_malloc
and the existing StaticRelRegistry implementation, with a small native bridge
for cDyl_LinkASync/Unlink and fatal process assertions. Unknown/uninstalled
modules report the original error phase; known static modules remain resident.
No PowerPC REL code is executed, no profile is invented for Link, and the
older process-adjacent host malloc/no-op-init shim is not linked here.

## Executed contracts

Original cMl init/memalignB/free executes positive and negative alignment
cases, checks the exact owning JKR heap, handles zero-size/null free, and
recovers exact free space. Its prior heap pointer is restored. The production
native registry bridge is tested with no registry and a typed one-entry
registry for known/unknown cases; prior registry state is restored.
This is not an environment process lifecycle test.

After original background-world construction, actor water lookup rejects a
no-hit query with its original negative-infinity sentinel. A real Always-
resource sea is constructed in an owned solid heap and updated 32 times.
Original actor water lookup returns the original wave query height each time;
outside-area lookup rejects with the sentinel. Sea reset and heap destruction
recover exact parent free space; stage type and room state are restored.
The existing independent sea regression separately checks wave interpolation.
No water polygons are registered here, so polygon/sea maximum selection and
live ripple effects remain outside acceptance.

## Verification and next action

Public 72/72 CTests, combined process/water/save/field/mirror/model/particle/
camera checks and existing private camera/event/sea/attention/DZB/Toripost/
McaMorf/BG/Stone2/Room44 and camera constructor/matrix/mass regressions pass
Debug, Release and strict ASan/UBSan. Both preparers pass (TWW 0164, Aurora
0001); shell syntax, whitespace and protected/lock hashes pass unchanged.
Audit retains only its known 20 tracked local-research files.

Creation-only linking retains four audio symbols in every mode:
JAIZelBasic::linkVoiceStart, getLinkVoiceVowel, checkPlayingSubBgmFlag and
checkPlayingMainBgmFlag. Creation plus execute retains 118. Both censuses
remain deliberate failures. These static counts are not execution evidence.

Source inspection confirms the two BGM query methods have concrete bodies,
but the two Link voice methods are marked Nonmatching with absent bodies
(including a non-void empty method). They must not be treated as working
audio. Next: compose original BGM queries and measure/reconstruct voice
requirements, using explicit fail-on-use diagnostic boundaries only to expose
actual phase-two reachability. Actual PLAYER remains the organizing milestone.

No external blocker or GUI/Simulator launch. Ignored logs:
`local-research/evidence/route-b-player-water-process-20260906`.
Reproduce with `bluewake_route_b_private_player_model_probe` and the validated
disc path. Strict environment:
`ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1`.
