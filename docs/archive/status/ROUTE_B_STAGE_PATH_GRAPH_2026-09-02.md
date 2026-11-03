# Route B Stage RPPN/RPAT Graph - 2026-09-02

## Boundary

The preserved Outset RPPN and RPAT chunks are one ownership graph. RPPN holds
40 sixteen-byte point records with byte arguments and big-endian coordinates.
RPAT holds four twelve-byte path records whose 32-bit point offsets are relative
to the RPPN entry base. Retail mutates those offsets into pointers in place;
that representation is invalid on a 64-bit host.

## Result

Patch 0107 adds exact serialized point and path records and stage-owned native
arrays for both. RPPN converts all coordinates and preserves all point argument
bytes. RPAT converts path fields, rejects misaligned and out-of-range point
ranges, and publishes native pointers into the owned point array.

The authentic Outset topology is exact: path lengths `6, 22, 4, 8`, next IDs
`1, 2, 3, 0xFFFF`, and point offsets `0x000, 0x060, 0x1C0, 0x200`. The four
ranges partition all 40 points. The public test exercises that topology and
every generated point field, native owner identity, and endpoint access.

## Evidence

- Preserved DZS SHA-256:
  `5f5a29bd46e5132b7446cff9085939cf2373eb471eaafadbfecb2748be3be113`.
- Path graph object SHA-256:
  `cbb502726bcd902f1ceec95dcfaa435ee5a75d39c9959bdfde6118200c9cfe05`.
- Debug test SHA-256:
  `9f26ea629adb8fa857fed35472a5e567672c13ad2b348823cc991d8e9ee9bdf6`.
- Patch SHA-256:
  `2ffcdc3b9f6bcdb879843d7321f48fde14d8fe423eb71c0d5edca91c1e951c10`.
- Patches 0001 through 0107 replay cleanly from pinned TWW commit
  `03d27aa14389e648f51df2bc055ee3d53a3b67f2` and reproduce every touched
  source blob.
- Debug, optimized Release, and strict ASan/UBSan pass 49/49 public tests.

No path consumer, SCOB request, BlueWake app, or Simulator was launched for
this tier.

## Next Boundary

Qualify the final process-bearing stage handler, SCOB. Decode all 50 scaled
actor records into native stage-owned storage, identify the exact retail names
and request payloads, and queue them through the authentic process stack while
keeping every actor profile unexecuted. Aggregate-loader composition remains a
separate successor test.
