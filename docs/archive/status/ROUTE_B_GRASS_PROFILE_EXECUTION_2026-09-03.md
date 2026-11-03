# Route B GRASS Profile Execution - 2026-09-03

## Boundary

Execute all 119 exact Room44 GRASS requests through the original profile and
standard actor framework. Qualify only placement into the shared grass, tree,
and flower packets. Packet simulation, ground projection, collision, audio,
particles, and rendering remain outside this tier.

## Result

The unchanged original `g_profile_GRASS` now runs for every title-room
`kusax1`, `kusax7`, `kusax21`, `flower`, `flwr7`, `flwr17`, `pflower`,
`pflwrx7`, `swood`, `swood3`, and `swood5` append. The test combines the
qualified exact room reloader with the static-REL registry and standard actor
creation. Non-GRASS requests are cancelled before any unavailable profile is
loaded.

Each request is checked independently for its parameter-derived kind and
offset-group multiplicity, first transformed position, Y coordinate, item
encoding, flower color type, and final process disposal. The exact room
publishes:

- 982 grass instances into the 1,500-entry packet;
- 61 tree instances into the 64-entry packet; and
- 130 flower instances into the 200-entry packet.

No packet clips in Room 44. Every temporary GRASS process and create request
is gone after the original method's deliberate `cPhs_ERROR_e`; only shared
packet data survives. Room-44 deletion clears all three linked owner lists,
and packet removal returns all play-state pointers to null. Forced allocation
failure publishes nothing.

`tests/route_b_grass_batch_owner.cpp` is a bounded native placement owner. It
ports the original packet construction, free-slot search, per-room linked
publication, and removal semantics required by GRASS. Its virtual draw methods
are inert fences, and it selects the non-overlap placement branch used by this
room tier. It is not evidence that vegetation update or drawing is complete.

## Verification

- Debug: 62/62 CTest tests pass; exact 119-request replay passes.
- Optimized Release: 62/62 CTest tests pass; exact replay passes.
- ASan/UBSan: 62/62 CTest tests pass; exact replay passes with
  `detect_leaks=0` because LeakSanitizer is unsupported on this macOS setup.
- Exact DZR SHA-256:
  `0c89307c89183f65fd8ea72595bd15855ca24f2441b91b1b4eb06a353a33ace9`.
- Protected recompcore files remain unchanged:
  `cpu_exception.c` = `5b8ebbc60520b8bc6ee2166c344d760a88a3d2f2419058eaeaa210f5ec07def6`;
  `hle_core.c` = `dcb97681b2d325cd9fbf23e7d359a6788aa3f05980174fc31fa299a7b3c4716a`.

No BlueWake process or Simulator was launched.

## Next Boundary

Promote only original `g_profile_Obj_Wood`. Its five `woodb` and four
`woodbx` Room44 requests are the remaining transient batch-contributor cohort.
Prove shared wood-unit publication, capacity and room ownership, allocation
failure, intentional proxy disposal, and reset before opening BG or any
persistent actor profile.
