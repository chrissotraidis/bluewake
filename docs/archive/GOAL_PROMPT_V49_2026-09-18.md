# BlueWake goal loop - v49 (2026-09-18)

**User-started. Finish and test the app.** The terminal condition is unchanged:

> From a double-click of BlueWake.app with no exported variables, a human
> reaches controllable Outset gameplay with correct video and audio, drives it
> with the keyboard without a crash, and time-to-playable is under five
> minutes, with the five governing product numbers holding on this host.

**This document supersedes the v25-v48 continuity pages** for one workstream
only: play-scene throughput. `PLAN_2026-09-18.md` and the "Where the goal
stands" page in `status/CURRENT.md` remain the record of that stretch and are
not replaced.

## Where this loop enters

The product half is met. The retail DOL and all 415 RELs boot into
controllable Outset gameplay on the macOS product path, from the signed app,
with real keys, real video, real audio, the game's own save and a normal stop.
The launch runs at authentic speed, paced by the audio queue.

What is not met is throughput. The play scene runs at about 51 percent of
authentic speed headless and 41 percent rendered, and authentic speed needs
1.81x and 2.47x respectively. The certified split:

| | instructions per play retrace |
| --- | --- |
| headless | 302.3 M |
| rendered | 403.3 M |
| renderer share | +101.0 M |

Play-window IPC is 4.01, at the machine's peak, so the port is not stalled,
not memory-bound and not oversubscribed. Only fewer host instructions per
guest cycle buys speed.

## The reframe this loop owns

The previous stretch measured eleven candidates and every one came back at or
under one percent: the access-path mirror clause (0.6), the GX sink payload
copy (0.07), the cycle-observation bookkeeping (under 1), `-O3` on the DOL
chunks (worse), stack hardening (byte-identical), the per-instruction pc store
(not removable, it derailed the route), the duplicated guard arms (2.0, landed
as patch 0017), the branch-density reading (a counting artefact), the
observation reconcile, the per-thread attribution (the collapsed call graph
does not partition) and the depth-difference method (retired).

The conclusion those eleven refutations support is not that the emitter is
near optimal. It is that **piecewise trimming cannot find this cost, because
the cost is a per-instruction constant rather than a set of removable
clauses.** At 302.3 M host instructions for 8.1 M guest cycles, the host
retires about 37 instructions per guest cycle against roughly 20 for authentic
speed. No clause in a 37-instruction path is worth 45 percent.

So this loop stops trimming and answers two questions with numbers:
how much would a leaner translation be worth at all, and which of the two
axes it can be reached on is cheaper to build.

## The work, in order

**1. The microbenchmark, which has been proposed three times and never built.**
Link one generated chunk object against the runtime with a synthetic
`CPUState`, call a hot body in a loop, and time it. The ledger has asked for
this instrument in three consecutive entries because it is the only one that
reports cost per guest instruction in *time*; every planning number in the
stretch is a code-size proxy, and code-size proxies have measured wrong five
times here. It is minutes of work and needs no composite rebuild. It also
settles a fork the ledgers never closed: how much of the 37 is the emitted
body and how much is helper work.

**2. The roofline, which is the decisive experiment.** Hand-optimise the same
body the way a lean emitter would: keep the guest registers it defines in host
locals across the body, hoist the cycle envelope to block granularity, push
the memory slow path out of line, and stop re-materialising state per
instruction. Measure it in the same harness. Whatever ratio comes back is the
ceiling for that function's shape, and it is the number that decides whether
1.81x is fundable or the target must be renegotiated. The candidate is
`func_802416E0` (4.4 percent of the rendered window, pure guest code) with a
body from `chunk_0144` as the second sample.

**3. Guest instructions per retrace.** The denominator of every efficiency
claim in this project, and nobody has it. The recorded estimates disagree by a
factor of two (61, 81 and 127 host instructions per guest instruction appear
in the ledger). A per-label counter on one digest-gated run closes it.

**4. Take the GX FIFO path off the main thread.** Independent of any emitter
work and the only large item on the dynamic census that is not a long tail:
about 79 M of the renderer's 101 M instructions are GX translation and draw
construction, synchronous on the main thread, while Aurora's render worker
sits idle in `BoundedQueue::pop_for` for 94 percent of samples. It is host
code, so the rebuild is about twelve seconds and the rendered measurement is
about five minutes; this is the fastest iteration in the project. The question
to answer first is what pins translation to the main thread. If it is the
observer reading guest state, the fix is a snapshot and a handoff rather than
a lock.

**5. Test the register-file hypothesis in its cheap form.** Mirror
`downcount`, the alias flag, the reservation flag and the write-journal
pointer in function locals, and write them back only across calls that can
change them. This is the ledger's own sketch and it measures the reload
traffic the leading theory predicts, without paying for the full redesign.

**6. Ask whether the access path's guards are live on this route.** Replace
`get_ram_ptr`'s fast path with an unchecked MEM1 pointer and check the digest
on the certified route. If it holds, the alias, mirror and MEM2 branches can
move to block granularity - one check per block instead of per access - which
is a constant reduction rather than a trim.

**7. An outside reference point: Dolphin's JIT on this machine and this disc.**
It is the same workload translated by an implementation that does register
caching and block-level cycle accounting. It bounds what this hardware can do
with this game, which is exactly what the 1.81x target assumes. It is not a
gate and cannot satisfy the digest; it is a sanity check on the target.

## Hypotheses this loop owns

- **H-FLAT-FLOOR** (leading): the per-instruction cost is a floor of the
  emitted shape, not a removable set of clauses. Prediction: the
  microbenchmark's time-derived cost per guest instruction is within about 25
  percent of the line-count-derived one, and no single envelope piece exceeds
  5 percent of a frame in time.
- **H-REGISTER-FILE**: the largest single term is the memory-based register
  file, where every guest operand is a load or store through `ctx->gpr[]`
  because no callee is known not to alias `ctx`. Prediction: the roofline
  body that keeps guest registers in host locals is at least 2x cheaper than
  the emitted form; the cheap-form mirroring of item 5 is worth 2-5 percent on
  its own.
- **H-ROOFLINE-SHORT** (the falsifier): a hand optimum is less than 1.4x
  cheaper, in which case 1.81x is not reachable by translation quality and the
  honest move is to renegotiate the target with the user rather than fund a
  redesign.
- **H-GX-CRITICAL-PATH**: the GX path's ~79 M instructions can leave the main
  thread, because the consumer is a bounded queue that already has a worker.
  Prediction: a rendered run with the flush path handed to the worker keeps
  the digest green and takes at least 10 percent off the rendered play
  window; if it instead moves the bottleneck to the queue mutex, the
  hypothesis is dead and the census share is real construction work.
- **H-ACCESS-GUARD-DEAD**: the alias, mirror and MEM2 branches of
  `get_ram_ptr` never fire on the certified route. Prediction: the unchecked
  variant holds the digest green over the full route.
- **H-GUEST-COUNT**: guest instructions per play retrace is about 3.2 M at a
  guest CPI near 2.5, so the host runs near 90 host instructions per guest
  instruction rather than the 37-per-guest-cycle figure. Either way the
  denominator becomes measured instead of inferred.

## The A/B protocol, since it is easy to skip

- `scripts/bench_instructions.sh` is the primary instrument: two headless
  tiers, differenced, giving instructions per play retrace, IPC and delta
  against a reference, at 0.02 percent resolution in about three minutes.
- `scripts/bench_rendered.sh` is the rendered differential. Its guard refuses
  to report unless both runs stopped normally inside the certified window.
- `scripts/recomp_chunks.sh` screens a candidate on the hot ten chunks in
  about twenty minutes against a ninety-minute DOL rebuild or a 2h15m full
  rebuild. It relinks directly rather than through make, because make
  re-checks every object against its headers and silently turns a screen into
  a full rebuild.
- A change counts only when the route digest reads UNCHANGED and the run
  stopped normally at the certified pc with the certified turn count. Read the
  run before reading its number: eight measurements in the previous stretch
  needed correcting and every one was caught by that check.
- Do not run a timing measurement while a build is running.

## What not to do

- Do not plan against W4's attribution. Its shares are code size, and the one
  of them that was tested (the 18 percent access path) was worth 0.6 percent.
- Do not re-run the refuted candidates: the access-path mirror clause, the GX
  sink per-draw payload copy, `DOL_GX_CORE=0` as a rendered split, `-O3` on
  the DOL chunks, or per-instruction pc removal.
- Do not measure the rendered product with the headless metric. Headless runs
  neither GX consumer and is structurally blind to 30 percent of the rendered
  stream.
- Do not treat a pad-driven run as evidence. It is a measurement, not proof.
- Do not weaken a clause to make a log pass, and do not re-propose a
  save-state warm start; the PRD forbids captured initialization state.

## The iteration contract

An iteration counts only if a measured number moved on a named path with the
digest green and the run verified, or a new instrument answered a question
that changed a decision. A document is not an iteration and neither is a
probe. Report one paragraph to the user every three iterations.

## Fences, non-negotiable

Never commit or publish Nintendo data, original binaries, generated game code,
saves, captures, device data, signing material or leaked source. One BlueWake
process at a time; no disk-heavy work while a run is live. Everything under
`ref/` is a pinned dependency whose uncommitted state *is* the project's patch
series: to back out an edit, reverse it or restore from `patches/`, and never
`git checkout` a file there. Every patch under `ref/` must be registered in
`config/dependencies.lock.json`.

## Appendix - constants this loop relies on

Certified artifact: `build/composite-cycle-hybrid-o2-v2/gGZLE01_recomp.dylib`,
SHA `39e05177...`, 518,235,064 bytes, 756 translation units compiled at
`-O2 -ffp-contract=off`, no LTO.

Route: normal stop at pc `0x80307EF4`, host turns 32,203,791, reference digest
`92dd816c...`.

Play window (retraces 13,910..14,100, 800 live retraces): 8.1 M guest cycles per
retrace at the Gekko's 486 MHz, 302.3 M host instructions per retrace headless,
403.3 M rendered, IPC 4.01.

Dynamic census of the rendered main thread: GX FIFO path 19.6 percent (through
`host_mmio_write -> aurora_backend_gx_write -> RetailGxFrontend::flush ->
emit_new_packets -> GxCoreSink::submit_packet -> accumulate_assembly ->
on_consumed_draw`), device sync 6.4, edge service 4.3, everything else a long
tail under 2 percent.

Hot chunks, as a share of the sampled main thread:
`chunk_0201_text1_803256E0.c` 15.7, `chunk_0144_text1_802416E0.c` 3.6,
`chunk_0015_text1_8003D6E0.c` 1.5, `chunk_0145_text1_802456E0.c` 1.2,
`chunk_0181_text1_802D56E0.c` 1.1.

Emitter: `ref/recompcore/DolRecomp/src/backend/emitter.c`, with
`c_cfg.c` computing the per-instruction cycle charges, block leaders and pc
materialisation; the runtime inlines `get_ram_ptr`, `mem_read*` and
`mem_write*` from `ref/recompcore/GXRuntime/include/core/cpu.h`.
