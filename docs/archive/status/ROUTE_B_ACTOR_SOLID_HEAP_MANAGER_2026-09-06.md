# Route B original actor solid-heap manager — 2026-09-06

## Measured result

Patch 0144 retains one original copy of `fopAcM_entrySolidHeap`, the reached
mDoExt create/select/restore/adjust/destroy services, and
`myMemoryErrorRoutine` plus its diagnostic helper. Source partitions exclude
unrelated actor, graphics, and machine initialization. The original manager
still owns estimated allocation, callback failure, maximum-size retry,
adjustment, and publication into `actor.heap`.

Toripost's private heap probe now calls this manager with the original
`0x7e0` estimate instead of directly creating an unlimited solid heap. Each
of eight cycles observes two original `_createHeap` calls, an increment in
the original solid-heap error counter, successful publication, 14 owned
pointers, a 2,432-byte adjusted heap, qualified BCK/calculation/packet behavior,
and exact parent-space recovery through original mDoExt destruction.

```
toripost-heap cycle=8 owned=14 heap=2432 reclaimed=1 manager=original retry=1 pass
```

The original error handler is essential: machine startup installs it in retail
source, and it records allocation failures without aborting. Using the JKR
default aborting handler would prevent the manager's intended retry. The
fixture explicitly installs the original handler; it does not claim full
machine startup. Zelda/command/archive heap identities and the actor diagnostic
name remain labeled test fixtures, used only for error reporting.

## Native corrections and negative evidence

The original main-thread assertions now compare against the native OS owner's
main-thread identity on TARGET_PC; PowerPC retains `&mainThread`. The public
host OS test verifies main-thread equality and original OS-worker inequality.
No assertion was disabled. mDoExt saves the prior current heap before child
construction on TARGET_PC, because JKR child construction can select itself
when root is current. Public manager tests require restoration to root and a
cleared saved-heap slot after every scenario.

The first strict sanitizer matrix failed in the new unallocatable-estimate
scenario: `JKRSolidHeap::create` applied offset 240 to a null allocation before
checking it. Moving the existing null check ahead of data-pointer calculation
preserves the intended null return and makes fallback defined. The negative
scenario remains in the default test suite; it was not weakened or removed.

New public `bluewake_route_b_solid_heap_manager_test` runs eight repetitions
of six scenarios: close estimate, generous estimate, no estimate, insufficient
estimate, unallocatable estimate, and callback failure at both estimated and
maximum capacities. It checks callback counts, current-heap restoration,
owned-pointer membership, unpublished failed heaps, and exact free-space
recovery. The private Toripost probe also retains direct undersized callback
failure and adds manager-owned two-callback failure cleanup.

## Verification

Base checkpoint: `a6bed4c`; Apple Silicon macOS 26.6.2 (25G83). Private input
alias: `GZLE01-disc-image`. Dependency-lock SHA-256:
`3d971036aa839d87cd4c2e4bf045f3cf963ca983f92b5881222e3b9bea4e4dd7`.

- Full public builds and **67/67 CTests** pass in Debug, optimized Release,
  and strict ASan/UBSan; final CTest elapsed times 1.63s, 1.23s, and 5.68s.
- Exact private Toripost heap, McaMorf, BG, Stone2, and Room44 parent probes
  pass in all three configurations, rebuilt against the final patch.
- `scripts/prepare_route_b.sh --check` passes through 0144; whitespace checks
  pass; both protected recompcore SHA-256 hashes remain unchanged.
- Repository audit still fails only for the same 20 tracked private-evidence
  files; dependency lock remains valid. No new private evidence is tracked.
- No BlueWake window or Simulator was launched.

Reproduction: configure/build `route_b` in each `build/route-b-{mode}` tree,
then run `ctest --test-dir build/route-b-{mode} --output-on-failure`. Private
targets use `build/route-b-aurora-gx-{mode}` and the user's validated disc input.
Strict runs set `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1`. Logs, including the initial sanitizer failure,
are retained privately in
`local-research/evidence/route-b-solid-heap-manager-20260906/`.

## Limits and next action

This qualifies the default original heap policy, not optional
`HeapAdjustEntry` exact-size compaction. Complete game startup, thread misuse
failure dispatch, process-wide leak freedom, BG/Stone2 heap-adapter replacement,
and Toripost process lifecycle are not accepted by this checkpoint.

BW-P4-0124 remains active: independently compose original resource loading,
createInit/fresh-save WAIT, idle execute/draw, and delayed deletion using this
qualified manager. Interactive mailbox/event/item/audio dispatch and collision
response remain closed, followed later by parent composition. The durable
BlueWake product goal remains incomplete; mobile remains deferred.
