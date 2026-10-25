# Route B Room44 Census - 2026-09-02

## Question

What does ROOM_SCENE phase 2 actually consume from the initial `Room44`
archive, and which ownership tiers reach that boundary without accidentally
admitting the much larger room-reload actor graph?

## Route Identity

The current source route starts in `sea_T`, so its exact archive is
`/res/Stage/sea_T/Room44.arc`. The similarly named gameplay archive at
`/res/Stage/sea/Room44.arc` was inspected only as a route-confusion control.
The census used AuroraDisc plus the original JKR decompressor and memory
archive. It did not launch BlueWake or Simulator.

The PLYR-capable private diagnostic executable has SHA-256
`09819bd5333274733ef2dd1eb30cd771b17a8a2fec3e85e0072f8e007cc6672c`.
Its public-safe source has SHA-256
`86edd65d30e1a186d56ae6af2718318c2d42c38fc7cd8fa2aafffb7531e1f244`.
The optional export path writes the exact title DZR for private aggregate
qualification without admitting game data to Git.

## Exact Initial Archive

The `sea_T` archive is 714,816 bytes and contains 26 entries. Its
`room.dzr` is 10,880 bytes with 11 nodes:

The table below is ordered by payload offset for representation analysis, not
by raw node-header order. The aggregate contract separately checks all raw
headers before the loader searches them in its own function-table order.

| Tag | Count | Offset | Span | Record bytes | Phase owner |
| --- | ---: | ---: | ---: | ---: | --- |
| `FILI` | 1 | `0x0088` | `0x0008` | 8 | room loader |
| `2DMA` | 1 | `0x0090` | `0x0038` | 56 | room loader |
| `TGDR` | 5 | `0x00C8` | `0x00B4` | 36 | later room reload |
| `LBNK` | 12 | `0x017C` | `0x000C` | 1 | room loader |
| `SHIP` | 1 | `0x0188` | `0x0010` | 16 | room loader |
| `PLYR` | 1 | `0x0198` | `0x0020` | 32 | room loader |
| `RPAT` | 40 | `0x01B8` | `0x01E0` | 12 | room loader |
| `RPPN` | 271 | `0x0398` | `0x10F0` | 16 | room loader |
| `SOND` | 2 | `0x1488` | `0x0038` | 28 | room loader |
| `ACTR` | 172 | `0x14C0` | `0x1580` | 32 | later room reload |
| `SCOB` | 1 | `0x2A40` | `0x0040` | 36 plus trailing alignment | later room reload |

The room-loader table has 22 handlers, but only eight are present here:
`FILI`, `2DMA`, `LBNK`, `SHIP`, `PLYR`, `RPAT`, `RPPN`, and `SOND`.
`TGDR`, `ACTR`, and `SCOB` are consumed later by the room reloader and
`objectSetCheck`; their 178 requests are therefore outside phase 2.

The one exact PLYR record is `Link`, parameters `0x00FF002C`, position
`(-192700.796875, 550, 318904)`, angles `(0, 30765, 0)`, and set ID `0xFFFF`.
It selects point 0, room 44, and no saved ship ID on the current title route.

## Gameplay Control

The `sea` archive is 729,408 bytes. Its 22,560-byte DZR has 31 nodes,
including 24 `PLYR`, 187 base `ACTR`, 59 base `SCOB`, and extensive layered
`ACT*`/`SCO*` data. This confirms that the gameplay archive is materially
broader than the current title route and must not be substituted into initial
phase-2 acceptance.

## Ownership Classification

Six handlers are representation-only at this boundary. `FILI`, `2DMA`, and
`SOND` need endian-correct native records; `LBNK` needs a native counted
wrapper; and `RPPN`/`RPAT` need one linked native point/path graph. Their
storage belongs to `dStage_roomDt_c`, not the stage owner used by patches
0101-0109.

`SHIP` also needs native records, but its unchanged handler reads and may
clear global ship/restart state and can reposition an existing ship actor.
`PLYR` is the sole process-bearing room-loader handler: it chooses one spawn,
updates start/ship state, queues the player, and creates title or meter/AGB
requests based on the owning scene. These effects require separate vertical
qualification.

## Decision

1. Add room-owned native views for `FILI`, `2DMA`, `LBNK`, `SOND`, and the
   linked `RPPN`/`RPAT` graph; qualify them together against the exact
   `sea_T` counts.
2. Qualify native `SHIP` decode and its exact start-state behavior.
3. Qualify native `PLYR` decode, spawn selection, and authentic request queue
   while keeping requested profiles fenced.
4. Compose all eight through the unchanged room loader, then enter the
   surrounding phase-2 sync/zone/resource and particle contract.

Room reload, its actor/object requests, profile execution, BlueWake launch,
and Simulator remain closed until those owners pass.
