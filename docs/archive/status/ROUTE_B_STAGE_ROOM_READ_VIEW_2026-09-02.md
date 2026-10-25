# Route B Stage RTBL Host View - 2026-09-02

## Boundary

`RTBL` is the next present Outset `stage.dzs` chunk after `ACTR` in aggregate
loader order. Its serialized form is a 50-element file-relative offset table;
each offset names an eight-byte record containing a second file-relative
offset to room bytes. Neither offset graph can be exposed as the retail native
pointer representation on a 64-bit host.

RTBL must exist before initial-room creation, but source tracing confirms that
it does not choose the initial room. The start-stage state owns that choice;
RTBL metadata is consumed later when room streaming follows background-group
room IDs.

## Result

Patch 0105 promotes the checked file-relative resolver to support arbitrary
entry types and adds an exact eight-byte serialized RTBL record. The handler
resolves the pointer table, copies all records into stage-owned native storage,
builds the native record-pointer array, resolves every nested room-byte array,
and publishes the resulting `roomRead_class` from the existing stage owner.

The preserved Outset table has 50 entries. Entry zero contains room byte
`0xC0`; entries 1 through 49 contain `0xC0` and `0x80 | index`. Every room has
the background bit, all reverb values are zero, and time passage is disabled
only for entries 1, 4, 11, 13, 23, 41, 44, and 45. The public test reproduces
that complete payload and checks native ownership, accessors, fallback reverb,
negative counts, and 32-bit offset-plus-size overflow.

## Evidence

- Preserved stage RARC SHA-256:
  `9c7cd338ef94c9a1c4077e598c428a3ea8a0b5b7f29f7143b9c4ec8690e55a1f`.
- Isolated RTBL stage object SHA-256:
  `268fd5e21f1974b916ee6b3ec618cb367394fe6ad10609ce4e68f61149f7ff00`.
- Debug test SHA-256:
  `eb1cfa719e980c7e3f4baaef20f8aa95e4f05c44038cc41261c88df93d403450`.
- Patch SHA-256:
  `f004e98ecda0d54aeaa8d0f45f7b8e6d8b4a03017f413ef8779aee5545203ef1`.
- Patches 0001 through 0105 replay cleanly from pinned TWW commit
  `03d27aa14389e648f51df2bc055ee3d53a3b67f2` and reproduce all touched
  source blobs.
- Debug, optimized Release, and strict ASan/UBSan pass 47/47 public tests.
- Repository audit validates the dependency lock and reports only the same 20
  tracked `local-research` evidence files awaiting user disposition.

No actor profile, room scene, room loader, BlueWake app, or Simulator was
launched for this source-native tier.

## Next Boundary

Perform one census of all ten remaining present aggregate-loader handlers and
their exact Outset payloads. Classify them by representation and runtime owner,
then select the smallest cohesive remainder tier. This census is required by
the anti-stall rule before another per-handler implementation experiment.
