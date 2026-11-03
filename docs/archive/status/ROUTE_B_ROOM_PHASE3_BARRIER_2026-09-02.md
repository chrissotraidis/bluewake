# Route B Room Phase 3 Barrier - 2026-09-02

## Boundary

ROOM_SCENE phase 3 waits for the optional demo archive and room-particle
command, creates the room particle scene from the completed payload, deletes
the command, and then enters `objectSetCheck`. That final call opens the much
larger room-reload actor graph, so it remains an explicit fence.

## Result

Patch 0117 promotes phase 3 only through the resource barrier. Narrow
target-PC adapters bind object-resource sync and the unpromoted particle
command/controller ownership. The unchanged phase result and error-restart
decisions surround those adapters; `objectSetCheck` is represented by a hard
observation fence.

The public contract covers demo-resource error, busy, and ready states;
particle-command busy and ready states; exact payload-address publication;
single command deletion; and the command-absent path. No room-reload function
or requested profile executes.

## Evidence

- Patch SHA-256:
  `eb96397f1b9013c17c2a59edacb9b493ff6c93fd5ab6baddf15cd2bdf81a29de`.
- Debug phase-3 object SHA-256:
  `dc76b4e2741e8a5890cc8c4291f954293fc08e53df703a0849a62904358b1c90`.
- Debug focused executable SHA-256:
  `d74ce69725312b088ccb4f8c4bb4467e856abea214ab3a7b1974651319eb0c84`.
- Release focused executable SHA-256:
  `c6b51c65f15c4eca1d6b359d988962baa2952f3abd284168ea0854666ac41af2`.
- ASan/UBSan focused executable SHA-256:
  `6a7eae26075218e7e364c8207c198f1bad54839b581bf3f91935d08e76535566`.
- Patches 0001 through 0117 replay cleanly from pinned TWW commit
  `03d27aa14389e648f51df2bc055ee3d53a3b67f2`.
- All 56 registered tests pass in Debug, optimized Release, and strict
  ASan/UBSan.

The patch changes 47 lines across the 512-line source boundary, a 9.18%
partition-and-adaptation density. No `objectSetCheck`, room reload, requested
profile, phase 4, BlueWake process, or Simulator was admitted.

## Next Boundary

Census the exact hidden-to-visible reload graph: the leading BG request and
all 172 ACTR, five TGDR, and one SCOB records. Classify profile mappings,
parameters, layers, native representation, and cohesive ownership tiers before
executing reload or any requested profile.
