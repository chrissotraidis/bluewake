# PRD — Ground collision walk, revisited

Status: Planned; M0 (local census and speed test) first, no native code before it  
Source: the [#503](https://github.com/chrissotraidis/bluewake/pull/503) stack (`b538550`)  
Follows: [#436](https://github.com/chrissotraidis/bluewake/pull/436) (closed no-go) with the method of [#541](https://github.com/chrissotraidis/bluewake/pull/541)  
Scope: `cBgW::GroundCrossGrpRp` (0x802491F4) and the tree walk under it, on the Steam Deck. Excluded: other collision checks, and changes to what the game computes.

## Problem

Every actor on the ground asks the collision code for the floor height under it each frame. `GroundCrossGrpRp` walks the stage's collision groups. For each one it:
1. filters the group (`ChkGrpThrough`);
2. tests the group's height bounds;
3. tests the group's polygon tree (`GroundCrossRp`);
4. recurses into the group's children.

#436 replaced this function with a native and found it **correct but not faster**:

| #436 measure | Result |
|---|---|
| Calls the native took | 123,609 of 440,495 (28%) |
| Declines that needed the tree walk | 315,167 of 316,886 (99.5%) |
| Accepted early exit, translated | 179.3 ns/call |
| Accepted early exit, native | 226.1 ns/call |
| Game A/B | +0.4–0.5%, inside noise |

## What #436 missed

| Finding | Evidence | Consequence |
|---|---|---|
| It covered only the cheap early exits | 99.5% of declines needed the tree walk | Each accepted call saved a few dozen guest cycles at most |
| Declined calls paid twice | Every call copied the CPU state (≈ 3.5 KB), ran ~10 memory checks, 4 direct-call lookups and the `ChkGrpThrough` native, then 72% ran the whole translation | The added cost on declines was never timed; it likely cancelled the gain |
| The accepted path was slower | 226 vs 179 ns; a 3.5 KB copy to replace ≈ 50 guest cycles | Validate first, then write the real state once (as the skinning native does) |
| No hand-over | Declines threw away the work already done | #541 M2's hand-over lets the native stop where it can and the translation carry on |

## Outcomes

- **First, without native code:** a local measurement of what the walk costs translated, and whether a native walk can beat it. A "no" ends this plan cheaply.
- Only if M0 shows a gain: a native for the walk that hands over to the translation at the calls it does not cover, exact in every register and byte.

## Success and constraints

| Metric | Target | Validation |
|---|---|---|
| M0 local speed | A prototype native walk ≥ 1.2× faster than the translated walk on the census's typical trees, **and** the declined path within 2% of the translation alone | `native_bg_test MODULE --bench` on the workstation, matched `-march` |
| Ceiling | Translated cost of the walk on Outset, in guest cycles per frame | Census build, one Game Mode minute |
| Game FPS | Ahead of the control in every interleaved launch (≥ 6) | Game Mode A/B on the Deck |
| Guest behaviour | Identical checkpoints; the differential test byte-exact | `native_bg_test`; render_bench |

Gains under 1% are kept when they are consistent, exact and tested. Private modules, states and logs stay off GitHub.

## Decisions

- **Measure before writing the native:** the M0 census and speed test decide whether there is anything to gain.
- **Hand over instead of declining:** whatever the native does not cover, the translation continues from that exact point.
- **No copy of the CPU state:** check as it walks, then commit once.
- **Opt-in until qualified (2026-11-03):** the native walk is off unless `BLUEWAKE_NATIVE_GROUND_WALK=1`, until the Deck A/B and checkpoints qualify it.
