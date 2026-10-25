# Route B Stage Remainder Census - 2026-09-02

## Question

After STAG, MULT, RCAM, ACTR, and RTBL, which present Outset `stage.dzs`
handlers remain, and what is the smallest ownership-aware route to completing
the aggregate loader without repeating one parser-shaped experiment ten times?

## Asset Evidence

The preserved `stage.dzs` is 0x30A0 bytes with SHA-256
`5f5a29bd46e5132b7446cff9085939cf2373eb471eaafadbfecb2748be3be113`.
Its ten unqualified chunks are exact contiguous payloads:

| Tag | Count | Offset | Bytes/entry | Representation | Runtime effect |
| --- | ---: | ---: | ---: | --- | --- |
| `SCLS` | 212 | `0x08E0` | 12 | byte-only flat entries | passive scene-change metadata |
| `EVNT` | 57 | `0x0388` | 24 | byte-only flat entries | passive event-name metadata |
| `EnvR` | 52 | `0x1528` | 8 | byte-only flat entries | passive palette-selection metadata |
| `Colo` | 10 | `0x16C8` | 12 | bytes plus BE float | passive color-selection metadata |
| `Pale` | 57 | `0x1740` | 44 | colors plus two BE floats | passive palette metadata |
| `Virt` | 37 | `0x210C` | 36 | four BE words plus colors | passive virtual-sky metadata |
| `RARO` | 1 | `0x2904` | 20 | three BE floats plus BE angles | passive camera-arrow metadata |
| `RPAT` | 4 | `0x2640` | 12 | BE fields plus point offset | linked path records |
| `RPPN` | 40 | `0x2670` | 16 | bytes plus three BE floats | linked path points |
| `SCOB` | 50 | `0x2998` | 36 | ACTR base plus scale | queues actor requests |

The spans agree exactly with source record sizes: SCLS 0x9F0, EVNT 0x558,
EnvR 0x1A0, Colo 0x78, Pale 0x9CC, Virt 0x534, RARO 0x14, RPAT 0x30,
RPPN 0x280, and SCOB 0x708. The source handlers are at
`ref/tww/src/d/d_stage.cpp:1809-1814`, `:1896-1934`, `:1976-1981`,
`:2007-2026`, and `:2051-2099`.

## Classification

All ten still cross the 32-bit counted-view boundary. SCLS, EVNT, and EnvR
may retain immutable serialized entry bytes but still need native counted
owners. Colo, Pale, Virt, and RARO also need explicit endian conversion into
native entries. These seven have no creation side effect and form one cohesive
passive-metadata tier.

RPPN and RPAT form one inseparable ownership graph. The four serialized path
records contain 32-bit point offsets; the original handler relocates them
against the RPPN base. They require native point and path arrays with stage
lifetime and should be qualified together.

SCOB repeats ACTR's native actor-record conversion and request queue but adds
scale. It is the only remaining process-bearing handler and must remain its own
vertical tier so request names, payloads, and profile fencing are observable.

## Decision

Complete the remainder in three bounded tiers:

1. Seven passive metadata views: RARO, Pale, Colo, Virt, SCLS, EVNT, EnvR.
2. The linked RPPN/RPAT point-path graph.
3. SCOB decode and authentic request enqueue, without profile execution.

Only after those tiers pass may the aggregate loader itself be qualified and
the loop advance to initial-room creation. No BlueWake app or Simulator was
launched for this census.
