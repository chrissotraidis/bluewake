# Route B Room44 Reload Census - 2026-09-02

## Boundary

Room phase 3 now reaches the call to `objectSetCheck`, whose visible-room path
queues BG and reloads ACTR/TGDR/SCOB records. The census classifies that graph
without invoking the reload or executing any requested profile.

## Result

The exact 10,880-byte `sea_T/Room44` DZR contains 172 ACTR records, five TGDR
records, and one SCOB record. Its reload table has no TGOB, TRES, TGSC, DOOR,
or layered ACT0-ACTb/SCO0-SCOb/TRE0-TREb nodes. Therefore `layerLoader` adds
zero requests for room layer 2. Together with the leading BG request, the
visible-room transition deterministically submits 179 requests.

The 28 resource names map to 15 retail profile families:

| Reload source | Profile | Records |
| --- | --- | ---: |
| ACTR | GRASS | 119 |
| ACTR | KAMOME | 10 |
| ACTR | Obj_Wood | 9 |
| ACTR | Obj_Lpalm | 8 |
| ACTR | TSUBO | 8 |
| ACTR | Lwood | 6 |
| ACTR | KB | 3 |
| ACTR | KANBAN | 2 |
| ACTR | KN | 2 |
| ACTR | BRIDGE | 2 |
| ACTR | Stone2 | 1 |
| ACTR | OBJ_TORIPOST | 1 |
| ACTR | OBJ_IKADA | 1 |
| TGDR | KNOB00 | 5 |
| SCOB | KYTAG01 | 1 |

All ACTR and SCOB records use set ID `0xffff`; the ACTR save predicate thus
admits all 172 records. All five TGDR records have persistent set IDs, but the
retail scaled-object path does not apply the ACTR save predicate. Parameters
vary within nine of the 28 names and are copied per record rather than
normalized by profile.

ACTR needs a room-owned decoded 0x20-byte view plus save-filtered append
requests. TGDR and SCOB share a room-owned decoded 0x24-byte scaled view and
unconditional append requests; TGDR additionally publishes that view through
`setDrTg`. The leading BG request and the aggregate reloader form a later
composition boundary. Requested profiles remain outside all three tiers.

## Evidence

- Exact private DZR SHA-256:
  `0c89307c89183f65fd8ea72595bd15855ca24f2441b91b1b4eb06a353a33ace9`.
- Census source SHA-256:
  `0b9df43423850c007b77a5b93731ca8c3c1046dfc5ed6aa9733345bd8ed32a5a`.
- Release census executable SHA-256:
  `64a886e567655f55aa4024185bd66fcd07ada851d83746808065d5cf43f8f7df`.
- Complete private output SHA-256:
  `ac27ae2ce03bd680ac3672520e2bb2ed4d3aa095475f954ee192541f6a0c862c`.
- The private census ran through Aurora DVD, original decompression, and
  `JKRMemArchive` against the user-owned GZLE01 disc.
- The complete private output remains untracked because it reports title data;
  this report retains only structural counts, mappings, and hashes.

No BlueWake process or Simulator was launched.

## Next Boundary

Implement the room-owned ACTR view and exact request-construction behavior as
one bounded tier. Prove 0x20 decoding, all 13 profile mappings, per-record
parameters/transforms/set IDs, room 44 propagation, save filtering, allocation
failure, unknown-name cleanup, and reset lifetime. Keep TGDR/SCOB, aggregate
reload, requested-profile execution, phase 4, BlueWake, and Simulator closed.
