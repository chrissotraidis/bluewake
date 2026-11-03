# Original BG collision in actor and room lifetimes — 2026-09-06

- Gate: P4 native player/camera runtime, BW-P4-0126 still active.
- State: ADVANCE within the collision dependency; not P4 completion.
- Base checkpoint: `254ccf7`; Apple Silicon macOS; locked `GZLE01-disc-image`.
- Change: compose already-qualified original source owners; no new dependency
  patch or pin change. Ordered source series remains through 0149.

## Actor and room result

The existing private BG profile probe now uses original cBgW/dBgW construction,
Set, polygon filters, cBgS/dBgS registration, ground/line queries, and release.
The former collision implementations in the shared test-fence file are excluded
from this target, not substituted with another successful response.

The original actor profile creates three models and real Room44 collision from
the resource-owned native DZB. Five acceleration arrays are owned by its solid
heap; static vertices alias the independently owned resource as intended.
Queries return the actual actor, own-actor exclusion works, and polygon safety
tracks real manager registration. Deletion clears the registration and rejects
the saved polygon reference. Recreation while the retired heap range is
occupied/poisoned builds fresh collision, rejects the old reference, answers
queries again, and cleans up with exact parent-heap recovery. Existing model,
animation, lighting, draw-submission, failure, and publication tests still pass.

The Room44 lifecycle probe also composes the original background owner. BG and
Stone2 occupy distinct real registry slots with correct actor identities and
game-owned acceleration arrays. Stone2 additionally owns its transformed
vertex buffer. Original ground/line queries select BG while excluding Stone2.
Original ClrMoveFlag clears the movement marker before each of two original
Stone2 executions; dBgW::Move republishes it after each. Stone2 deletion retires
only its slot, leaving the BG polygon valid. BG deletion retires its slot and
invalidates that polygon. Room and Ekao archive owners then retire separately,
as verified in the previous checkpoint. Every background registry slot is
unused before manager teardown.

The manager is explicitly placement-constructed in these isolated fixtures'
game-info storage; that is diagnostic initialization, not authentic game-info
or scene boot. Stone2's actor-correction/contact, audio, effects, and other
existing test seams remain explicit. The standalone historical Stone2 probe
still uses its older collision fence configuration; the composed room probe
is the new original-background evidence. No player movement/collision response
or complete scene acceptance follows merely from registration/query success.

## Measured composition and camera frontier

The first BG link failed because the shared query source list duplicated
JKRSolidHeap, already supplied by static_rel. Removing that duplicate source
resolved the build without replacing heap behavior. The room composition
similarly reuses math/check objects already provided by its Stone2 runtime.
BG call-counter assertions were replaced by stronger original registry,
query-result, ownership, and stale-reference assertions; unused counter APIs
remain only for historical fence consumers.

The camera Run link census now retains original functional background and
geometry sources instead of root-only background methods. It still fails
linking, but with the same 40 symbols in Debug, Release, and strict sanitizers,
down from 60. No update execution is claimed. The remaining classification is:

| Owner | Symbols |
|---|---:|
| Matrix/projection | 10 |
| Attention | 3 |
| Dynamic collision mass/camera state | 3 |
| Actor shared state | 5 |
| Actor/process search | 2 |
| Sea/wind | 3 |
| Event/demo | 5 |
| Graphics blur/state | 5 |
| Audio | 2 |
| Vibration | 1 |
| Assertion reporting | 1 |

Resolved link references are not equivalent to executed coverage: notably
camera roof/sphere paths and full camera update remain unexecuted. Previously
retained game-info/event/audio fences must still be qualified if reached.

## Verification

- BG actor create/query/draw/delete/recreate with poisoned retired heap:
  PASS in Debug, Release, strict ASan/UBSan.
- Room44 parent and Stone2 real-background composition/unload: PASS in all modes.
- Public default build and CTest: 68/68 PASS in all modes.
- Original synthetic background and real-DZB oracle probes: PASS in all modes.
- Toripost heap, McaMorf, historical standalone Stone2, and original camera
  construction regressions: PASS in all modes.
- Camera Run census: expected diagnostic FAIL, 40 unresolved symbols per mode.
- Player asset generator: five tests PASS.
- Source preparation through 0149 and whitespace check: PASS.
- Protected recompcore and dependency-lock SHA-256s: unchanged.
- Audit: only known 20 tracked local-research files; no new private-data issue.
- No app or Simulator launched; no speed or playable-session evidence.

Reproduce the existing private BG and Room44 lifecycle targets with the verified
disc path. Strict environment is
`ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1`.
Build `bluewake_route_b_camera_run_link_census` separately to reproduce its
remaining failure. Logs remain ignored in
`local-research/evidence/route-b-bg-functional-collision-20260906/`.

## Next action

Compose the original camera matrix/projection and attention/dynamic-collision
owners from the measured Run closure, then execute camera updates with actual
PLAYER integration. Do not return to an unrelated prop queue. Retain actor
collision/deletion and real-resource regressions as the supporting vertical
route. Continuous controllable Outset, normal boot, transition/save/reload,
and every later PRD gate remain required; the full goal is active.
