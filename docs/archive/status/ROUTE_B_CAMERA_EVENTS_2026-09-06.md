# Original camera event services and serialized frontier — 2026-09-06

Base `3a6c930`, Apple Silicon macOS 26.6.2, private input alias
`GZLE01-disc-image`. BW-P4-0126 remains active; no P4 or playability promotion.
State: `RETRY_NEW_HYPOTHESIS` toward actual PLAYER/camera updates.

## Changes and original runtime evidence

The camera Run target now composes the qualified stage-resource/J3D owners
instead of its resource-destructor fences. Full original event manager, event
data, demo, vibration and vibration-pattern units compile (3,014 source lines).
Patch 0156 changes two strlen result variables from u32 to size_t, preserving
the original bounded name comparison. No event behavior is reimplemented.

The optional `bluewake_route_b_camera_event_probe` executes original methods
against typed native records, with independent expected outcomes:

- Staff selection respects event control mode, active event states, excluded
  ALL staff, name and tag. Inactive and missing matches return -1.
- Advance flags, cached action selection, forced exact/prefix matching and
  cut-completion bits 31/10239 behave as expected. Reset clears flags and
  resource pointers; invalid staff -1 does not modify them.
- Eight original demo-camera create/remove lifetimes preserve active-camera
  identity. Setters stage requested values and enable flags; getters observe
  the separately published game camera. Distinct inputs prevent a false
  round-trip oracle. These host allocation lifetimes do not qualify custom
  game-heap ownership or full JStudio playback.

The original control constructor is compiled through the existing play-state
owner tier. The test uses raw game-info storage and exits without pretending
to destroy an unconstructed global. Event progression, real PLAYER updates,
camera Run, vibration hardware/StartShock execution and audio remain untested.

## Actual serialized resource frontier

The optional disc argument mounts the real sea Stage archive and retrieves
event_list.dat from its DAT resource group. Independent big-endian reads
validate all seven section extents before comparing the raw native view.

| Section | Actual count | Raw native count |
| --- | ---: | ---: |
| Events | 252 | 4,227,858,432 |
| Staff | 992 | 3,758,292,992 |
| Cuts | 3,386 | 973,930,496 |
| Data | 4,228 | 2,215,641,088 |
| Float values | 4,231 | 2,265,972,736 |
| Integer values | 1,849 | 956,760,064 |
| String bytes | 5,232 | 1,880,358,912 |

Original setData overlays the native header and derives pointers directly
from its offsets. It is deliberately NOT called on these unconverted bytes.
Disc mode exits exactly 2 after identifying this frontier; this is a passing
diagnostic expectation, not successful event resource admission.

Two preceding failures belonged to the harness: linking the synthetic arena
before the Aurora arena owner caused initialization to abort, and pathname
lookup at the archive root missed the DAT-group resource. Correct owner link
order and typed resource lookup resolve those failures without weakening arena
validation or altering game archive behavior.

## Remaining execution dependencies

Run's unresolved-symbol count falls from 16 to 8 in all three configurations:
five actor globals (Medli flight/mirror, Makar flight, possessed seagull and
cannon pointer), JAIZelBasic interface and seStart, and JUT assertion
confirmation. Resource/J3D composition first reduced 16 to 14; full event/demo
reduced to 9; vibration temporarily exposed two pattern tables before their
original source reduced the closure to 8. This is not a remaining-task count.

Important source gap: the pinned JAIZelBasic::seStart body contains only two
diagnostic prints and no return/implementation. It must be reconstructed or
replaced behind a validated service boundary, not linked as working audio.
Original vibration and pattern bodies are link-composed only at this point.

## Verification and next action

Debug, optimized Release and strict ASan/UBSan all pass the no-argument event
probe, all 69 default CTests, full PLAYER compilation, eight private probes
(sea, attention, DZB, Toripost heap, McaMorf, BG, Stone2 and Room44 lifecycle),
and camera constructor/matrix/mass-composed regressions. Disc event mode exits
exactly 2 in each mode with the same counts. Run fails with the same eight
unresolved symbols in each mode, with no duplicate-definition failure.

Five player asset-generator tests and locked private asset regeneration pass.
TWW preparation through 0156, Aurora patch verification, shell syntax and
whitespace checks pass. Dependency revisions and lock SHA256 remain unchanged:
`b9e74b327368a9bc6e3c994b6f8794dea92e06bf822512774fb42bf32a240aba`.
Both protected recompcore hashes are unchanged. Audit retains only the known
20 tracked local-research files; no new private artifact is added. No app
window or Simulator was launched, and no FPS/playability claim is made.

Build the event probe in each existing Aurora-GX Debug/Release/sanitize tree.
No argument expects exit 0; the private disc argument currently expects exit
2. Strict runs use ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 and
UBSAN_OPTIONS=halt_on_error=1. Reconfigure existing trees before requesting
the newly added target. Logs are ignored under
`local-research/evidence/route-b-camera-events-20260906`.

Next: admit event_list.dat through a bounded resource-owned native decoder,
validate section/cross-reference/string constraints and mutable record
lifetimes, then exercise original setData and camera event queries on real
records. Continue remaining Run owners toward actual PLAYER, not unrelated
props. The organizing milestone remains continuous visible Outset control,
then transition/save/reload and authentic boot. No external blocker.
