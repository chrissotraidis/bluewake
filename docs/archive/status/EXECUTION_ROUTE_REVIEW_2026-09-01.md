# BlueWake macOS Execution-Route Review

**Date:** 2026-09-01  
**Decision:** Decompose the preserved exact AArch64 LLVM route once; stop
translated-C edge optimization.

## Trigger

BlueWake compiles, signs, launches, boots retail code, and reaches the accepted
Outset route. The promoted run still executes about 235 seconds of guest time
in 343.88 seconds, roughly 68% real speed before pacing headroom. The
post-hybrid profile assigns generated dispatch about 56% of active-main
samples, but no remaining host subsystem is individually large enough to close
the gap.

The final bounded translated-C chassis hypothesis is now falsified. A
generated-side PI/intercept filter preserved the same route as the pure edge
callback and avoided ordinary callback entry, yet changed user CPU from 23.02
to 23.18 seconds. It also retained the late delivery-hash mismatch. Host
callback overhead was not the missing architecture-scale gain.

This satisfies the PRD route-review trigger: physical Apple performance has no
measured headroom on the current execution shape.

## Options

| Option | Decision | Reason |
|---|---|---|
| Continue translated-C edge/callback tuning | Reject | Three materially different edge mechanisms failed correctness, payoff, or both. The shape is closed. |
| Decouple compile partitions from runtime barriers | Retain as build debt | It can reduce the 4h16m rebuild cost, but the source-correct hybrid already compiles generated units at `-O2`; this is not a demonstrated runtime-speed mechanism. |
| Replace one host subsystem natively | Defer | Deadline calculation and donor DSP are only 6.4% and 4.3% of sampled main-thread time. No stable narrow subsystem currently offers the required architecture-scale gain. |
| Replace authentic DSP with Zelda DSP HLE | Not selected | It changes bit-exact DSP acceptance and requires an explicit product-policy decision; it is not a routine optimization experiment. |
| Revisit exact AArch64 LLVM | Select one decomposition tier | Exact v27 exists and reaches canonical title timing, but its 44.20s/42.51s 700 tier was rejected against 27.8s C without profiling. That result predates the trace-off host, hybrid-O2 baseline, and current cycle contracts. Its slowdown owner is still unknown. |
| Pivot to another donor or whole execution route | Reserve | Consider only if LLVM decomposition finds no bounded architecture-scale owner. |

## Bounded Next Action

1. Preserve and verify exact LLVM v27 artifact
   `782edf96b1614e95536cd0e5d8dde06f27f8b2a6fb0b5cebd4d1c59c80c71a30`.
2. Reconstruct its source-compatible host from repository history without
   modifying the current accepted host or protected user-edited files.
3. Run one trace-off, 700-retrace native sample/profile. Attribute time among
   structural admission/cold fallback, native region execution, dispatcher,
   host services, DSP, and GX.
4. Continue LLVM work only if one bounded owner plausibly supports at least a
   1.5x end-to-end gain while preserving the exact route. Otherwise reject the
   backend and reopen the route review at the donor/whole-route level.

No new translated-C edge callback, boundary trace, guard window, address list,
runtime partition, coalescer, or sub-5% cleanup is admissible during this tier.

## Decomposition Result

The preserved artifact and source-compatible historical host pass the tier.
The 700 route reproduces all retained historical summaries and an unchanged
card in 31 sampled wall seconds. Of 22,745 samples, 40.7% land in old monolithic
host `main`, 23.5% in the old DSP schedule/interpreter branch, 9.8% in old REL
alias handling, and 9.8% in `selected_dispatch` plus the hottest generated
budget loop.

This overturns the assumption that v27's old aggregate slowdown was primarily
LLVM execution. The next decision tier is a strict current-host diagnostic
adapter for the append-only ABI-4 CPUState prefix. A passing material speed
result authorizes regeneration against current ABI/cycle emission; it does not
promote the legacy artifact itself.

## Current-Host Qualification Result

The strict diagnostic prefix adapter passed its smoke and 700-retrace runs.
The 700 tier reached title-ready at retrace 333, completed 4,295 ARAM transfers
and 36,429 DSP DMAs, and preserved the copied card. Current host work reduced
the historical sampled v27 wall time from about 31 seconds to 25.83 seconds,
but the module still consumed 25.00 user seconds. It stopped after 105,544,680
blocks at `0x80245664`, differing from the historical route, and ABI 4 cannot
emit the current generated cycle-observation suffix.

That result does not support the required 1.5x end-to-end opportunity. A
current LLVM regeneration would add current cycle work without a measured
mechanism likely to close the remaining gap. Reject regeneration and remove
the diagnostic adapter. Reopen this review at the reserved donor/whole-route
level under `BW-P4-0068`; first produce a bounded source/execution census of
the pinned no-JIT candidates and Route B rather than beginning another backend
implementation by momentum.
