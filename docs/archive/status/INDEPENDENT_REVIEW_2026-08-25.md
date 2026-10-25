# Independent Review — 2026-08-25

**Author:** independent senior systems/runtime reviewer (out-of-loop), at the
user's request.
**Audience:** the autonomous implementation loop (`docs/GOAL_LOOP.md`).
**Status:** advisory, evidence-backed. Read at Step 0 alongside `PRD.md`,
`CURRENT.md`, and `REORIENTATION_2026-08-22.md`.
**Reviewed state:** `main` = `origin/main` = `ad3eff7` (one docs commit past
the `ade8e69` named in the review request; no code difference).

---

## 1. Executive verdict

The DSP donor integration is real and substantially complete: the Dolphin LLE
core boots the retail Zelda ucode, completes the authentic
`DCD10002`/`CDD10001`/`DCD10004`/`DCD10005` task exchanges, runs the mixer,
and DMAs frames back to the guest DAC buffers. Zero PCM at this point in boot
is **not yet evidence of any defect** — no retail note-on has been requested,
and the splash screens are silent on hardware. The loop is testing audio too
early, and the active blocker (`BW-P4-0049`, "DSP command/data continuation
exits before PCM production") is misframed.

The earliest divergence current evidence actually proves is not in audio at
all. It is a **two-clock incoherence in the host time model** — the same
defect class the 2026-08-22 reorientation corrected for the decrementer, now
one boundary over:

- timebase, decrementer, and the donor DSP advance on generated **cycle
  charges** (`runtime/host/src/main.c:8497-8507`,
  `GUEST_CYCLES_PER_TIMEBASE_TICK 12`);
- VI retrace, PAD polling, and the AI-DMA cadence fire per **dispatched block
  count** (`k_host_retrace_blocks = 1000000`, `main.c:980` and `main.c:8521`;
  `dol_audio_dma_set_vi_clock(&g_audio_dma, k_host_retrace_blocks, 60u)`,
  `main.c:2248`).

On hardware both are one clock: one NTSC retrace every ~675,000 timebase ticks
(8.1M CPU cycles). In BlueWake one retrace costs 1,000,000 dispatched blocks;
with the generated `DOLRECOMP_C_LOOP_CYCLE_BUDGET 256`, that is on the order
of 10^8 CPU cycles — retraces are roughly **one to one-and-a-half orders of
magnitude too rare relative to guest time** (exact factor to be measured, see
§9). Every anomaly the loop is currently probing — 16 frames in a 20M-block
run, byte-identical black captures, "no title consumer at retrace 16", zero
note events, splash pacing compressed into ~13 retraces — is consistent with
this one deformation and is therefore not independently probative.

**Recommendation: stop the input-schedule differentials, derive VI retrace
(and the AI cadence already chained to it) from the promoted guest cycle
clock, then re-run the no-input boot against a Dolphin reference-run oracle.**
Separately, four default-on, address-specific scheduler interventions in
`main.c` have accumulated past the GOAL_LOOP §2.15 threshold and need to be
promoted into one explicit interrupt-delivery/context-coherence contract
(§7, §11).

## 2. Verified progress

These claims were checked against code, commits, and the cited logs and hold:

- **Donor DSP LLE integration** (`2ab565e`, `88e8d6d`, `3ddbd03`;
  `runtime/host/src/dsp_adapter.cpp`): the adapter enforces the donor
  scheduling contract (12,600 CPU-cycle update cadence, 72-cycle mailbox
  slice, 6:1 CPU:DSP ratio — `main.c:1252-1284`), composes live control bits
  through `DSP_LLE_CONTROL_MASK 0x0C07` (`main.c:1435-1465`), retries
  `CR_EXTERNAL_INT` until the DSP mask accepts it, and separates ARAM from
  MEM1 accelerator access. The fabricated host mailbox protocol is correctly
  bypassed whenever the adapter is attached (`main.c:1322-1335`,
  `1363-1394`, `3336-3351`).
- **The DSP task genuinely executes.**
  `/tmp/bluewake-dsp-dma-content-3m-20260825.log` records 32
  `dma-from-dsp` transfers of 160 bytes into the retail DAC buffers
  (`0x806AFFA0`, `0x806B0400`, …) from guest-visible DSP DMA — the ucode is
  running `DsyncFrame` mixes. All payloads are zero with one identical hash
  (`nonzero=0 hash=0x02187E45`), i.e. a silent mix, not a missing mix.
- **Visible authentic boot content.** The 8M/12M/20M Aurora runs present the
  Nintendo logo and Dolby Pro Logic II splash from authentic GX submission
  (`submitted=992 rejected=0 failed=0` at 20M;
  `/tmp/bluewake-aurora-dsp-visible-8m-20260825.log`,
  `/tmp/bluewake-aurora-dsp-window-10m-20260825.png`). The 25M A-pulse log
  shows the route inside title-scene resource loading
  (`title_dolby_mark.bti` lookups, retail audio bank `.aw` opens).
- **Sequence lifecycle is healthy through `rootInit`.**
  `/tmp/bluewake-audio-root-init-4m8-20260825.log` shows sequence
  `0x80000800` at state 4 entering and returning `rootInit` normally
  (`blocks=4789599..4790950`), root track active with two children,
  `checkSeqActiveFlag` = 1.
- **PAD/SI delivery is healthy.** The A-pulse run latched `0x0100` at
  retraces 16-17 (`si-latch` lines in
  `/tmp/bluewake-aurora-dsp-a16-25m-20260825.log`); the earlier PAD
  origin/wire fixes are consistent with retail `SPEC2_MakeStatus` semantics.
- The prior reorientation was implemented: the composite emits cycle charges
  in all 748 chunks (`tests/test_cpu_abi_contract.py::test_full_composite_emits_cycle_charges`),
  and timebase/decrementer derive from them
  (`main.c:1208-1237`, `8497-8520`).

## 3. Earliest authentic blocker

**BW-P4 (reframe): VI retrace and AI cadence are not derived from the
promoted guest cycle clock.** Category `CPU_RUNTIME`, owner: BlueWake host
(`runtime/host/src/main.c`), not the donor DSP, not JAudio, not Aurora.

Measured code facts:

- `main.c:977-980` — comment and constant: "One host retrace per million
  dispatched guest blocks"; `main.c:8521` — `(blocks + 1) %
  k_host_retrace_blocks == 0` asserts VI retrace and latches the PAD poll.
- `main.c:8497-8507` — timebase, decrementer, and donor DSP advance from
  `-cpu.downcount` cycle charges each dispatch.
- `main.c:2248` + `ref/recompcore/GXRuntime/src/audio_dma.c:82-90` — the
  AI-DMA chunk cadence is `work_units_per_frame × 60` in **block** units.

Hardware contract (inference from documented constants, consistent with the
runtime's own `GUEST_CYCLES_PER_TIMEBASE_TICK 12` and 6:1 DSP ratio): 486 MHz
CPU, 40.5 MHz timebase, ~60 Hz VI ⇒ **675,000 timebase ticks (8.1M CPU
cycles) per retrace**. The block-based retrace is therefore slower than the
cycle clock by a factor of roughly `avg_cycles_per_block × 1,000,000 /
8,100,000` — between ~8× and ~31× depending on the true average charge per
dispatch (bounded by the 256-cycle budget). The exact factor is one
measurement away (§9, Tier 0).

Downstream consequences currently being misread as separate mysteries:

- A 20M-block run yields only ~16-20 frames/retraces, so any milestone that
  needs N frames needs N million blocks — this is why soaks are
  "impractically slow" and why the title consumer does not exist by retrace
  16 on this route.
- Splash transitions that wait on OS time (cycle clock) complete in a handful
  of presented frames — the observed logo→Dolby→black compression across
  ~13 retraces. Content is authentic; **pacing is not**.
- Retrace-indexed input schedules (room-0 pulses, A96 16/22/26) are
  calibrated in a deformed, route-relative retrace space. The DSP build
  changed average block cost, so the same retrace index no longer maps to the
  same guest progress — exactly what the a16 negative result showed.
- JAudio sequence tempo (AI/DSP frame cadence, block clock) and guest wall
  time (cycle clock) disagree by the same factor.

## 4. Evidence accepted

- `/tmp/bluewake-aurora-dsp-visible-8m-20260825.log` (+5M/7M window PNGs) —
  visible authentic logo, clean GX stats.
- `/tmp/bluewake-aurora-dsp-12m-20260825.log`,
  `/tmp/bluewake-aurora-dsp-window-10m-20260825.png` — Dolby splash reached.
- `/tmp/bluewake-aurora-dsp-20m-20260825.log` — 16 frames, 992 ops,
  `rejected=0 failed=0`, normal stop `pc=0x80307EF4`; 12 `deterministic
  retrace` report lines (cap 12) at exact 1M-block intervals — direct log
  confirmation of the block-based cadence.
- `/tmp/bluewake-dsp-dma-content-3m-20260825.log` — 32 all-zero 160-byte
  DSP→guest DMA payloads, single hash.
- `/tmp/bluewake-audio-root-init-4m8-20260825.log` — `rootInit`
  entry/return, state 4, coherent object pointers.
- `/tmp/bluewake-aurora-dsp-a16-25m-20260825.log` — complete; `0x0100`
  latched at retraces 16-17, 21 frames, normal 25M stop, no title consumer,
  no note event, no nonzero DSP payload. Accepted as a clean **negative**
  result whose cause is over-determined by §3.
- `/tmp/bluewake-dsp-control-mask-800k/3m-20260825.log` — donor task-yield
  exchanges and normal stops as described in `BLOCKERS.md`.

## 5. Claims not supported by evidence

1. **"Default-on DSP route."** `BLUEWAKE_ENABLE_DSP_ADAPTER` defaults to
   `OFF` (`runtime/host/CMakeLists.txt:8`). The donor route is default-on
   *within the explicit `build/runtime-host-dsp` configuration only*; the
   repository-default build still runs the **fabricated** host mailbox
   protocol (`0x8071FEED`, `DCD10000/10004`, `F355FF00` — `main.c:1239-1250`,
   `1337-1431`). CURRENT.md's banner language conflates a runtime-flag
   default with a build default.
2. **"Authentic … splash progression."** The splash *content* is authentic;
   the *pacing* is not evidenced as authentic and is almost certainly
   deformed by §3 (a hardware boot does not traverse logo→Dolby→black in ~13
   retraces). Promoted milestones should say "authentic content, inauthentic
   frame pacing" until the cadence is coherent.
3. **"The CPU/runtime contract suite passes 13/13" as behavioral
   verification.** `tests/test_cpu_abi_contract.py` is almost entirely
   source-text presence assertions on `main.c`/generated files (grep-style
   tripwires), including assertions that specific diagnostic `fprintf`
   format strings exist. Useful as drift tripwires; not runtime verification,
   and it hard-wires temporary diagnostics into the suite so that future
   cleanup will superficially look like test-weakening.
4. **Ledger dates.** Files named `…-20260826` through `…-20260829` were
   created on Aug 24-25 (filesystem mtimes), and `BW-P4-0048` records "First
   seen: 2026-08-26" — one day in the future. All date-bearing fields in
   `CURRENT.md`/`BLOCKERS.md` are unreliable for ordering and violate the
   PRD §16 evidence-record intent.
5. **Zero PCM as an audio defect.** Nothing observed contradicts "the mixer
   is correctly mixing zero active voices." No `BankMgr::noteOn` has fired;
   no retail note has been requested; the splash interval is silent on
   hardware. Until a reference run establishes *when* retail first produces a
   nonzero DSP payload, "silence" is not a failure signature.

## 6. Ruled-out hypotheses (accepted as closed — do not reopen)

- ARAM/`allocBack`/heap-boundary/`0x802B5FCC` corruption —
  `SUPERSEDED_BY_DECISION` (2026-08-22 reorientation). Still closed.
- DSP mailbox word format, task-descriptor corruption, donor initialization,
  `DSBL` length semantics — closed by the donor scheduler/control increments
  (`2ab565e`, `3ddbd03`) and the 800k/3M task-exchange logs.
- Platform sink startup, SDL prebuffer, PCM conversion, queue cadence —
  closed by playback-start logs (zero-content pushes delivered intact).
- Sequence selection/activation, `SeqUpdateData` object corruption — closed
  by the corrected-oracle traces and the `rootInit` state-4 evidence.
- PAD/SI wire delivery of the A edge — closed by the a16 latch evidence.
- "Current thread is null at first audio receive" — was a mislabeled-field
  diagnostic artifact; closed in the ledger, stays closed.

## 7. Risks in current runtime behavior

Four **default-on, address-specific interventions** now sit in the host's
scheduler/exception path. Each is fitted to one measured state of one thread:

1. `preserve_selector_return` (`main.c:1100-1113`) — during external
   interrupt context save, skips writing live `r3` into the saved context,
   but only for the audio thread context `0x803E9260` at `pc=0x80307EAC`
   with an exact state/`r3` pattern.
2. Idle interrupt restore (`main.c:3312-3333`) — the host forces
   `MSR |= EE` at `pc=0x80307EAC` under two hard-coded memory patterns,
   instead of the guest's own code re-enabling it.
3. `SelectThread(FALSE)` no-preempt redirect (`main.c:6693-6712`) — the host
   **rewrites the guest PC** (`cpu.pc = 0x80307E2C`) when the audio thread's
   priority invariant holds. If the generated code takes the wrong branch
   while the invariant holds, that is a translator/codegen defect that
   deserves a reduced reproducer; steering the PC hides it, and only for the
   audio thread — the identical defect class is unguarded for every other
   thread the game will create during gameplay, which predicts confusing
   future S1s at other addresses.
4. `dispatch_unwind_boundary` (`main.c:3432-3434`) — interrupt delivery is
   suppressed inside hard-coded PC ranges (`0x80307FD0`, `0x80304DF8`,
   `0x80303A50-0x80303B24`), an undocumented "interrupt-safe-point"
   contract.

These are all symptoms of one systemic gap: **interrupts are delivered at
translated-block boundaries where guest architectural state is only partially
materialized**, and each observed inconsistency has been patched at its
address. GOAL_LOOP §2.15 ("if diagnostic hooks are added and removed more
than twice, promote the subsystem") applies: promote a single explicit
interrupt-delivery/context-coherence contract (which PCs/blocks are safe
points; what the context-save invariants are), test it in the existing
public `bluewake_cpu_boundary_test`, and retire the per-address carve-outs
against it. Until then, keep them — removing them blind would regress —
but stop adding a fifth.

Additional risks:

- **Opt-in state-fabrication hooks remain in tree** —
  `BLUEWAKE_WAKE_DEFAULT`, `BLUEWAKE_WAKE_DVD`, `BLUEWAKE_LOAD_DVD_CONTEXT`,
  `BLUEWAKE_RESUME_SCHEDULER` (`main.c:8665-8729`) write guest thread
  records, run queues, and `OS_CURRENT_THREAD` directly. The reorientation
  §3 said to stop building removable hooks; GOAL_LOOP §2.13 makes their
  evidence inadmissible anyway. Remove or quarantine.
- **Superseded-investigation instrumentation still in the hot path** —
  `heap_host_delta` snapshots four heap words on *every* delivered external
  interrupt and decrementer past 7M blocks (`main.c:3437-3445`,
  `8511-8519`), a leftover of the closed ARAM hunt. The FP-unavailable host
  resume (`main.c:8787-8801`) replaces the guest OS lazy-FP handler with a
  host reimplementation — currently justified, but it should be recorded as
  a host-owned service replacement, not authentic guest behavior.
- **Diagnostic weight is now the run-rate limiter.** Two recent probes were
  operator-stopped because logging made them impractically slow; dozens of
  per-dispatch `getenv`/watch checks sit in the dispatch loop. After the
  cadence fix (which multiplies retrace/frame work), this worsens.
- **Evidence lives in `/tmp`.** Volatile; several artifacts are SHA-256'd in
  the ledgers, but the files themselves will not survive a reboot. GOAL_LOOP
  §3 puts evidence under `local-research/evidence/`.

## 8. Answers to the review questions

1. **Blocker framing:** wrong subsystem. The active work is framed as
   AUDIO/DSP ("continuation exits before PCM production") but the DSP
   continuation demonstrably completes and mixes. The correct earliest
   blocker is `CPU_RUNTIME`: the frame/retrace clock is not derived from the
   promoted cycle clock (§3). Ownership is the BlueWake host main loop.
2. **Zero PCM:** not proven anomalous; the loop is testing audio before the
   first retail note event. The decisive oracle is a Dolphin reference run
   from the same disc: frame index of first nonzero DSP DMA payload and
   first `noteOn`. Until BlueWake reaches the equivalent progress point
   under a coherent clock, silence is expected.
3. **A-pulse differential:** was not the smartest experiment, and its planned
   16/22/26 successor inherits the same flaw: fixed-retrace schedules in a
   deformed, route-relative retrace space. Its negative result was
   predictable from the frame count alone. Replace with the cadence fix plus
   event-synchronized input (fire the edge when the title consumer is
   observed to exist — GOAL_LOOP 14.5's milestone-synchronization rule).
4. **Earliest proven divergence:** the two-clock incoherence, code-anchored
   at `main.c:980/8521/2248` vs `8497-8507`. Everything downstream (16
   frames/20M blocks, black captures, missing title consumer, zero notes) is
   consistent with it and proves nothing further on its own.
5. **Retained corrections audit:** donor LLE scheduling and the `DSBL`
   bit-15 strip are source-aligned and regression-covered — keep. The four
   scheduler interventions and the FP resume are host-owned deviations
   fitted to measured states — keep for now but promote per §7. The
   "default-on" claim is overstated (§5.1). The env-gated scheduler
   fabrication hooks should be deleted.
6. **Ledger health:** `CURRENT.md` (2,332 lines) still carries superseded
   "next smallest action" directives mid-file (post-wrap `SelectThread`
   traces, MSR experiments, fabricated-mailbox-era conclusions) that a
   resuming agent reading linearly could re-execute; three differently-worded
   "active" `BW-P4-0049` records coexist in `BLOCKERS.md` (donor-era,
   DSP-status-exit-era, and the long pre-donor record whose evidence came
   from the fabricated mailbox route); and all dates are unreliable (§5.4).
   Reframing in §12.
7. **Next hypothesis:** §9.
8. **Prohibited experiments:** §11.

## 9. One falsifiable next hypothesis

> **Hypothesis.** Because VI retrace and the AI cadence fire per 10^6
> dispatched blocks while timebase/decrementer/DSP advance on generated cycle
> charges, guest wall-time runs N× faster than the frame clock
> (N = measured timebase-ticks-per-retrace ÷ 675,000, predicted ≈ 8-31).
> If VI retrace is asserted every 675,000 timebase ticks of the promoted
> guest clock (with the AI work-rate re-derived from the same clock), then a
> no-input DSP-route replay will present ≈ 60 frames per guest second, the
> logo/Dolby/title transitions will occur at retrace indexes matching a
> Dolphin reference run from the same disc, and the title scene will become
> input-ready within an existing 20-35M-block bound — while the no-disc
> control still stops normally and the focused adapter/C-ABI/GXRuntime tests
> still pass (control observables unchanged).

### Tiered verification plan

- **Tier 0 — measure before changing (one bounded metric, no behavior
  change):** log `cpu.timebase` at each of the first ~16 retraces in one 3M
  DSP-route replay. This pins N and converts the §3 inference into a
  measured fact. If N ≈ 1 (timebase-per-retrace ≈ 675k), this review's
  primary finding is falsified — stop and report.
- **Tier 1 — unit:** drive retrace from the guest clock
  (`guest_clock_advance` already owns the cycle→tick conversion); update
  `dol_audio_dma_set_vi_clock`'s work-rate source to the same clock. Extend
  `bluewake_cpu_boundary_test`/contract tests with a behavioral assertion
  (ticks-per-retrace), not only text assertions.
- **Tier 2 — controls:** no-disc smoke and the focused adapter/C-ABI/
  GXRuntime suites unchanged.
- **Tier 3 — authentic replay:** 20M-block no-input DSP-route run. Expected:
  hundreds of retraces, frame count ≈ retrace count, splash pacing spread
  over retail-plausible frame counts, and materially deeper boot progress
  per block.
- **Tier 4 — reference oracle (Tier D per PRD 14.4):** one Dolphin run from
  the user's own disc recording frame indexes of: logo appearance, Dolby
  splash, first title-scene frame, first nonzero DSP→RAM payload, first
  `noteOn`. Compare BlueWake's retrace-indexed markers against it. This
  answers the audio question definitively and cheaply.
- **Tier 5 — input:** only after Tier 3/4, one A edge fired on an observed
  title-readiness marker (event-synchronized, still authentic PAD input),
  not a fixed retrace index.
- **Staleness:** on Tier 1 landing, mark every retrace-indexed schedule and
  milestone (room-0 retraces 18-19, A96 16/22/26, a16) `STALE` per PRD §16 —
  the oracle changed.

## 10. Explicit stop-doing list

1. **Stop fixed-retrace input-schedule differentials** (the planned 16/22/26
   three-edge replay included) until retrace cadence is cycle-coherent. This
   is the third input-schedule experiment shape; §6 anti-stall applies.
2. **Stop extending unchanged no-input replays to larger block bounds**
   (8M→12M→20M→25M is four repetitions of one shape). Longer bounds on the
   current cadence buy retraces at ~1M blocks each and cannot reach the
   title in practical time.
3. **Stop tracing zero PCM** — no more `rootInit`/track-processing/DMA
   payload probes until the Tier 4 reference run defines when nonzero output
   is even expected.
4. **Do not add a fifth address-specific scheduler/exception carve-out.**
   The next scheduler symptom triggers the §7 subsystem promotion instead.
5. **Do not use non-adapter-build (fabricated-mailbox) runs as audio
   evidence** now that the donor route exists (GOAL_LOOP §2.13).
6. Standing prohibitions remain: no ARAM/allocBack/heap/`0x802B5FCC`
   tracing; no sample or mailbox-word injection (both prior injections
   produced handler storms and were reverted); no forced scenes, captured
   state, or synthetic-route evidence promoted as authentic; no edits to
   generated C; at most one Simulator (none needed for any step above).

## 11. Recommended CURRENT.md / BLOCKERS.md reframing

- **Open one blocker:** `BW-P4-0050 — VI retrace/AI cadence not derived from
  the promoted guest clock` (CPU_RUNTIME, S1, owner: host main loop), citing
  `main.c:980`, `main.c:8521`, `main.c:2248`, with the §9 hypothesis as its
  next action. Make it the single active P4 blocker.
- **Re-scope `BW-P4-0049`** to: "not reproduced as a defect — awaiting
  coherent-clock replay past the first reference note event." Collapse its
  three concurrent "active" records into one; move the fabricated-mailbox-era
  narrative (the `…-2026082[6-9]` entries) into a clearly labeled
  historical/superseded section noting that its evidence predates the donor
  route.
- **Keep `BW-P4-0048`** (presentation/stability qualification) active but
  blocked-on `BW-P4-0050`, since stability soaks are meaningless at ~16
  frames per 20M blocks.
- **Open a tracked `S2` runtime-debt item** for the §7 scheduler-intervention
  promotion (interrupt-safe-point/context-coherence contract), and one for
  deleting the `BLUEWAKE_WAKE_*`/`BLUEWAKE_RESUME_SCHEDULER` fabrication
  hooks and the leftover `heap_host_delta` instrumentation.
- **Ledger hygiene:** correct the future-dated filenames/"first seen" fields
  or annotate them untrusted; archive the resolved back-half of `CURRENT.md`
  (keep the banner + current state + the last two increments; move the rest
  to `docs/status/archive/`); copy the SHA-256'd `/tmp` evidence into
  `local-research/evidence/` before it evaporates.
- **Wording:** replace "default-on DSP route" with "DSP-adapter build
  (`build/runtime-host-dsp`), donor route default-on at runtime"; qualify
  splash milestones as "authentic content, pacing unqualified."

## 12. Confidence level and unresolved questions

**High confidence (code- and log-anchored measured facts):** the two-clock
split (§3 code citations; 1M-block retrace lines in the 20M log); the donor
DSP executing and DMAing zero frames; the fabricated-mailbox gating; the four
scheduler interventions and their exact guards; the CMake `OFF` default; the
future-dated ledger entries; the a16 negative result.

**Medium confidence (inference, one measurement from proof):** the skew
factor N (bounded 8-31×, Tier 0 measures it); "splash pacing is time-driven,
hence compressed" (corroborated by the observed retrace indexes but not
traced); "title readiness needs materially more frames than 16."

**Lower confidence (needs the Tier 4 reference run):** the retail frame
index of the first nonzero DSP payload and first `noteOn` — this review
assumes the boot splashes are silent and the first audible event is at/near
the title screen; the reference run must confirm before "audio is healthy
through the splash interval" is promoted to a fact.

**Unresolved questions for the loop:**

1. What is the measured N (Tier 0)? If ≈ 1, this review's primary
   reorientation is wrong and the loop should say so and continue its
   current framing.
2. Does the title scene on this route require an input edge at all, or does
   it auto-present after the splash under a coherent clock? (The retained
   "bounded controller edge" claim predates the DSP route; re-derive it from
   the reference run.)
3. After the cadence fix, does the wall-clock cost per guest second remain
   practical with current diagnostic weight, or does the §7 diagnostic
   pruning need to land first?
4. Are the `preserve_selector_return`/no-preempt states reproducible under
   the coherent clock? If either stops triggering, its underlying
   translator-granularity question may have been timing-dependent and the
   carve-out can be retired earlier than expected.
