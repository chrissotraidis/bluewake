# Route B Stage MULT Host View - 2026-09-02

## Census

The preserved `sea_T/Stage.arc` RARC (SHA-256
`9c7cd338ef94c9a1c4077e598c428a3ea8a0b5b7f29f7143b9c4ec8690e55a1f`)
contains a `stage.dzs` with 15 chunks, not the 13 recorded by the earlier
summary. Fourteen match aggregate `stageLoader` handlers: `MULT`, `ACTR`,
`RTBL`, `RARO`, `Pale`, `Colo`, `Virt`, `SCLS`, `SCOB`, `EVNT`, `EnvR`,
`RPAT`, `RPPN`, and `RCAM`. `STAG` is consumed by the already-qualified
stage-info loader. No layer-specific chunk is present.

## Portability Finding

GameCube stage loading mutates each 12-byte node's 32-bit offset into a
pointer, then aliases its `{entryNum, offset}` fields as a counted native
view. That representation is algebraically invalid on 64-bit macOS: the
serialized count is big-endian, a native pointer is eight bytes, and reading
it from node offset 8 consumes four bytes from the following node. This is an
upstream DZS ABI issue shared by most stage handlers, not a `MULT` logic bug.

## Result

Patch 0101 proves the first bounded host-view pattern. `dStage_stageDt_c` owns
a native `dStage_Multi_c` view for the stage lifetime. The target-PC
`dStage_multInfoInit` keeps the original count and publication behavior but
resolves the entry pointer from the immutable file base and serialized node
offset. `dStage_Mult_info` uses exact big-endian wrappers for its two floats
and signed angle while preserving its 12-byte file layout.

The public test supplies two serialized entries, invokes the retained handler,
and verifies view identity, count, entry address, both signed/unsigned fields,
and positive/negative float decoding. No callback or copied payload stands in
for the stage owner. All 43 tests pass in Debug, optimized Release, and strict
ASan/UBSan, and patch replay through 0101 passes.

Debug SHA-256 values are:

- isolated `d_stage.cpp.o`: `929859dd5613b275f2c75ea727d15f842f669a537a0ecea4a84f792a2711084a`
- test executable: `ba1f962dd4b700ed2414fcd22267c163cd99759a3369f564af194f7a7e6d34e8`

Patch 0101 changes 31 of 3,730 reached header/source lines (0.83%).

## Next Boundary

Do not repeat one handler at a time without a shared ownership pattern. In
aggregate loader order, the next present Outset chunk is `RCAM`, whose handler
needs another counted host view and enters camera creation. Measure that exact
closure once. If the same view shape repeats, promote a common stage-view
builder before adding further handlers. Keep actor creation, room reads, map,
events, later environment lifecycle, and concurrent processes closed.
