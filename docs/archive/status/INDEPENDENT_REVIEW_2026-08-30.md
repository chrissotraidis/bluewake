# Independent Review — 2026-08-30

**Author:** independent senior emulator/runtime, compiler, graphics, and macOS
performance reviewer (out-of-loop), at the user's request.
**Audience:** the autonomous implementation loop (`docs/GOAL_LOOP.md`).
**Status:** advisory, evidence-backed. Read at Step 0 alongside `PRD.md`,
`CURRENT.md`, `REORIENTATION_2026-08-22.md`, and
`INDEPENDENT_REVIEW_2026-08-25.md`.
**Reviewed state:** `main` = `12f9c83` plus the uncommitted `BW-P4-0062`
execution-budget candidate (`runtime/host/src/execution_budget.{c,h}`,
`runtime/host/src/main.c`, `scripts/generate_composite.py`,
`tests/execution_budget_test.c`, `tests/test_cpu_abi_contract.py`).
Line numbers cite that state. Static review only; no build, run, or simulator
was launched, and the two user-edited files in `ref/recompcore`
(`GXRuntime/src/core/cpu_exception.c`, `GXRuntime/src/hle/hle_core.c`) were
read but not touched.

---

## 1. Executive verdict

The correctness engineering since 2026-08-25 is genuinely strong: the
two-clock defect from the last review is fixed (VI/AI/decrementer/DSP all
derive from the promoted cycle clock — `runtime/host/src/main.c:2955-2960`,
`:10314`), the lazy-FPU ownership work is the right model, and the exact
matched-route discipline has repeatedly caught real semantic drift that most
projects would have shipped.

The performance situation is different in kind from how the ledger frames it.
The loop has been optimizing the *contents* of host turns while the dominant
cost is the *number* of host turns — and the number of host turns is dominated
by a translator feature that exists, is already written, and is **never
enabled**: cross-chunk direct calls (`emit_set_chunk_table` has no caller
anywhere in DolRecomp, so `emit_cross_chunk_call` is dead code and all 121,427
cross-chunk `bl` sites, 84.6% of static call sites, compile to
`ctx->lr=…; ctx->pc=…; return;`). Each guest call that leaves its 16 KiB tile
costs two host turns, and each host turn costs ~1,059 host cycles of
dispatch/loop machinery around an average of only 92 guest cycles of work.
This — not the 256-cycle budget — is why "translated dispatch" owns half of
every scene.

The deadline-capped dynamic budget is directionally reasonable but is built on
an acceptance premise that cannot hold in general: **bit-exact equivalence
with the 256-quantum baseline is not achievable by any turn-count-changing
optimization**, because the baseline's own observable timing (timebase and
decrementer reads, device-state MMIO reads, interrupt delivery points) is
defined *by* its turn quantization. The fixed-1,024 probe's ±2 lazy-FPU drift
and 330-cycle final overshoot were not implementation bugs; they were this
principle showing itself. Section 4 details three concrete defects in the
current candidate and the contract change that makes budget scaling — and
cross-chunk chaining — exactly qualifiable instead of luck-dependent.

Two further headline results: the DSP's 30.5% Omasao / 11.9% Outset share is
overwhelmingly the Zelda ucode **spinning in mailbox-poll idle loops**, not
mixing audio (the donor's own idle-skip analyzer identifies these exact loops
and is unreachable by construction), and Link's hair defect has a specific
prime suspect — a real logic bug in the single-texmap fast path that binds the
*last-written* texture instead of the texture the TEV stage samples.

Finally, on product honesty: no human being has ever played or listened to
BlueWake. A live keyboard path and an audible SDL audio sink both exist and
are plausibly functional, but every accepted run uses scripted PAD routes that
*overwrite* live input. "Controllable Link" in `GATES.md` is evidence of a
controllable *machine*, not a playable game. That gap is cheap to start
closing and should be closed early, because it will surface a class of defects
(pacing, latency, audio quality, camera feel) that no digest can.

---

## 2. Critical findings

Ordered by severity × leverage. **[O]** = observed fact, **[I]** = strong
inference, **[S]** = speculation.

### F1 — Cross-chunk call chaining exists in the translator and is dead code (highest leverage in the repository)

- **[O]** `func_XXXXXXXX` is one C function per 4,096-instruction (16 KiB)
  address tile (`ref/recompcore/DolRecomp/src/app/pipeline.c:34`), not per
  guest function. BlueWake is already at the maximum tile size.
- **[O]** Cross-chunk direct calls have a full implementation —
  `emit_cross_chunk_call` (`ref/recompcore/DolRecomp/src/backend/emitter.c:355-376`)
  with depth guard and inline resume — but it bails when `chunk_start_for()`
  returns 0, which it always does because `emit_set_chunk_table`
  (`emitter.c:321`) **has no caller anywhere in the translator**. Zero of the
  748 generated chunk files contain `dolrecomp_call_enter`.
- **[O]** Static census over all 748 chunks: 121,427 cross-chunk `bl` sites
  (84.6% of 143,439 direct-call sites) compile to a host return; 5,710
  `bctrl`/`bclrl` and ~6,064 `bctr` sites (every C++ virtual call in the
  actor system) always return; `blr` back to a cross-chunk caller returns via
  the `return_dispatch` `default:` case (`emitter.c:2001-2013`).
- **[O]** Per-turn economics from the matched 700-retrace pair
  (`docs/archive/status/PERFORMANCE.md:68-76`): 61,469,941 turns for 5.67B guest
  cycles = **92.2 guest cycles per turn**; 4,502 retired host instructions and
  1,059 host cycles per turn. Each turn pays ~30 dispatch instructions plus an
  unpredictable indirect call, 12 callee-saved register spills/reloads in the
  chunk prologue, an entry jump table (a second unpredictable indirect
  branch), and 600–1,000 instructions of host-loop body.
- **[I]** Cross-chunk calls and their paired returns are the dominant
  host-return cause (est. 60–80% of turns combined); the cycle budget is
  bounded at ≤36% by arithmetic and measured at 23.5% by the fixed-1,024
  probe. The dynamic-budget candidate is therefore optimizing the *minority*
  turn cause.

### F2 — The exact-equivalence gate is quantization-bound; no turn-count-changing optimization can pass it except by luck

- **[O]** `mfdec` returns the boundary-updated `g_guest_clock_decrementer`
  without accounting for in-flight `downcount` (`main.c:435-446`). Timebase is
  advanced only in `guest_clock_advance` at turn boundaries
  (`main.c:1277-1301`). Device state (DSP, audio DMA, VI event clock) advances
  only at turn boundaries (`main.c:10280-10315`). Interrupts are delivered
  only at turn boundaries (`main.c:10310-10313`).
- **[I]** Therefore every guest observation of time or device state is stale
  by "cycles since the last turn boundary," and every interrupt lands at a
  boundary position determined by the quantum. Change the quantum (fixed
  1,024, dynamic 256/1,024, or cross-chunk chaining) and boundary positions
  shift; observations and deliveries shift with them; small drift (moved
  lazy-FPU switches, deferred-FP observations, final-cycle offsets) is the
  *expected* signature, not a defect of the candidate. The fixed-1,024
  rejection evidence (`PERFORMANCE.md:102-105`) is exactly this signature.
- **Consequence:** the loop is about to spend a full private composite rebuild
  qualifying a candidate whose failure mode is baked in. Section 4 gives the
  contract change (cycle-accurate observation + deadline-domain delivery) that
  converts "hope the phase aligns" into "equivalence by construction," and
  makes quantum-invariance itself a permanent regression test.

### F3 — Concrete defects in the uncommitted budget candidate

Even accepting the current acceptance premise, the candidate has holes
(details in §4): the `mtdec` path creates deadlines mid-dispatch with no
budget clamp; nested dispatches in `callback_delivery.c` and any other
re-entrant path run on a stale budget; the guard window ignores back-edge
overshoot (there is no budget check at chunk entry, only at loop back-edges
and `return_dispatch`); and the qualification harness's identity counter
("3,670,902,066 blocks") changes by design under any budget change, so the
pass/fail comparison set must be explicitly redefined before the run, not
after.

### F4 — Performance micro-experiments are being adjudicated on the wrong scene

- **[O]** The FP-availability inline, CARD dispatch envelope, and both GX
  allocation-reuse probes were accepted/rejected on **700-retrace title-route
  A/Bs** (`PERFORMANCE.md:134-158`, `:737-781`), a scene with almost no
  gameplay FP, collision, or draw load. The Outset sample shows
  `ppc_fp_available`, `psq_load_value`/`psq_store_value`, and FP helpers
  (`ni_madd_msub`, `ppc_fcmp`, `ppc_fmuls`) prominently in generated-code
  stacks, and the composite contains **305,494** out-of-line
  `ppc_fp_available_inline` call sites (`emitter.c:635-636`).
- **[I]** Rejections made at title scale are not valid for gameplay scale.
  The loop needs one short **exact Outset-scene A/B route** (a bounded replay
  with its own digest) as the standard performance-adjudication tier; the
  title route should qualify correctness, not rank gameplay owners. This is a
  systematic selection-bias flaw in the current experiment shape, and it means
  some "closed" micro-shapes (FP inline in particular) were closed on
  unrepresentative evidence — reopen them only after a gameplay-scene profile
  re-ranks them, per the loop's own rules.

### F5 — The DSP cost is mostly idle-loop interpretation; an exact fast-forward is available and in-methodology

- **[O]** The Omasao profile's DSP subtree is dominated by `Step()` self time
  and the opcode mix `lr`/`jcc`/`lri`/`tstaxh` — the donor analyzer's own
  Zelda busy-wait signatures (`ref/recompcore/Source/Core/Core/DSP/DSPAnalyzer.cpp:51-60`).
  Real mixer math (`madd`/`mulcac`/`mulmvz`/`mulxac`) is a rounding error.
  81M DSP instructions are interpreted per guest second at ~30–38 host cycles
  each; ~53% of that is dispatch scaffolding.
- **[O]** Idle-skip machinery exists in the donor and is unreachable by
  construction: `batch_limit = 8` (`runtime/host/src/dsp_adapter.cpp:299-304`)
  never advances past `Interpreter::RunCycles`' no-idle-skip prefix
  (`DSPInterpreter.cpp:201-202`). The analyzer flags are computed after ucode
  load and never read.
- **[O]** Dolphin's stock idle skip is *inexact* (it discards remaining cycle
  budget — the mechanism behind the rejected 64-cycle batch's DMA-count
  shift). An exact variant — verify the loop body touches only `pc`/`SR`,
  advance the DSP cycle counter by the full remaining slice, land `pc`/`SR`
  at the modular position — preserves the digest and matches the already
  promoted CPU zero-poll fast-forward pattern (Decision 2026-08-28).
- **[O]** Free companion fix: `Adapter::run_cycles` has **no halt early-exit**
  (`dsp_adapter.cpp:286-307`) — while `CR_HALT` is set it still spins
  `cycles/8` iterations of interrupt-check + instruction peek + a probe into
  the 512 KiB op-template table, and halt can only clear via
  `write_control` (`:539-552`), so exiting early is provably exact.
- **[O]** A complete, WW-validated Zelda-ucode **HLE exists in-tree**
  (`ref/recompcore/Source/Core/Core/HW/DSPHLE/UCodes/Zelda.cpp`;
  `ZeldaUCodesTable.cpp:26-27` lists WW's CRC `0x86840740` with zero quirk
  flags) and is simply excluded from the donor archive
  (`runtime/host/CMakeLists.txt:333-336`). It would delete ~3×10¹⁰
  interpreted instructions per full route but breaks the bit-exact DSP digest
  by construction — an acceptance-policy decision, not an experiment.

### F6 — Link's hair defect has a specific prime suspect, and the best isolation tool needs zero source changes

- **[O]** **Single-texmap fast path binds the last-written texture, not the
  sampled one.** `bound_texture_` is overwritten by every texture resource
  packet (`ref/recompcore/GXRuntime/graphics/frontend/src/render_sink.cpp:133-175`);
  when `texmap_popcount(used) <= 1` the plan's texture fields come from that
  last-written slot (`graphics/gxcore/src/gxcore.cpp:1377-1394`, `:799-803`)
  and the shader always samples `tex0` (`gxcore_shader.cpp:336`). J3D
  materials bind an IMAGE0/IMAGE3 pair per texmap in the material display
  list, so any material with multiple bound texmaps but one *enabled* TEV
  stage samples the wrong image; if the last-written slot fails to resolve,
  `textemp` becomes opaque white and alpha-tested hair cards render as solid
  quads. This reads exactly like the reported defect.
- **[O]** Secondary suspects, all counter-instrumented already: TEV stages
  truncated at 8 (hardware max 16; final-stage output substitution —
  `shader.hpp:109-110`, `gxcore.cpp:810-814`), texgens truncated at 5 of 8
  with Colors/Binormal/Tex4-7 source rows emitting constant coordinates
  (`shader.hpp:104-107`, `gxcore_shader.cpp:1019-1020`), destination-alpha
  requiring Dawn dual-source blending support at runtime
  (`gxcore_draw.cpp:303`, `webgpu/gpu.cpp:897-915`), and mip-0-only texture
  uploads under mip-enabled samplers.
- **[O]** Ranked *lower* with reasons: matrix/skinning (faithful Dolphin-parity
  PNMTXIDX port), display-list parsing (CALL_DL recurses on the identical
  byte path), cull/depth/TLUT/vertex-attribute decode (complete
  implementations, correct conventions).
- **[O]** The smallest isolation experiment requires no renderer change:
  `DOL_AURORA_RECOMP_TRACE_OUT` + `DOL_AURORA_RECOMP_TRACE_FRAMES` captures
  2–3 Omasao frames; `dolgx_replay --histogram` immediately confirms or
  refutes TEV>8/texgen>5; `dolgx_replay --core --png-dir` produces the
  repeatable game-data-free pixel checkpoint `BW-P4-0063` requires — headless,
  offline, no live boot.
- **[O]** Diagnostic wiring gap: the per-draw
  `DOL_AURORA_RECOMP_DRAW_TRANSFORM_*` observer is installed on the shadow
  sink only (`backends/aurora/aurora_backend.cpp:331-333`) and produces **no
  output on the default gxcore render path**. One observer-forwarding change
  in `GxCoreSink::on_consumed_draw` would fix it.

### F7 — No human has ever played or heard BlueWake, and the scripted route actively prevents it

- **[O]** A live keyboard path exists (Aurora SDL3 backend, default WASD/JKUI
  mapping — `ref/recompcore/GXRuntime/backends/aurora/aurora_input.cpp:19-47`)
  and merges additively into SI (`runtime/host/src/pad_wire.c:10-28`) — but
  `host_pad_state` **overwrites** the merged stick whenever a waypoint is
  active (`main.c:1683-1701`), and legacy pulse variables zero merged buttons
  outside their windows (`main.c:1679-1681`). Every accepted run used these.
- **[O]** Audible audio is fully wired (AI DMA → SDL3 stream with 40 ms
  prebuffer and 250 ms queue cap — `aurora_audio.cpp:30-101`) and active in
  signed Aurora runs; the WAV capture is a separate diagnostic tap. No human
  listening judgment has ever been recorded.
- **[O]** There is **no frame limiter or pacing logic anywhere in the host** —
  once performance improves, presentation will free-run against vsync with
  nothing reconciling guest VI cadence to the display. There is no fullscreen,
  no settings surface, no UI at all; launch requires ~38 environment
  variables and a shell invocation; the .app is not double-clickable.
- **[O]** `GATES.md:9`'s "P4 milestone set now passes … controllable Link"
  overstates: every listed milestone was evidenced under scripted PAD that a
  human cannot coexist with. The same cell does list physical control as
  outstanding, but the table row reads as a pass.

### F8 — Evidence and test hygiene regressions persist from the 2026-08-25 review

- **[O]** **8,545 `bluewake-*` evidence files still live in `/tmp`**,
  including the four samples backing the current `BW-P4-0062` decision. The
  08-25 review flagged this; it was not acted on. A reboot destroys the
  provenance behind every SHA-256 in the ledgers.
- **[O]** "211 tests" is an aggregate: 18 CTest binaries + 61 Python test
  functions (of which ~35 are source-text greps on `main.c`/generated files,
  including the new budget assertion added in this working tree) + 6 manual
  GPU pixel fixtures (one known flaky). CI builds nothing and runs no test
  (`.github/workflows/audit.yml` is an 11-line repo audit).
- **[O]** `COMPATIBILITY.md` describes a coverage universe;
  `tests/coverage/catalog.json` contains 3 records, none gameplay content.
- **[O]** There is no automated visual digest/frame-hash regression; every
  framebuffer hash in `CURRENT.md` was transcribed by hand from manual runs.

---

## 3. Performance ownership audit (question 1)

**The diagnosis is correctly measured but incompletely decomposed.** The
samples are admissible (correct process attachment, exact-route correctness
match) and the headline split — Outset 51.2% translated dispatch / 12.0%
nested GX / 11.9% DSP; Omasao 49.4% / 12.1% / 30.5% — is real. What the
call-tree attribution hides:

1. **"Translated dispatch" conflates guest work with turn tax.** Inside
   `selected_dispatch` live both actual translated guest execution and the
   per-turn machinery (dispatch lookup, 12-register chunk prologue/epilogue,
   entry jump table). Outside it, the surviving host-loop body (173 top-level
   statements, ~204 PC comparisons, ~20 out-of-line calls, an unconditional
   guest `mem_read32(0x800000D4)` at `main.c:3763`, three dead write-watch
   stores at `main.c:8491-8492`, `:8955`, a 64-bit modulo at `:10548`) costs
   an estimated 600–1,000 instructions per turn — the same order as the
   dispatch machinery itself. At 92 guest cycles per turn, turn tax is
   plausibly 35–45% of total CPU **[I]**, and it is owned by F1, not by the
   cycle budget.
2. **The GX 12% is frontend CPU shape, not draw count.** The 6,914
   draws/frame is the authentic retail batch count — the frontend provably
   does not split draws (one GX draw opcode → one packet → one plan → one
   `DrawIndexed`; CALL_DL recurses in place). The measured cost is (a)
   `write_fifo` + full `flush` **on every single WGPIPE store**
   (`aurora_graphics.cpp:376-377`), each flush re-entering the parser and
   erasing the consumed prefix of a vector, and (b) ~10 KB of snapshot/copy
   plus two heap allocations per draw (2.3 KB `DrawTransformSnapshot` copied
   ≥3×, full `GxCoreState` copy per draw, per-draw
   `build_topology_indices` `operator new`/`free` visible in the Outset
   sample). Early-depth emulation also issues two GPU draws per plan for
   affected draws. GPU is idle; batching GPU draw calls is *not* the current
   bottleneck — the CPU-side frontend shape is.
3. **DSP share scales inversely with scene cost, and is idle-loop time.** DSP
   work is ~constant per guest second, so its share (11.9% Outset → 30.5%
   Omasao) rises exactly as other work falls. The instruction mix proves the
   interpreter is mostly executing mailbox-poll spins (F5). Prioritizing
   Outset is correct; concluding "DSP is a minority owner in Outset, so skip
   it" is not — the same fix removes a large constant tax from every scene.
4. **Repeated texture hashing is genuinely gone** — the MEM1 dirty-generation
   promotion is verified in the samples. The 1.4% CPU yield against a 99%
   hash-count reduction was itself the tell that hashing was never the frame
   owner; the loop drew the right conclusion there.

Scale check **[I]**: Outset needs ~5× to reach 30 fps. No single item on the
board delivers that. A credible composition: turn-count collapse via
cross-chunk chaining (§7.1) + DSP idle fast-forward + GX frontend flush/copy
repair + host-loop tail cleanup ≈ 2–2.5×; the remaining ~2× must come from
generated-code quality (the `-O1` composite, out-of-line FP/paired-single
helpers, eventually the LLVM backend's PGO/musttail path). Plan for both
stages; neither alone suffices.

---

## 4. Deadline-budget design audit (questions 2–3)

### 4.1 What the candidate does

`host_execution_budget` (`main.c:1367-1397`) computes the nearest deadline
among the VI event clock, DSP update cadence (`DSP_LLE_UPDATE_RATE` −
elapsed), audio-DMA work remaining, and the decrementer, and selects 1,024
cycles only when that distance exceeds 1,024 and no decrementer/external
interrupt is pending; otherwise 256. `host_mmio_read`/`host_mmio_write` clamp
`cycle_budget` back to 256 mid-dispatch (`main.c:1400-1401`, `:1862-1863`).
The generated loop budget becomes a per-back-edge load of
`ctx->cycle_budget` (`scripts/generate_composite.py:454-459`).

Verified sound: the audio-DMA and event-clock deadline units are cycles
(`GXRuntime/src/audio_dma.c`, `event_clock.c:15`); the decrementer distance
math is correct within one tick; treating a pending-but-masked (EE=0)
interrupt as a 256 trigger is conservative and right for exactness;
`cycle_budget` is a real ABI field at offset 3488 with compatible upstream
semantics (`ref/recompcore/DolRecomp/tests/test_module_abi.c:28`).

### 4.2 Concrete hazards (question 2's enumeration)

1. **`mtdec` creates a deadline mid-dispatch with no clamp.**
   `host_spr_write` (`main.c:448-462`) rearms the decrementer from translated
   code but does not touch `cycle_budget`. A guest arming a short decrementer
   inside a 1,024 window can have its expiry observed up to ~768+ cycles
   later than the 256 baseline. Same gap for any non-MMIO path that changes
   deadlines (`ppc_mtspr` from the cold-fallback executor at `main.c:2678`).
2. **Nested dispatches run on a stale budget.**
   `bluewake_deliver_guest_callback` loops `module->dispatch`
   (`runtime/host/src/callback_delivery.c:96-99`) without setting
   `cycle_budget`; callback code inherits whatever the interrupted turn had
   (possibly 1,024 with a deadline now near, possibly a clamped 256). Any
   re-entrant dispatch site needs an explicit budget policy.
3. **The guard window ignores back-edge overshoot.** Budget checks exist only
   at loop back-edges and `return_dispatch` (`emitter.c:387-405`, `:2004`);
   there is no check at chunk entry, and straight-line code can overshoot the
   threshold by an unbounded amount (the fixed probe's final boundary
   overshot by 330 cycles). A deadline at distance 1,025–(1,024+overshoot)
   selects the 1,024 budget and is crossed mid-turn, delivering later than
   the baseline could. The guard would need to be `MAXIMUM + max_overshoot`
   — and max overshoot is not statically bounded.
4. **The fundamental hazard is F2 and is not fixable by guard tuning.** Even
   with 1–3 repaired, timebase reads (`ctx->timebase` is boundary-updated),
   `mfdec` reads, and device-state MMIO reads observe values as of the last
   turn boundary, and boundary *positions* differ between quantizations. The
   OS scheduler reads time constantly; ±2 lazy-FPU switches is precisely the
   expected drift signature. **[I]** The current candidate may pass the
   700-retrace gate if the title route happens not to straddle any sensitive
   boundary — but that is luck, not qualification, and gameplay scenes
   (interrupt-dense, FP-dense) multiply the exposure.
5. Minor: the per-back-edge `cycle_budget` load replaces a compare-against-
   immediate in the hottest check in the system (23,512 sites); measure it,
   and consider caching the budget in a local with MMIO clamps forcing the
   next check via `downcount` instead. The re-entrancy interaction of the
   MMIO clamp with `bluewake_card_runtime_dispatch`'s HLE calls
   (`main.c:3574`) is benign today but undocumented.

### 4.3 The architecturally sound formulation

Promote a **cycle-domain observation and delivery contract** as a default-on
`CPU_RUNTIME` subsystem, *before* qualifying any budget:

1. **Cycle-accurate reads.** `mftb`/`mfdec`/time-bearing MMIO reads flush the
   in-flight `-downcount` into the guest clock (and lazily into any device
   whose state the read observes) so the observed value is a pure function of
   the absolute guest cycle at the reading instruction, independent of turn
   quantization.
2. **Deadline-domain delivery.** Set `cycle_budget = min(cap, distance to
   nearest deadline)` — the *distance*, not a binary 256/1,024 — so a turn
   never spans a deadline, and the event is delivered at the first
   check-site/return whose absolute cycle count ≥ the deadline. That
   instruction position is then identical for **every** cap value.
3. **Mid-turn deadline creation clamps.** Generalize the MMIO clamp to
   `host_spr_write` and every path that can arm a nearer deadline; nested
   dispatch sites set an explicit budget.

Under this contract, quantum choice provably cannot move any observation or
delivery, so: re-qualify the full correctness route **once** at cap 256 (the
digests will shift — this is a one-time re-baseline, exactly like the clock
promotions of 08-22/08-25), then add a permanent public regression that runs
a bounded route at caps 256, 1,024, and dynamic and requires **identical
digests**. Both the dynamic budget and F1's cross-chunk chaining (which is
otherwise unqualifiable for the same reason) then land as pure performance
dials. This converts the candidate's current biggest risk — a full composite
rebuild spent on a coin-flip — into equivalence by construction.

### 4.4 Is the exact 700-retrace gate sufficient? (question 3)

No, on three axes:

- **Identity must be redefined before the run.** "3,670,902,066 blocks" and
  the final cycle boundary change by design under any budget change. Specify
  in advance which counters are invariants (retrace-indexed milestones, guest
  cycles at each milestone, DSP DMA count and first-nonzero hash, ARAM/DVD/GX
  totals, FPU outcome tuple, collision provenance counts, card SHA) and which
  are expected to move (host turns, wall/CPU). A drift in an "expected"
  counter must not be silently reclassified after the fact.
- **The title route under-exercises the hazards** (few interrupts, little FP,
  light GX). Minimum additional tiers before promotion: (a) the exact
  22,500-retrace Outset→Omasao route with the full invariant set — this is
  where lazy-FPU/collision sensitivity actually lives; (b) a
  decrementer/alarm-dense window (the title's `OSSetAlarm` display path
  qualifies if extended); (c) one save/reload cycle on a copied card;
  (d) the 36,500-retrace event-75 tier, since door events were historically
  the most timing-fragile behavior.
- **A pass at 700 retraces without the §4.3 contract proves route
  insensitivity, not equivalence.** Record it as such if taken.

---

## 5. Graphics/hair correctness audit (question 6)

The smallest investigation, in order (all game-data stays private):

1. **Zero-code-change capture + histogram.** On the accepted Omasao view:
   `DOL_AURORA_RECOMP_TRACE_OUT=… DOL_AURORA_RECOMP_TRACE_FRAMES=A:B`, then
   `dolgx_replay trace.dolt --histogram --quiet`. Any `tev_stage_counts`
   entry > 8 or `texgen_counts` > 5 convicts the truncation limits
   immediately; the format tallies simultaneously clear or implicate texture
   decode. Also read the shipping `[gx-core] shutdown:` gap-counter line from
   the same run — `tev_stages_over`, `texgen_count_6/7/8plus`,
   `texgen_source_*`, `dst_alpha_active`, `indirect_ignored` are already
   counted.
2. **Repeatable checkpoint.** `dolgx_replay trace.dolt --core --png-dir …`
   reproduces the defect headless from the trace and is the screenshot
   checkpoint `BW-P4-0063`'s exit criterion needs (public state assertions +
   private PNG/digest).
3. **Draw isolation.** Fix the one-line observer gap (forward the transform
   observer from `GxCoreSink::on_consumed_draw`) so
   `DOL_AURORA_RECOMP_DRAW_TRANSFORM_{FRAME,DRAW,LIMIT}` work on the shipping
   renderer; binary-search the frame's draw index; dump the owning draw's
   material/TEV/texmap state. A draw-skip filter, if needed, belongs in
   `core_plan_observer` (`aurora_graphics.cpp:38-45`).
4. **Verify Rank-1 directly.** For the owning draw, compare the plan's bound
   texture against `tevorders_texmap` of the enabled stage; if they disagree,
   the `bound_texture_`/`texmap_popcount<=1` fast path (F6) is the owner. The
   fix is at the GX contract level (bind by sampled slot), not per-model.

Cause ranking: (1) single-texmap last-written binding bug — a logic error,
not a capacity limit; (2) TEV-stage truncation at 8; (3) texgen truncation at
5 / constant-coordinate source rows; (4) destination-alpha unavailable if
Dawn lacks dual-source blending on this host (check `dst_alpha_active` vs
actual pipeline factors); (5) mip-0-only uploads (shimmer, not geometry);
lower: matrices/skinning, display-list parsing, cull, depth, TLUT, vertex
attributes — all verified faithful ports. Do not open a skinning or DL-parser
investigation unless 1–4 are cleared.

---

## 6. Finish-line gap analysis (question 7)

What "stable macOS game" still requires beyond current evidence:

| Area | State | Gap |
|---|---|---|
| Performance | 6.0 fps Outset median | ~5× composition (§3, §7); then a frame limiter, which does not exist at all |
| Graphics | hair defect + no wider corpus | BW-P4-0063; then a canonical visual corpus with automated digests (none exist — all hashes hand-transcribed) |
| Input | live keyboard path exists, never human-tested; waypoints overwrite it | one unscripted human session; controller (GameController/SDL gamepad) qualification; remapping surface |
| Audio | audible sink wired, active, never human-judged; underrun logged not remediated | human listening pass; underrun/drift handling; note the SDL throttle sleeps on the guest thread |
| Save | works via HLE at hardcoded GZLE01 addresses, single slot | fine for now; brittle beyond rev 0; no card management |
| Product shell | none — 38 env vars, non-double-clickable .app, no UI, no fullscreen | minimal launch/config path before any external tester can run it |
| Endurance | 1h53m scripted route, bounded RSS, clean thermals | unscripted free-play soak; memory growth curve; endurance at *fixed* (higher) speed will change thermals — current "clean" thermal data reflects a one-core-bound slow build |
| Packaging/repro | scripts exist; `HANDOFF.md` is superseded; no current from-scratch runbook; `-O1` composite rebuild wall time undocumented | clean-machine reproduction is explicitly unverified (`FINISH_LINE.md` exit condition) |
| Tests/CI | 18 behavioral CTest binaries; ~57% of Python tests are text tripwires; CI runs no build/test | CI build+test; convert tripwires or label them; register the GPU fixtures |
| Coverage | catalog has 3 records, no gameplay content | P5 campaign catalog is effectively not started |

The honest one-sentence status: BlueWake is a **correct-execution research
vehicle with a working presentation/audio/input substrate that has never been
exercised by a person** — the distance to "game" is one performance
architecture push plus an entire (small but nonzero) product layer.

---

## 7. Ranked opportunities (questions 4–5)

Ranked by expected gain × confidence ÷ (risk + build cost). "Build" notes
whether a full private composite rebuild is required (the dominant cost
quantum; its `-O1` wall time is undocumented — measure and record it).

| # | Opportunity | Expected gain | Risk | Build cost | Falsification cost |
|---|---|---|---|---|---|
| 1 | **Cycle-domain observation/delivery contract** (§4.3) | Enabler for #2/#3; removes a latent observation-staleness incoherence | Medium (one-time re-baseline of all digests) | Composite rebuild (budget macro) + host | One 700-retrace + one 22,500-retrace re-qualification; then the 256/1024/dynamic invariance test is nearly free |
| 2 | **Cross-chunk call chaining** — wire `emit_set_chunk_table`, ensure budget/deadline checks at cross-chunk call sites | Est. 25–45% total CPU **[I]** (removes the majority of 61M turns and their ~1,000-instruction tax) | Medium — code exists and is depth-guarded but has never run; gated on #1 | Full regeneration + composite rebuild | Turn count alone falsifies instantly; digests under #1's contract |
| 3 | **Exact DSP idle fast-forward + halt early-exit** | ~15–25% Omasao, ~8–10% Outset **[I]** | Low — host/adapter only; analyzer already flags the loops; exact-by-construction variant | Host only | Existing DSP digest (DMA count, first-nonzero hash, card) |
| 4 | **GX frontend shape**: flush per GX command boundary instead of per WGPIPE store; eliminate per-draw snapshot copies/allocs (reserve+reuse, index into a ring) | ~5–10% Outset **[I]** | Low-medium; caution — the two allocation-reuse rejections were title-route-scoped and are *not* evidence against Outset-scale wins (F4) | Host only | Outset-scene A/B route (create it first, F4) |
| 5 | **Dynamic execution budget** (current candidate, repaired per §4.2, under #1) | ~5–8% standalone; partially subsumed by #2 | Low once #1 lands | Composite rebuild (shared with #1) | 700-retrace + invariance test |
| 6 | **Host-loop tail cleanup**: dead write-watch stores, 11-register snapshot, unconditional `mem_read32(0x800000D4)`, `blocks % 1000000`, consolidate the ~204 PC guards behind one range check | ~3–5% **[I]** | Low | Host only | Matched A/B |
| 7 | **Chunk-cache key alignment** (750 keys → 64 slots, 332 two-chunk collisions) | ~1–2% | Trivial | Header regen only | Matched A/B |
| 8 | **Generated-code quality**: `-O2` via partition/timeout triage (split the 2h53m units, per-unit fallback to `-O1`), out-of-line FP/psq helper inlining re-adjudicated on the Outset route, per-block FP-availability hoisting | 1.3–2× on guest-code share **[I][S]** | Medium; compile-cost management is the whole problem; FP-inline shape may be reopened only with fresh gameplay-scene evidence per anti-stall rules | Repeated composite rebuilds | Outset A/B + full digests |
| 9 | **LLVM backend revisit** (musttail cross-range calls, threaded budget guards, PGO, partitioning all already exist there) | Architecture-scale (potential 2×+) | High; v27 was rejected 44.2s vs 27.8s — but that predates trace-off, chunk-cache, and the cause (generalized structural admission cost) was never decomposed | New pipeline qualification | Decompose v27's slowdown first (one profile) before any commitment |
| 10 | **Zelda DSP HLE** (in-tree, WW-validated) | Removes DSP cost entirely | Policy: breaks bit-exact DSP digests by construction; violates current acceptance philosophy | Host + donor archive | User-level decision, not an experiment |
| 11 | GPU-layer draw batching / early-depth double-draw reduction | Deferred — GPU is idle | — | — | Revisit when GPU stops being idle |

**Question 5 mapping (Outset 6 fps → playable):**
- **1–5% each:** #6, #7, texture/palette residuals (already done), audio-throttle decoupling.
- **5–15% each:** #3 (Outset), #4, #5.
- **Architecture-scale:** #2 (+#1), #8, #9, #10. The credible path to 30 fps is
  #1→#2 plus #3/#4/#6 (≈2–2.5× combined), then #8 or #9 for the remainder.
  Nothing on the small end of the list composes to 5× — do not spend further
  loop iterations on sub-5% shapes while #1/#2 are open.

Already attempted/rejected — do not re-run without a materially new mechanism:
fixed-quantum sweeps (256→1,024 probe), fourth DSP-helper inline shapes,
decode-table/fetch/exception-guard micro-inlines as standalone experiments,
GX allocation reuse *on the title route* (Outset-route re-test is new
evidence, not a repeat), HLE `data_version` texture versioning, 64-cycle DSP
batching (inexact idle-skip mechanism — #3's exact variant is the new
mechanism), whole-composite `-O2`/`-O3`/LTO without partitioning, LLVM
promotion without decomposing v27's slowdown.

---

## 8. Recommended next actions with stop conditions (question 10)

1. **Preserve evidence (immediate, 30 minutes).** Copy the 8,545 `/tmp`
   artifacts (at minimum every file whose SHA-256 appears in a ledger) into
   `local-research/evidence/`. Stop condition: every ledger SHA resolves to a
   durable file. Repeat finding from 08-25; do not defer again.
2. **Dispose of the in-flight budget candidate deliberately.** If the
   composite rebuild + 700-retrace qualification now in progress completes:
   record pass *or* drift as expected information under F2 (a pass = route
   insensitivity, not equivalence; drift = confirmation of §4). **Do not
   iterate guard windows on failure — that is a closed shape after one
   attempt.** Either way, open the §4.3 contract as the successor blocker.
3. **Land the cycle-domain contract (§4.3)** as one promoted subsystem:
   flush-on-read for TB/DEC/time-MMIO, distance-valued budget, `mtdec` and
   nested-dispatch clamps. Re-baseline the 700-retrace digest, then the
   22,500-retrace digest. Add the public quantum-invariance regression
   (256 vs 1,024 vs dynamic ⇒ identical digests). Stop condition: invariance
   test green. Back up `main`.
4. **DSP idle fast-forward + halt early-exit (#3)** — independent of 2/3,
   host-only, start in parallel. Stop condition: exact DSP digest (DMA count,
   first-nonzero payload hash, card) on 700 and 22,500 routes; then measure
   Outset/Omasao fps. Back up `main`.
5. **Create the Outset performance A/B tier (F4).** One bounded exact
   Outset-scene replay with its own digest, promoted as the standard
   adjudication route for all gameplay-scale performance work. Stop
   condition: two identical back-to-back digests.
6. **Wire the chunk table (#2) as a probe** (falsification tier, like the
   1,024 probe): regenerate with `emit_set_chunk_table` called, verify
   cross-chunk call sites carry budget/deadline checks (add them if
   `emit_cross_chunk_call` lacks them — see §10), measure host-turn count and
   CPU on the title route. Stop condition: if turns do not fall ≥40%, stop
   and re-derive the host-return census dynamically before proceeding.
   Qualify under the §4.3 contract, never against the old block-count
   identity. Back up `main` on acceptance.
7. **Hair isolation (BW-P4-0063), in parallel with any of the above** — it
   touches only capture/replay tooling: §5 steps 1–2 (histogram + PNG
   checkpoint), then step 4 (Rank-1 verification). Stop condition: owning
   draw identified with its full state dumped; fix only at the GX contract
   level; add the replay-based pixel checkpoint as the regression.
8. **One human session.** Launch the signed app with no `BLUEWAKE_PAD_*`
   variables, keyboard only, title → file select → Outset; listen to the
   audio. Record observations (pacing feel, input latency, audio quality) as
   product evidence, explicitly labeled non-digest. This is one hour and
   retires the "no human has ever played it" caveat from every future status
   claim. Do not run concurrently with any other BlueWake process.
9. **Measure and record the `-O1` composite rebuild wall time** the next time
   one runs (it gates all planning above), and fix the ledger habit: every
   new evidence file lands in `local-research/evidence/` directly.
10. **Only after 3+4+6 are adjudicated:** pick between #8 (partitioned `-O2`)
    and #9 (LLVM decomposition profile) from measured remaining ownership on
    the Outset tier — not from the title route.

After each accepted milestone: run the repo audit, update
CURRENT/BLOCKERS/PERFORMANCE with the redefined invariant set, and back up
`main`. Keep exactly one active blocker; items 4, 7, and 8 are the sanctioned
parallel exceptions because they share no files with the contract work.

---

## 9. Evidence reviewed

- **Docs (read in full):** README, PRD, GOAL_LOOP, CURRENT (banner + head +
  authoritative sections), BLOCKERS (active head + structure), PERFORMANCE
  (all 2026-08-29/30 entries), FINISH_LINE, DECISIONS, GATES,
  REORIENTATION_2026-08-22, INDEPENDENT_REVIEW_2026-08-25, COMPATIBILITY,
  RELEASE, DSP_INTEGRATION_2026-08-25, PORTING_HISTORY (structure + recent).
- **Code:** `runtime/host/src/main.c` (dispatch loop, clocks, SPR/MMIO paths,
  budget integration), `execution_budget.{c,h}`, `callback_delivery.c`,
  `card_runtime.c`, `pad_wire.c`, `scripts/generate_composite.py`, the
  uncommitted diff, `tests/execution_budget_test.c`,
  `tests/test_cpu_abi_contract.py`; DolRecomp C backend
  (`emitter.c`, `c_cfg.c`, `pipeline.c`, `dispatch.c`) and LLVM backend
  (`branch_targets.cpp`, `exits.cpp`, `cli.c`); GXRuntime frontend/gxcore
  (`retail_gx_frontend.cpp`, `render_sink.cpp`, `gxcore.cpp`,
  `gxcore_shader.cpp`, `gxcore_draw.cpp`, `aurora_graphics.cpp`,
  `aurora_input.cpp`, `aurora_audio.cpp`, `audio_dma.c`, `event_clock.c`);
  donor DSP (`DSPInterpreter.cpp`, `DSPAnalyzer.cpp`, `DSPCore.h`,
  `DSPIntTables.cpp`, `dsp_adapter.cpp`, DSPHLE tree).
- **Generated composite (static census, 748 chunks, 17.9M lines):** call-site
  and return-site counts as tabulated in §2 F1; chunk alignment/cache-key
  analysis; `_selected_dispatch` and representative `func_*` ARM64
  disassembly from the shipping dylib.
- **Native samples (primary evidence for §3):**
  `/tmp/bluewake-shipping-outset2-20260830.sample.txt` and
  `…omasao2…` (the runs backing the current banner), including the DSP
  opcode-mix subtree and the GX frontend/malloc subtrees.
- **Ledger-cited measurements** used as given (not re-run): all matched A/B
  CPU/instruction/cycle figures, the fixed-1,024 probe deltas, the `-O2`
  build abort, fps telemetry.

Verification method note: four independent read-only audit passes (dispatch
economics, GX/hair, DSP, product surface) were cross-checked against my own
direct reading of the budget candidate, clock/SPR paths, samples, and
ledgers; where an audit claim conflicted with the ledgers I re-read the
source before including it. One correction applied: the dynamic budget is
**uncommitted and unqualified**, not shipping — any statement treating it as
the current shipping cadence is wrong.

---

## 10. Unknowns and requests for stronger evidence

1. **Does `emit_cross_chunk_call` emit budget/deadline checks at call sites?**
   The depth guard is confirmed; the budget interaction is not. Read
   `emitter.c:355-376` against the §4.3 contract before the #2 probe; if
   absent, checks must be added or turns can span deadlines unboundedly.
2. **Dynamic host-return census.** The 60–80% cross-chunk-call share is a
   static-census inference. One cheap counter build (bucket returns by cause:
   cross-chunk call / return / bctr / budget / exception) on the Outset tier
   would convert it to fact and set exact expectations for #2.
3. **DSP idle fraction.** The opcode mix implies "most" of DSP time is idle
   spin; the exact fraction (and thus #3's ceiling) needs one counter:
   cycles retired inside analyzer-flagged idle PCs vs total.
4. **Dual-source blending support on this host.** `dst_alpha_active` counter
   vs actual pipeline blend factors — one shutdown-line read. If unsupported,
   WW's dst-alpha rendering is silently wrong everywhere, not just hair.
5. **`-O1` composite rebuild wall time** — undocumented, yet it is the unit
   of cost for half the plan. Record it on the in-flight rebuild.
6. **LLVM v27 slowdown decomposition** — 44.2s vs 27.8s was recorded without
   attribution (structural-admission cost? missing chunk cache? dispatch
   shape?). One profile of the v27 artifact would decide whether #9 is a
   dead end or the end-state architecture.
7. **TB read emission.** I did not locate the exact emitted form of
   `mftb` in generated code (direct `ctx->timebase` load vs helper). The
   §4.3 flush-on-read design depends on where to interpose; confirm in
   `emitter.c` before implementing.
8. **Whether the in-flight composite rebuild embeds the per-back-edge budget
   load** in REL objects as well as DOL, and its measured cost — the ABI
   tripwire in `test_cpu_abi_contract.py` was updated, but no A/B isolates
   the load itself.
9. **Audio throttle coupling.** The SDL push path can sleep on the guest
   thread (`aurora_audio.cpp:55-68`); at current frame rates the queue rarely
   fills, but after performance work this becomes a real pacing interaction.
   Needs one measurement once fps rises.
10. **Human-session unknowns** — input latency, camera feel, audio quality,
    pacing perception: no digest can answer these; only action §8.8 can.
