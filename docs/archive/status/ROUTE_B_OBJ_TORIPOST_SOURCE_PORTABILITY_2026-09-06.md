# Route B OBJ_TORIPOST Source Portability - 2026-09-06

## Boundary

After exact parent composition retained Stone2, Room44 still had 48 cancelled
requests. `OBJ_TORIPOST` and `OBJ_IKADA` were the two smallest remaining
one-request cohorts. Toripost is the smaller translation unit (1,056 source
lines versus 1,644) and its original source stopped at only two strict
target-PC representation errors.

## Result

Patch 0136 closes those two errors without partitioning behavior. The
collision-source complement is now explicitly represented as the destination
`u32` mask. J3D model user data now uses `std::uintptr_t`, matching the earlier
native-width J3D packet user-area contract, and Toripost passes its actor
pointer without truncation. On 32-bit PowerPC, `uintptr_t` preserves the
original field width and layout.

The complete original `d_a_obj_toripost.cpp` now compiles under the standard
Route B target in Debug, optimized Release, and strict ASan/UBSan. The object
target is included in every default build. No profile execution, resource
load, model construction, animation, collision, mail/event behavior, or draw
claim is made at this boundary.

Patch 0136 changes five lines across the 1,056-line actor source and 177-line
J3D model header: 0.41% measured adaptation density. The unpartitioned Debug
object has 107 unique undefined symbols. Optimized Release has 102 and the
instrumented sanitizer object has 137; those configuration differences are
expected from optimization and sanitizer runtime calls, and do not make the
full owner graph bounded.

## Evidence

- Debug object SHA-256:
  `ab9c2cdbb23aff8f3c5a5ea469851d80b4c5015ef10265a406d5845eee40d9e8`.
- Release object SHA-256:
  `540ec748f1aea456557982bc7574c7099e8f3a2ef8414aad2f62a0b22203ad1c`.
- ASan/UBSan object SHA-256:
  `f12a157e0df1f987ed79d6f0237e513650b18ca09f9023516d86e2396cfeae4b`.
- Full default Route B builds and all 65 public tests pass in Debug, optimized
  Release, and strict ASan/UBSan after the shared J3D model layout change.
- The exact Room44 parent, standalone Stone2, and standalone BG probes also
  pass in all three private configurations.
- Patches 0001 through 0136 replay cleanly from pinned TWW commit
  `03d27aa14389e648f51df2bc055ee3d53a3b67f2`.
- The protected recompcore files retain their required SHA-256 values.
- The repository audit remains failed only by the known 20 intentionally
  tracked `local-research/` evidence files; this checkpoint adds none.

## Limitations

The 107-symbol Debug owner surface includes archive/resource and solid-heap
ownership, J3D morph/model/animation, actor collision and ground correction,
mail and save predicates, event/talk/present flow, particles, audio, item
delivery, lighting/shadow, and process services. Linking that graph unchanged
would overstate a much broader actor lifecycle than the next proof requires.
The exact Room44 Toripost request remains cancelled.

## Next Boundary

Census and partition Toripost's unresolved owner surface around the exact
Room44 idle lifecycle. Preserve archive acquisition, model/animation and
solid-heap construction, create initialization, stable wait execute/draw, and
delayed resource/heap teardown. Classify mail delivery, talk/present/event,
item, particle, audio, crash, and non-idle branches explicitly before
registering one independent standard request. Keep parent composition, the
other 47 requests, another BlueWake process, and another Simulator closed.
