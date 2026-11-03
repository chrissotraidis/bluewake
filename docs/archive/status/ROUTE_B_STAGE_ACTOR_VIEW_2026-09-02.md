# Route B Stage ACTR Host View - 2026-09-02

## Boundary

`ACTR` is the next present Outset `stage.dzs` chunk after `RCAM` in aggregate
loader order. It is generic stage actor data, not the room-owned `PLYR` record
needed by CAMERA phase 2. The preserved Outset stage DZS has four ACTR entries
and no PLYR chunk.

The retail handler aliases serialized records directly and submits one actor
request per admitted entry. On a 64-bit host, the serialized big-endian fields
need a native representation with stage lifetime; process requests must still
flow through the original standard-create queue.

## Result

Patch 0104 promotes one checked file-relative entry resolver shared by MULT,
RCAM, and ACTR. It rejects null inputs, negative counts, and 32-bit
offset-plus-size overflow. ACTR decoding copies the four 32-byte serialized
records into stage-owned native storage, converts parameters, positions,
angles, and set IDs, then enters unchanged `dStage_actorCreate` and
`fopAcM_CreateAppend` behavior.

The Outset tier admits exactly the four names observed in the preserved asset:

| Name | Process | Parameters | Position | Y angle | GBA |
| --- | --- | ---: | --- | ---: | ---: |
| `Ship` | `fpcNm_SHIP_e` | `0x0000ff00` | `(19200, 0, -2200)` | `0` | `0` |
| `sea` | `fpcNm_SEA_e` | `0x00010000` | `(0, 0, 0)` | `0` | `0` |
| `Md1` | `fpcNm_NPC_MD_e` | `0x00ff0300` | `(198800, 3115.562988, -196200)` | `-3094` | `60` |
| `Cb1` | `fpcNm_NPC_CB1_e` | `0xffffffff` | `(215510, 1997.290283, 196730)` | `21845` | `59` |

All four requests are owned by the authentic current process layer and retain
the retail defaults for room, argument, scale, parent ID, and set ID. The test
observes queued request state and does not execute any of the four unqualified
actor profiles. The name table is intentionally an Outset-only qualification,
not a claim that the complete retail object-name table is portable.

## Evidence

- Preserved stage RARC SHA-256:
  `9c7cd338ef94c9a1c4077e598c428a3ea8a0b5b7f29f7143b9c4ec8690e55a1f`.
- Isolated ACTR stage object SHA-256:
  `8ab7bd13e59d1dcceede3bfaa5126e77512b081fcd288808821089babbcc8e00`.
- Actor-append object SHA-256:
  `5685a51a31844a50e6a9d46775003b0fc4793145e29e01887c1cdc5c4204fdc6`.
- Save actor-query object SHA-256:
  `0fc20366221a9654b578a65e49b63f1a6217e09e14c5b53f9069d6ffcf59b30b`.
- Debug test SHA-256:
  `c61106264dca0054e9a710c489be45e1c3b10e378953e38f0caf252b816a862d`.
- Patch SHA-256:
  `707af2095139a71c74279a929c610763ea8a4da7a4e89db3c56ed467914eeff7`.
- Patches 0001 through 0104 replay cleanly from pinned TWW commit
  `03d27aa14389e648f51df2bc055ee3d53a3b67f2` and reproduce all four touched
  source blobs.
- Debug, optimized Release, and strict ASan/UBSan pass 46/46 public tests.
- Repository audit validates the dependency lock and reports only the same 20
  tracked `local-research` evidence files awaiting user disposition.

The save predicate's room-status and unrelated aggregate lifecycle paths are
fail-closed or inert in this test. Every reached Outset ACTR record returns at
the retail `setID == 0xffff || room == -1` branch. No BlueWake app or Simulator
process was launched for this source-native tier.

## Next Boundary

Continue aggregate loader order at present `RTBL`. It has nested serialized
offsets: a counted entry-pointer table, per-entry room arrays, and metadata
required before initial-room creation and later consumed by room streaming.
Promote an owned native RTBL view and prove the authentic Outset room metadata before entering
`dStage_roomInit` or creating a room scene. CAMERA phase 2 remains pending until
the later room loader publishes a real player.
