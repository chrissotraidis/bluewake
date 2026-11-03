# Route B Room Scene Phases 0-1 - 2026-09-02

## Boundary

Patch 0110 proved initial ROOM_SCENE request transport but stopped before the
request machinery consumed it. The next bounded owner was the real chain from
scene request through node request, standard create, profile lookup, scene
allocation, append-to-parameter transfer, and ROOM_SCENE creation phases 0-1.

## Result

Patch 0111 partitions the original ROOM_SCENE profile and unchanged phase 0/1
bodies, with phase 2 behind a hard fail-closed boundary. The public test starts
at `dStage_roomInit(44)` and drives the real request/process stack. The created
scene owns profile and parameter 44, and phase 0 publishes its actual process
ID to room status.

Phase 1 is exercised with a real `JKRExpHeap`. A live allocation keeps the
scene in creation phase 1 and suppresses the archive request. After that exact
allocation is freed, the same process resumes, submits `Room44` with the room
heap as owner, and stops in phase 2. A supplemental failure branch proves that
a rejected stage-resource request invokes `dStage_escapeRestart` and returns
`cPhs_ERROR_e`.

The patch adds 95 source lines across the 320-line room unit and 3,153-line
stage unit, a measured 2.74% partition/adaptation density. The retained retail
phase bodies have no semantic edits; the host-specific code is the partition
and phase-2 fence.

## Evidence

- ROOM_SCENE object SHA-256:
  `912571640116fcad6f721c3b194f42246d84a992f8637dd77c85c4d350905cf8`.
- Initial-room object SHA-256 after the exact memory-view addition:
  `0913565d2ae9f64f9e856be31f3f48a321d6de8e816d44b6e70960720bafa3f7`.
- Debug focused executable SHA-256:
  `72b5a82d3dc97c1e17add6093b06cf8ac3839f96479704230fd938105af7a4c6`.
- Patch SHA-256:
  `7404a58865023788d43e649b7be697b1d5e74ddde28a57648b9ed2b701629440`.
- Patches 0001 through 0111 replay cleanly from pinned TWW commit
  `03d27aa14389e648f51df2bc055ee3d53a3b67f2`.
- The focused test and all 52 public tests pass in Debug, optimized Release,
  and strict ASan/UBSan.

No room resource was synchronized or decoded, and no BlueWake app or Simulator
was launched.

## Next Boundary

Perform one source-and-private-asset census of Room44 phase 2 and the complete
22-tag `dStage_dt_c_roomLoader` table. Record present tags, counts, offsets,
serialized representations, process effects, and existing reusable stage-view
owners. Group the closure into cohesive tiers before implementing any room
handler. Keep room reload actors, maps, particles, another BlueWake process,
and another Simulator closed.
