# Original PLAYER actor-manager composition — 2026-09-06

Base `d67ba75`; BW-P4-0126 remains active. No phase-three or P4 acceptance.
Apple Silicon macOS 26.6.2 / SDK 26.5, unchanged dependencies and GZLE01.

## Phase-three frontier

The new optional player_phase_three_link_census retains both original phase_2
and phase_3, not execute. It initially exposed 104 unresolved symbols in strict
configuration. makeBgWait selects starting behavior, which retains additional
actor/event/item/audio procedure dependencies. This is not proof that every
linked branch is reached during a particular start.

Original f_op_actor_mng.cpp compiled with one missing concrete JKRExpHeap
include, repaired by patch 0170. PLAYER now composes the full original actor
manager instead of the separate water-only and solid-heap-only fragments.
Original scene manager/iterator/tag owners support its scene lookup. The
existing native registry gains cDyl_IsLinked with the same resident-profile
contract as the older native process adapter, without adopting its unrelated
host allocation/initialization shims.

Three diagnostic camera actor methods (named fast creation, stage-layer
selection, named actor search) are no longer supplied as abort fences in the
PLAYER composition. The original bodies are linked instead. Other diagnostic
camera/stage/audio fences remain visibly abort-on-use. This change does not
claim that the newly linked actor-creation branches have executed.

## Complete original name owner

Removing those actor fences exposed missing dStage_searchName. The existing
stage-actor tier has only four entries and is not a complete lookup owner.
Patch 0171 moves the original full table and three lookup functions into a
shared source include, consumed by both the original full unit and a native
name-only compilation. All 825 OBJNAME rows compare byte-for-byte with the
pinned original source table. Initializer casts preserve signed-byte constants
on the native compiler; lookup algorithms and row ordering are unchanged.
The patch's large line count is primarily a source move, not new game logic.
The fixed synthetic dStage_getName2 diagnostic is excluded in PLAYER.

The public actor-names test checks 12 expected mappings spanning grass aliases,
door variants, Link, Ship, sea, NPCs and the final row, signed arguments/GBA
IDs, reverse lookup, first-alias selection, exact case-sensitive misses and
the original unknown-name fallback. This proves lookup, not actor creation.

The combined private probe executes original default and supplied/null actor
append construction. It checks owned allocation, every supplied coordinate,
angle and scale component, room/argument/parent/parameter identity, defaults,
and exact root-heap recovery. Native registry tests cover absent/known/unknown
profiles and continued residency after unlink. Existing original water and
actual PLAYER phase-two heap initialization continue to pass.

## Verification and limits

- Debug, Release and strict ASan/UBSan: actual phase two and accumulated
  player-model/camera/event/sea/attention/DZB/Toripost/McaMorf/BG/Stone2/Room44
  and camera constructor/matrix/mass regressions pass.
- Public CTest: 73/73 in every configuration, including the new name test.
- Deliberately failing production censuses, every configuration: phase two
  retains four audio symbols; phase-two/execute 95 (previously 118);
  phase-two/three 82 (initially 104). The intermediate actor-only composition
  had 84 before scene lookup and native linked-state support. These counts
  classify remaining link closure, not task counts or runtime coverage.
- Both preparers pass: TWW through 0171; Aurora 0001 invoked with bash.
  Shell syntax, whitespace, protected recompcore and lock hashes pass.
  Repository audit retains only the known 20 tracked research files.

Commands: build bluewake_route_b_private_player_init_probe in each
build/route-b-aurora-gx-MODE tree, run with the validated private disc; build
the optional player_create_link_census, player_runtime_link_census and
player_phase_three_link_census targets to reproduce their expected link
failures. Public: ctest --test-dir build/route-b-MODE --output-on-failure.
Strict environment: ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 and
UBSAN_OPTIONS=halt_on_error=1. MODE is debug, release or sanitize.

Ignored evidence: local-research/evidence/route-b-player-actor-manager-20260906.
No GUI or Simulator launched. No external blocker. Original phase three,
normal admission, PLAYER updates, drawing, input, audio and save/reload remain
unproven. Next: compose original event-control order/PID/compulsory owners
required by the measured phase-three closure; continue to actual makeBgWait
with real room ground and initial model calculation, then continuous updates.
