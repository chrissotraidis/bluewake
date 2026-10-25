# Route B BG/MoveBG allocation ownership — 2026-09-06

## Result

Patch 0143 extends the explicit `JKR_NEW` policy to nine original source
allocation sites: BG material animation, BTK/BRK objects and wrappers, TEV
state, BG collision, MoveBG collision, and shared animation frame control.
Global host allocation remains unchanged. Together with patch 0142, this
closes BW-P4-0125 for the measured reached actor graph, not all game code.

The exact Room44 BG probe checks actor-heap membership for models, packet
arrays, TEV, collision, the present BTK wrapper/animation/frame control, and
every nonnull material-animation pointer in shared model data. Both standard
create/execute/draw/delayed-delete lifetimes return the game heap to its exact
prior free-space count. Between lifetimes the test reserves and overwrites
the full retired heap range with `0xa5`, forcing the second actor heap to a
different address. All cached pointers belong to the second heap and every
guard byte remains untouched after its deletion.

This is supported by original source behavior: `daBg_c::createHeap` clears
every shared model material's animation pointer before rebuilding animation
state. No new actor cache-reset behavior was added. Deletion alone does not
clear those borrowed references; this proof covers original recreation before
reuse, not arbitrary consumers of shared model data after actor destruction.

Stone2 now proves its MoveBG collision owner belongs to its actor heap and
that one complete standalone lifecycle restores exact game-heap free space.
The separate exact Room44 parent regression also checks BG and Stone2 owned
pointers and retains its existing child-before-parent unload/reset assertions.
It is not a second standalone Stone2 lifetime in the same process.

## Failure boundary and fixture correction

A `0x100` solid-heap limit drives the original BG heap callback to failure.
The existing test adapter destroys the partial heap, restores the previous
current heap, leaves the actor heap unpublished, and returns all parent space.
The adapter now captures the current heap before creating the child, because
JKR construction can itself change the current heap when root is selected.
Its frame-control allocation also follows the explicit JKR policy.

Attempting this failure through full standard BG creation reached the original
`dStage_escapeRestart` abort fence. That fence is unchanged: a TARGET_PC
diagnostic bridge invokes the original heap callback directly for the negative
test. Neither the production manager's retry nor the full scene-restart error
flow is qualified by this fixture. The failed actor fixture explicitly seeds
its heap pointer, as standard process allocation normally does.

## Verification and limits

- Debug, optimized Release, and strict ASan/UBSan: exact BG, Stone2, Room44
  parent, Toripost heap, and McaMorf probes all pass. Toripost retains eight
  successful cycles with 14 owned pointers and 2,432-byte adjusted heaps.
- Full public builds and 66/66 CTests pass in each mode.
- Patch preparation passes through 0143; whitespace checks pass. Protected
  recompcore hashes and dependency lock are unchanged.
- Repository audit retains its known failure for exactly 20 already tracked
  private-evidence files. No new private input or evidence is tracked.
- No BlueWake window or Simulator was launched. Private input alias:
  `GZLE01-disc-image`. Logs are retained privately under
  `local-research/evidence/route-b-bg-movebg-allocation-20260906/`.

Strict sanitizers use `detect_leaks=0:halt_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1`. Heap membership/free-space/poison tests are
explicit lifetime evidence, not a process-wide leak detector. Existing BTK
sampling/frame-control and collision test seams remain labeled; this adds no
functional collision or rendered-pixel acceptance. Exact Room44 contains three
BDL models, one BTK, and no BRK: BRK allocation follows policy but its behavior
remains unqualified. The parent still cancels 48 remaining requests.

## Next loop

BW-P4-0124 is the sole active blocker: independently qualify original Toripost
resource/solid-heap creation, fresh-save WAIT, idle execute/draw, and delayed
delete, including the real heap manager retry if reached. Mailbox/event/item,
audio dispatch, collision response, and parent composition remain closed.
This checkpoint does not complete BlueWake or advance mobile work.
