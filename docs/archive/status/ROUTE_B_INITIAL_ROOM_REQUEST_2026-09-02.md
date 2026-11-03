# Route B Initial Room Request - 2026-09-02

## Boundary

After the complete Outset stage loader returns, retail `dStage_roomInit(44)`
marks the room active, allocates an `int` carrying the room number, queues a
`ROOM_SCENE` request, and publishes room 44 as the stay room. The checked-in
scene-manager API narrowed that allocated pointer through `u32`, so the request
could not own its authentic payload on a 64-bit macOS host.

## Result

Patch 0110 promotes the create/re-request user payload to `void*` throughout
the target-PC scene-manager boundary. The original request path now preserves
the allocation's exact native address, including an address above
`UINT32_MAX`, and forwards it unchanged to `fopScnRq_Request`.

The focused test executes the source-order `dStage_roomInit(44)` body and
proves room status flag 2, request type 0, null source scene,
`fpcNm_ROOM_SCENE_e`, no fade process, zero fade-peek time, payload value 44,
draw publication, old/stay room values `-1/44`, and exact pointer identity. An
allocation failure queues no request. The tier deliberately stops before the
queued profile is looked up or executed.

The source patch changes three files by 45 insertions and 15 deletions. Most
insertions are a narrow test partition of unchanged `d_stage.cpp`; the semantic
host adaptation is the native-width scene user ABI and removal of its integer
round trip.

## Evidence

- Initial-room object SHA-256:
  `784516dbd8c55bb075d58a46c803e97c8ef4d40ae34a8173f8a90afa36be1a75`.
- Debug focused executable SHA-256:
  `d36381e4fd6659a9b5136f1523e3d367b40ce5f66969232e6e4b0232b7d974e8`.
- Patch SHA-256:
  `15357f30976d82276ba0de6e8e4d21ffbb9fde7bf3068da0473dcd4840c90712`.
- Patches 0001 through 0110 replay cleanly from pinned TWW commit
  `03d27aa14389e648f51df2bc055ee3d53a3b67f2`.
- The focused test and all 51 public tests pass in Debug, optimized Release,
  and strict ASan/UBSan.
- The two protected user-edited RecompCore files retain their expected hashes.

No queued profile, room archive, BlueWake app, or Simulator was launched.

## Next Boundary

Qualify the queued request through authentic `ROOM_SCENE` profile lookup,
allocation, and creation phases 0 and 1. Prove the room parameter reaches the
scene, the room process ID publishes, empty room memory admits creation, and
the exact `Room44` stage-resource request is submitted. Keep resource sync,
`room.dzr` decode, actors, maps, events, particles, another BlueWake process,
and another Simulator closed.
