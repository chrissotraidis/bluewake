# Route B Room Reloader - 2026-09-02

## Boundary

The Room44 ACTR, TGDR, and SCOB mechanisms had passed separately, but their
interaction through the original `dStage_dt_c_roomReLoader` table was still
unqualified. The title DZR contains 178 matching records and no active-layer
reload tags.

## Result

Patch 0120 restores the original seven-slot reload table and the three-slot
active-layer table around the qualified native handlers. The exact
10,880-byte title payload submits 172 ACTR, five TGDR, and one SCOB request.
All three room-owned buffers coexist, the five-entry door table remains
published after SCOB decoding, and the selected layer contributes no request
because Room44 has no matching `ACT2`, `SCO2`, or `TRE2` node.

The aggregate owner supplies the single reset implementation needed when the
ACTR and scaled handlers link together. Reset releases all three buffers and
clears their published views. Requested profiles do not execute, and the
leading BG request remains outside this boundary.

## Evidence

- Exact Room44 DZR SHA-256:
  `0c89307c89183f65fd8ea72595bd15855ca24f2441b91b1b4eb06a353a33ace9`.
- Patch SHA-256:
  `fda053eb06a5b641424bcdaae4515f00c54a172f9a41f35c86a49177e648d044`.
- Public contract SHA-256:
  `b7cab95c09bb1d6f0ea5259a09c45a55aeb2eb9af54dc3e88faf882a9d0d34e9`.
- Debug focused executable SHA-256:
  `77ceef0d0f7074c3e4c5e5544523568c5c3cc9aea36416e19c825da16b210258`.
- Release focused executable SHA-256:
  `c979da9de6525b41b1a7aba76ca2e74648f1e27c3f56117f9f057c8642423b40`.
- ASan/UBSan focused executable SHA-256:
  `2bd99e3de271dcbb6e2c4dfec1c47805d2b8183a86720edb0fd7b98c8c7d3de1`.
- Patches 0001 through 0120 replay cleanly from pinned TWW commit
  `03d27aa14389e648f51df2bc055ee3d53a3b67f2`.
- All 59 registered tests pass in Debug, optimized Release, and strict
  ASan/UBSan. The focused contract also passes separately against the exact
  private DZR in all three modes.

No BlueWake process or Simulator was launched.

## Next Boundary

Promote `objectSetCheck` as one visible-room state transition. Prove that an
unhidden, not-yet-reloaded room queues BG before the exact 178-record reload
and marks itself reloaded; prove that hiding a reloaded room deletes its layer
children, clears its room switch state, and makes a later unhide reload once.
Keep requested-profile execution, room phase 4, BlueWake, and Simulator
closed.
