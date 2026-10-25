# BlueWake goal loop - v52 (2026-09-22): the per-block constant is closed, the split is the question

**User-started and not blocked by the user; write the loop and go.** The governing
specification is [PRD.md](PRD.md), the operating procedure is
[GOAL_LOOP.md](../GOAL_LOOP.md), and the previous workstream page is
[GOAL_PROMPT_V51_2026-09-21.md](GOAL_PROMPT_V51_2026-09-21.md). What the PRD demands
that this loop is measured against, restated once so it is not re-derived:

- NFR-001: original 30 FPS presentation at 100 percent emulated speed; release needs
  mean emulated speed of at least 99.5 percent and a 1st-percentile of at least 95
  percent during active gameplay, frame-time p95 at most 36.7 ms and p99 at most
  50 ms, and zero interpreter-executed guest instructions across the acceptance
  corpus.
- Definition of done item 9: original game speed sustained across representative
  workloads on the documented minimum devices.
- Section 18: the agent may instrument, test, patch, commit and maintain the
  ledgers; it may not weaken a gate, reduce scope, or publish.

**The target does not move.** 60 retraces per second, 16.667 ms per retrace, on the
certified route, headless and rendered. One retrace is one guest retrace; 30 FPS
presentation is every other retrace, which is what the PRD calls authentic.

## Where this loop enters

The per-block constant was the whole of v51's workstream and it is now closed by
measurement rather than by argument. The internals are in
[status/CURRENT.md](status/CURRENT.md); the shape of it is:

| increment | measured | digest |
| --- | --- | --- |
| edge fast-reject (2026-09-21) | 486.6 -> 465.1 M | unchanged |
| overlap observation's address translation | -0.5 percent | unchanged |
| per-block dispatch leaves the loop's stack slot | -1.38 percent | unchanged |
| boundary predicate loses its call frame | -0.78 percent | unchanged |
| interrupt-source publish path inlines into the refresh | -2.29 percent | unchanged |
| **interrupt-source recomputation becomes conditional** | **-7.33 percent** | **unchanged** |
| per-term mask over those sources | **refuted**, +0.15 / +0.32 percent | reverted |

**The number that matters now: 412.3 M instructions per play retrace** (bench
window, 13,900 to 14,700, digest `83d2590d...`), against roughly 465 M at the start
of the session. The host retires about 14 G instructions per second on this
machine, so a retrace costs about 29.5 ms against the 16.667 ms target: **the gap is
1.77x**, and the steady window (14,100 to 14,700, the window a player spends time
in) is the less favourable of the play phases. Closing it needs the instruction
count to fall from 412.3 M to about 233 M, which no single remaining candidate in
the per-block path can do: the host-side per-block body is now about forty
instructions a boundary and every item left in it is worth half a percent or less.

**So this loop's first job is a measurement, not a change: where do the 412.3 M
instructions go?** The per-block constant was the cheap lever because it was
*localised* - one function called once per boundary, whose frequency the census could
count. The rest of the window is not localised: it is the emitted body, the renderer,
the DSP and audio service, the card, the VI clock, and the chassis. A split is the
only honest way to choose the next lever, and this ledger already contains one
warning about splits: W4's shares were shares of *code size*, and the dynamic census
that replaced them - GX 14-20 percent, draw planning 7.1, edge service 4.3 - was of
the *rendered* main thread, not of the headless instruction count the benches quote.

## The queue, ordered by what is known rather than by what is hoped

**1. The split.** Instrument or re-run the dynamic census against the current build
and state the 412.3 M as shares: emitted body, GX/draw submission, DSP and audio,
card and VI, chassis and dispatch. Everything below is ordered by a guess until this
exists, and the ledger has been burned twice by ordering work on a guess (the 0.22
percent constant, and the per-term mask that removed 39 percent of a counted thing
and paid nothing). The census instruments are already in the tree:
`scripts/emit_census.py`, `scripts/sample_owners.py`, `scripts/profile_play.sh`,
and the host's own `*_CENSUS` env instruments.

**2. The cycle accounting: REFUTED at the route on 2026-09-22. Read
docs/archive/status/CURRENT.md under that date before re-opening it.** The flat-body fast copy
was built (the precise copy byte-identical, the sweep at -6.0 percent) and refused by
the route: 400 retraces took 3,836,466 host turns against the control's 1,372,978 with
identical guest cycles, so removing the per-instruction charge path is not
route-neutral and the 9.8 percent the ablation prices is not available to this shape.
The reasoning that got it to the screening loop, kept for the record: `prepaid-decision` - the
block-entry decision kept, the body forced down the fast path - measures 24.26 to
24.29 instructions per guest cycle against a baseline of 26.91 to 26.92 on chunk 0144,
the same 9.8 percent that removing the decision entirely measures, so **the decision is
free and the prize is the per-instruction `!cycle_block_prepaid` test plus the
observation reconcile**. That is ~20 M instructions per retrace, less one branch a
block.

The emitter is `ref/recompcore/DolRecomp/src/backend/emitter.c` - not the
`ref/DolRecomp` checkout beside it, which is an LLVM-side tree that has never contained
the cycle emission. It emits a body in exactly two places: `emit_counted_loop` at
2042, which owns its `bool cycle_block_prepaid` at 2060 and the `label_%08X:` decision
above the body, and the function emitter at 2133, whose body carries the 4096-entry pc
table at 2149-2229.

**The counted loop was the first half and it is a null at the route** - the twin was
built, screened (9.5 min) and measured at 408.6 M against the control's 408.7 M with
identical digests, because **the loop bodies are 71 of 9,094 sampled emitted-body
frames, about 0.8 percent of the body time**. The twin stays as groundwork. **The
change that pays is the flat body**, which carries the same per-instruction test and
the same reconcile and is what the play scene runs: in `emit_function`, emit the body
twice - a fast copy with suffix-bearing labels, its own pc-table labels, no
`emit_precise_instruction_charge` and no reconcile, and every in-body `goto`
redirected into the copy it belongs to - and dispatch pc into the fast copy's labels
with the precise copy reached only from a fast copy's block-entry bail-out. A C label
is per function, so the copies need distinct names; the flat body is why that matters
and the loop was why it did not.

The route is the gate, in this order: regenerate the DOL (1.8 s) and diff the emitted
shape against the live chunks before compiling anything; screen the hot ten with
`scripts/recomp_chunks.sh` (8-15 min); bench the pair with `scripts/bench_instructions.sh`
(4 min) and require the recorded stop pcs, turn counts and digest. Register the change
as the next `patches/dolrecomp/` patch and in `config/dependencies.lock.json`; 0018 is
the highest number this emitter's series uses. The one thing the ablation could not
price is the second body's code, which is a cache effect and not an instruction count:
the screening loop is where that shows up, and a null or a regression there ends the
candidate.

**3. The GX FIFO path off the main thread: DONE 2026-09-22 with a measured win.** The
translation now runs on a worker of its own, the guest-visible synchronizations drain
it, and the certified route measures **-13.5 percent on the median rendered frame time
and -41.8 percent on p90** against the pre-deferral baseline (-16.4 and -37.7 against
the deferral-only control), every run stopping at the certified `0x8027fa30` after
40,502,699 turns, with headless inert (409.2 M against the reference's 408.7, digest
`83d2590d...` unchanged) and 217 host tests passing. The thread evidence is that the
new worker carries 16,034 of 16,287 sampled intervals while Aurora's render worker
stays blocked for 16,246 - the work moved rather than vanished. What remains of this
item is the p99 campaign on the devices P6 names, not another mechanism. See
docs/archive/status/CURRENT.md 2026-09-22. The earlier scoping entry, kept because it is what
the implementation followed:

**3b. The fact that scoped it.**
the next workstream.** The rendered profile (20 s at retrace 14,010+) puts the guest's
GX write path at **20.89 percent of the rendered main thread cumulative**,
`host_mmio_write` → `shadow_frontend_write` → `RetailGxFrontend::flush` (18.21) →
`emit_new_packets` (15.38) → `GxCoreSink::submit_packet` (14.84) →
`ConsumingAuroraRenderSink::submit_packet` (12.29) → `accumulate` (11.92) →
`on_consumed_draw` (7.24), all inside one call from the guest's store. Meanwhile
`aurora::gfx::render_worker` is blocked in a condition variable for **92.3 percent**
of intervals and `aurora::gfx::pipeline_worker` for **100 percent**, with the Metal
completion workloops at 0.4 percent. Nothing architectural pins the translation to
the main thread: the call site does. The design is a FIFO ring filled on the main
thread, a worker draining it, and a barrier at each guest-visible synchronization -
the FIFO's own read-back and idle state, the mirrored CP/BP metadata in guest order,
and the draw-done publication the guest polls at `0x80308A9C` - with the rendered
gates (digest, stops, `scripts/bench_rendered.sh`) as the judge. See
docs/archive/status/CURRENT.md 2026-09-22. **Its first implementation step is to draw the
movable/immovable line in code, not in a share.**

**4. The save-continue path.** Boot the game, load the save the game itself wrote,
reach control: the PRD's time-to-playable reading and P4 milestone 9 at once.

**5. The crumbs, in case the split says otherwise.** The overlap observation's guard
and cached slot read (a dozen instructions at 100 percent of boundaries, nil output
in play); the three device-predicate call frames in the refresh, which need a
registered `ref/` patch; the five instructions a boundary that the dispatch cache's
1.43 percent implies and its static count does not account for.

**Not to be re-run, because they are refuted with numbers.** The per-term mask over
the interrupt sources (this session). The redundant re-probes in the intercept
predicate - the predicate answers true on 1.53 percent of boundaries, which bounds
them under 0.02 percent. The access-path clause and the pc-store (v49). Branch hints
on the hot guards (a 14 percent layout regression). The register-file hypothesis.
Anything already listed as refuted in v49 through v51.

## The protocol, as of today

- `scripts/bench_instructions.sh` is the primary instrument: two ceilings differenced,
  `/usr/bin/time -l` instructions retired, its guards refusing any pair whose stop pc,
  turn count, title or play milestones do not match the recorded reference. Recorded
  stops: 13,800 -> `0x80307EF4`, 29,737,920; 13,900 -> `0x80307EF4`, 29,937,744;
  14,100 -> `0x80307EF4`, 32,203,791 (certified, digest `92dd816c...` at the
  certified window and `83d2590d...` on this build's route); 14,700 -> `0x8027FA30`,
  40,502,699.
- **Quote instructions, not milliseconds.** Wall clock varies by a third between
  processes; the instruction count on a digest-gated route does not.
- **Price from a measured frequency, not an assumed one.** The boundary census gives
  the frequency for anything per-block; the mmio census gives it for anything per
  access; the edge census gives it for the parts of the chassis body. One host
  instruction per block boundary is 0.080 percent of the steady window and 0.082
  percent of the bench window.
- **A count is not a price.** `ablate_chunk.py` for a code shape, a private build for
  a host change, and the census for a frequency; a static disassembly count screens a
  candidate and does not price it (three increments have measured 1.3 to 2 times
  their static counts).
- **Verify against the count the change could break.** The recomputation gate was
  accepted because `published` stayed at 261,281 over the whole route while
  `publishes` fell 65 percent, not because its instruction count fell.
- **Freeze what you measure.** Copy the composite and point the bench at the copy,
  build host candidates in their own build directory, and check a private build
  against the shared one with `nm -S` before its numbers are used.
- `scripts/recomp_chunks.sh` screens emitter changes on the hot ten in 8-15 minutes
  against a 90-152 minute DOL-only rebuild; `scripts/ablate_chunk.py` prices a
  construct in about two minutes; `scripts/bench_rendered.sh` gives the rendered
  split; `scripts/bench_chunk.sh` runs the roofline sweep on real in-game state. Read
  the run before reading its number.
- An iteration counts when a measured number moved on a named path with the digest
  green and the run verified, or when a new instrument answered a question that
  changed a decision, or when a candidate was refuted with a number. Report to the
  user every three iterations.

## Fences

Inherited and not negotiable: no Nintendo data, original binaries, generated game
code, saves, captures, signing material or leaked source in the repository; one
BlueWake process at a time; no timing measurement during a build; `ref/` is a pinned
dependency whose uncommitted state *is* the patch series, so reverse an edit or
restore from `patches/` and never `git checkout` a file there; register every
`ref/` patch in `config/dependencies.lock.json`; do not weaken a clause to make a log
pass; do not re-run the refuted candidates.

One this loop keeps, because three increments in one session proved it: **a generated
artifact must be able to prove itself**, and a census that counts something is only
evidence if the count it could have broken is also reported. The gate that skipped
607 M recomputations was accepted because the 261,281 publishes were still there.
