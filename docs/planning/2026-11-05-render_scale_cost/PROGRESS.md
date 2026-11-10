# PROGRESS — Why higher render settings drop the Deck below 30 fps

Status: Goal reached at Outset in the bench: 3x + full HD look + Smooth Motion holds 30 with faster clouds, faster occlusion and Mailbox presentation (24.9 → 30.0). Next: the five scenes, and a play test that in-between frames reach the screen under Mailbox.
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md)

## Milestones

### M1: Measure as played
- [x] M1-T01: Profile the owner's real play over SSH (session `[perf]`/`[fps-dip]` lines, GPU busy, clock, power): 3x holds 30 at ~60 % GPU busy; 4x 26–28 fps at ~97 % busy; 3x + HD 25.5–29; 4x + HD 19–20 with in-between frames skipped; the game thread waits in `acquire-drawable` 30–42 ms
- [x] M1-T02: render_bench `--paced` and copy-once for identical AppImages (uncommitted in the #524 worktree's `scripts/`; to land in its own PR)
- [x] M1-T03: Paced bench from the owner's save state with the Game Mode build and settings, 4 launches per setting: 29.0 / 25.8 / 24.5 / 18.2 fps for 3x / 4x / 3x+HD / 4x+HD, each setting's launches within ±1.5 %
- [x] M1-T04: Power does not set the GPU clock: GPU ≥ 95 % busy runs at about the same clock below 12 W as at 15 W

### M2: Attribute the GPU time
- [x] M2-T01: Per-pass `[gpu-prof]` for the four settings: in-between replay 5.3–11.9 ms, staging copies 5.6–9.0 ms; in-between EFB copy 2, cloud shadows and ambient occlusion cost 2–4x the real frame's
- [x] M2-T02: L1 — closed as a measuring error. `[gpu-prof]` counts each HD effect twice on the in-between frame (a zone and a pass with the same label, [post_fx.cpp](../../../ref/recompcore/GXRuntime/graphics/aurora/lib/gfx/post_fx.cpp) `encode`), and encoder-level zone timestamps leak time between neighbours. Frame totals hold: at 3x the real frame is ~17 ms and the in-between frame ~10.5 ms of GPU time
- [x] M2-T05: Cost of each part by turning it off, paced, owner's state at 4x + HD, 2 launches each: Smooth Motion off 29.96 fps (+64 %), cloud shadows off +14.5 %, ambient occlusion off +7.5 %, bloom +2.6 %, far blur +2.4 %, edge light +2.0 % (baseline 18.25)
- [x] M2-T06: Cloud shadows shader: 4 octaves of gradient noise per pixel, each 4 hashed lattice gradients with `cos`/`sin` (~32 trig calls per pixel, at 5.4 MP twice a game frame at 4x). An exact early exit when `smoothstep(0.42, 0.6, n)` is already decided saves only ~8 % of octaves (simulated: the noise sits in that band on 54 % of the screen)
- [x] M2-T03: L3 — owner's play with the GPU locked at 1600 MHz: 3x 29.9, 4x 27.4, 3x + HD 28.9, 4x + HD 18.8 fps. 4x and 3x + HD gain 1–3 fps but the game thread becomes the limit (98 % busy, chip at 14–15 W); 4x + HD does not gain. Clock is not a fix
- [ ] M2-T04: L4 — 4x + HD stalls in present: 255–282 ms a second waiting, `acquire-drawable` 30–65 ms, with the GPU 77 % and the game thread 80 % busy. Find what holds the frames (gamescope, swap-chain depth, Smooth Motion pacing)

### M3: Source experiments
- [ ] M3-T00: E2 — cheaper cloud shadows, opt-in "Faster cloud shadows" in the Performance section: baked 4096² noise texture (owner's choice over exact shortcuts worth ~1 %). Code: [RecompCore#54](https://github.com/chrissotraidis/RecompCore/pull/54) (`b48524af`), switch and pin: [#586](https://github.com/chrissotraidis/bluewake/pull/586) on the #554 stack. Full stack build running; then paced Deck A/B (switch on vs off, same build, 6+ launches) and pictures
- [ ] M3-T01: L1 fix, if the reading finds avoidable work; opt-in, picture identical, judged paced
- [ ] M3-T02: L2 — retest RecompCore #53 staging-copy batching paced with these settings
- [ ] M3-T03: B1–B3 — count pass cuts and full-screen passes at 4x + HD (`DOL_AURORA_GPU_PROF=2`), then fuse/merge where exact
- [ ] M3-T04: S2 — measure the off-screen share of the in-between frame by turning those passes off
- [ ] M3-T05: S4 — reprojection prototype (opt-in), only option likely to hold 60 at 4x + HD
- [ ] M3-T06: E3 — ambient occlusion cost

### M4: 3x + full HD look holds 30 (goal)
- [x] M4-T07: Five scenes, kept switches vs all off, 6 launches each: clouds Outset +9.3 % (every launch), butterfly +5.4 %, lava/shipwreck no cloud pass (no bake: spread only), Hyrule held at 30; AO +1.8–2.6 % in every scene not held at 30. Overall +2.3 % / +1.6 %
- [x] M4-T01: Cost of each part at 3x + HD, turned off, paced, owner's state, 2 launches each (baseline 24.39): Smooth Motion off 29.92 (+22.7 %), cloud shadows +7.9 %, ambient occlusion +6.5 %, far blur +5.2 %, edge light +1.4 %, bloom +1.1 %. The effects together ≈ the whole gap; far blur matters more at 3x than at 4x
- [x] M4-T02: Step 1 — faster cloud shadows (`DOL_AURORA_CLOUD_NOISE_TEXTURE`): **+9.9 %** (27.36 vs 24.90 fps), every launch ahead (26.84–27.99 vs 24.72–25.22); picture min PSNR 67.8 dB with Smooth Motion off. **Kept** in #586 / RecompCore#54
- [x] M4-T03: Step 2 — faster ambient occlusion (`DOL_AURORA_AO_PACKED`): **+1.9 %** (25.36), every launch ahead (25.23–25.71); picture min PSNR 70.5 dB. **Kept**
- [x] M4-T04: Step 3a — skip effect pixels that change nothing: +0.4 %, one launch below the reference; picture identical. **Removed** (RecompCore `066e3327` reverts, #586 `f6e2bfbf3`)
- [x] M4-T08: What remains with clouds + AO on (2 launches each, rough): far blur off +3.9 %, edge light off +2.9 %, bloom off +1.8 %, far blur's sky check off +0.1 % (not its cost). HUD pass 4 (draw log) is small blended quads, not a copy-back: no copy to elide
- [x] M4-T04b: Step 3b as batching — the full-size effects in one shared render pass (`DOL_AURORA_POST_SHARED_PASS`): identical picture (frame-exact), but +0.76 % and not ahead in every launch. **Removed** (RecompCore `78232962`, #586 `b4f0b4e05`). With 3a's result, per-pass and per-write overhead are not where the effects' cost is; the copy-and-combine 3b would not pay either
- [x] M4-T09: L2 retest — RecompCore #53 staging-copy batching, paced with the player's settings, clouds + AO on both sides: run 1 +2.6 % (5 of 6 pairs), run 2 −1.6 %; picture identical. Run-to-run spread: **out again** (RecompCore `85f2c3ea` reverts, #586 `0100157a2`). The all-off reference itself moved 0.5 fps between sessions, so ~2 % is the noise floor for a 6-launch A/B
- [x] M4-T10: Smooth Motion reuses the real frame's soft blurs: −0.17 % over 12 launches each; GPU time fell (18.0 vs 18.6 ms) but the clock governor lowered the clock instead. **Removed** (#586 `5f12ddaa7`)
- [x] M4-T11: Finding — the GPU sits at ~91–93 % busy in every setup while the governor moves the clock (all off 1018 MHz → clouds 944 → all three 860 MHz); the game thread is ~85 % busy and the game runs at ~27. Neither side is full: the remaining limit is the pipeline (Fifo, 3 swap-chain images, two presents per game frame with Smooth Motion; `acquire-drawable` waits of 30–40 ms and present waits up to ~150 ms a second in `[fps-dip]`). Small GPU savings now turn into a lower clock, not frames
- [x] M4-T12: Present mode — Mailbox vs Fifo, clouds + AO on: **29.99 vs 27.28 (+9.9 %)**, every launch at 30, game thread 95.6 % busy, `[fps-dip]` 1 vs 26. Kept as "Present without waiting for the display" (`BLUEWAKE_PRESENT_MAILBOX`, #586); in-between frames on screen still to confirm in a play test
- [x] M4-T13: Five scenes, clouds + AO + Mailbox vs today's defaults, 6 launches each: **+10.1 %** overall (26.41 → 29.08); Outset bench state +15.4 %, lava +8.9 % (holds 30), butterfly +14.6 %, shipwreck +12.6 %, every launch ahead; Hyrule already at 30. The bench's Outset state is now game-thread bound (~98 %)
- [x] M4-T14: At 4x + HD, five scenes, the three switches vs none: **+21.3 %** (19.48 → 23.630); Outset +16.4 %, lava +21.4 %, Hyrule +31.9 %, butterfly +23.6 %, shipwreck +14.5 %. Still short of 30: the docked target is the next round
- [ ] M4-T06: Play test — the owner plays with the three switches on; confirm in-between frames reach the screen under Mailbox
- [x] M4-T05: Step 4 — closed by measurement before building. `DOL_AURORA_PASS_TRACE=1` at 3x + HD (owner's state): pass 0 (31 flat draws) and pass 1 (17 flat draws) end in 1024×768 copies; pass 2 is the scene (1,026 draws) with a full-size depth copy; pass 3 copies the finished scene; pass 4 is the HUD drawing that copy back. The only reusable off-screen work is passes 0–1, which are cheap; the full-size copies must be redrawn or the in-between frame would show the real frame. Reprojection (S4) stays the lever for 4x / docked. Confirmed in all five bench scenes at 3x + HD: Outset 4 small passes (56 flat draws), lava room 1 (31), Hyrule Castle 1 (31), butterfly room 4 (56), shipwreck 1 (31), each into 1024×768; scene passes 254–1,831 draws; no large reflection or haze passes anywhere
- [ ] M4-T06: Check against the goal: 3x + HD + Smooth Motion ≥ 30 fps paced, and in the owner's play

## Evidence ledger

| Task | Artifact | Method | Environment | Result |
|---|---|---|---|---|
| M1-T01 | owner's session log `session-20261109-212210.log`, hwmon samples | Read-only SSH: `[perf]` lines, `gpu_busy_percent`, `freq1_input`, slowPPT | Steam Deck Game Mode, stack `afb6c35d8805`, owner's settings, Outset | 3x 30 fps / GPU ~60 % / 810 MHz / 8.4 W; 4x 26–28 / ~97 % / ~1200 MHz / 10.3 W; 3x+HD 25.5–29 / ~95 % / ~1300 MHz / 11–18 W; 4x+HD 19–20 / ~95 % / ~1480 MHz / 13.8 W |
| M2-T03, M2-T04 | owner's session log `session-20261109-222400.log`, hwmon samples | Read-only SSH; segments split by the log's menu open/close and settings-saved lines | GPU clock locked at 1600 MHz in Steam, same build and settings | 3x 29.9 fps / game thread 86 % / GPU 50 % / 15.4 W; 4x 27.4 / 98 % / 75 % / 14.2 W; 3x+HD 28.9 / 98 % / 65 % / 13.9 W; 4x+HD 18.8 / 80 % / 77 % / 13.4 W, `[fps-dip]` waits present=255–282 ms/s |
| M4-T01 | `render-ablate3/report.md` | Same as M2-T05 at 3x + HD | Owner's state, Game Mode build and settings | 24.39 baseline; Smooth Motion off 29.92; cloud shadows off 26.31; AO off 25.98; far blur off 25.66; edge light off 24.73; bloom off 24.67 |
| M4-T02–T04 | `render-steps/report.md`, `render-pictures/report.md` | render_bench `--paced`, 5 setups × 6 interleaved launches, build `164433578b04`; pictures in a separate run with Smooth Motion off (frame-exact) | Owner's state, 3x + HD, Game Mode settings | All off 24.90; clouds 27.36; AO 25.36; skip 25.00; all three 27.02. Pictures vs all off: clouds 67.8 dB, AO 70.5 dB, skip identical. Checkpoints identical |
| M4-T14 | `render-five-all4/report.md` | As M4-T13 at 4x | 4x + HD + Smooth Motion | 19.48 → 23.630 overall |
| M4-T13 | `render-five-all/report.md` | All three switches vs none, 6 launches each, five states | 3x + HD + Smooth Motion, build `e4782ae070f9` | 29.08 vs 26.41 overall; per scene on #586 |
| M4-T12 | `render-vsync/report.md` | `BLUEWAKE_VSYNC=0` (Mailbox) vs default (Fifo), clouds + AO on, 6 launches each | Owner's state, 3x + HD + Smooth Motion | 29.99 vs 27.28; Mailbox launches 29.97–30.06 |
| M4-T09 | `render-batch*/report.md`, `render-batch-pic/report.md` | Batching on vs off, clouds + AO on, 2 runs × 6 launches; pictures Smooth Motion off | Owner's state, 3x + HD | Run 1 27.47 vs 26.77; run 2 26.81 vs 27.24; pictures identical |
| M4-T04b | `render-shared/report.md`, `render-shared-pic/report.md` | Shared pass on vs off, clouds + AO on both, 6 launches each; pictures with Smooth Motion off | Owner's state, 3x + HD | 27.01 vs 26.81 (+0.76 %), one pair 26.35 vs 27.53; pictures identical |
| M4-T07 | `render-five/report.md` | render_bench `--paced --every 0`, 3 setups × 6 launches, five states | 3x + HD, Game Mode settings, build `164433578b04` | Off 26.66, clouds 27.27, AO 27.07 overall; per scene in the table on #586 |
| M4-T05 | `render-passtrace` log | One paced launch, `DOL_AURORA_PASS_TRACE=1`, 3x + HD | Owner's state, Game Mode build and settings | 5 passes: 0 (31 flat, copy 1024×768), 1 (17 flat, copy 1024×768), 2 (1,026 draws, depth copy 2560×1440), 3 (0 draws, colour copy 2560×1440), 4 (121 flat, on screen) |
| M2-T05 | `render-ablate/report.md` | render_bench `--paced --every 0`, 7 setups x 2 launches, each turning one part off | Owner's state, 4x + HD, Game Mode build and settings | 18.25 baseline; Smooth Motion off 29.96; cloud shadows off 20.91; AO off 19.62; bloom off 18.74; far blur off 18.69; edge light off 18.62 |
| M1-T03, M1-T04, M2-T01 | `render-paced/report.md`, samplers | render_bench `--paced`, A/B/C/D interleaved, 2 pairs, no warm-up; samplers tagged by launch settings | Same build; state `quick-1835` (Outset, retrace 1835); 16:9 widescreen + BetterWW (`mask=0x3`), gate and huge pages on | FPS 29.0 / 25.8 / 24.5 / 18.2; game thread 88 / 82 / 79 / 69 %; GPU 85 / 89 / 89 / 93 % at 553 / 913 / 938 / 1266 MHz; 16/16 launches loaded; HD-on checkpoints differ from HD-off (the HD look's culling) |

## Decisions

| Date | Decision |
| --- | --- |
| 2026-11-09 | Measure render settings paced, with the owner's build, settings and save state. Unpaced runs hide a full GPU because the game thread pins the chip at its power limit |
| 2026-11-09 | Retract the unpaced results: power-sharing cause, 1100 MHz cap confirmation, scale 2 over 3, the M6 recheck, and #53's rejection (to be retested). Keep the identical-draw count |
| 2026-11-10 | Next lever is pipelining, not effect cost: with the GPU ~92 % busy at a governor-chosen clock and the game thread ~85 %, presentation and frames in flight decide the rate. Test present mode and queue depth before more effect work |
| 2026-11-10 | #53's staging batching stays out after a paced retest (+2.6 % then −1.6 %). With ~2 % run-to-run spread, small candidates get 12 launches per side |
| 2026-11-10 | Keep faster cloud shadows (+9.9 %) and faster ambient occlusion (+1.9 %); remove the skip switch (+0.4 %, not ahead in every launch, though bit-identical). Pictures with Smooth Motion on are not frame-exact; check them with it off. Next: confirm across the five scenes and measure each effect's cost per scene |
| 2026-11-10 | Owner: one PR per workstream (#586, RecompCore #54), 3b only if the measurements call for it, 4 as an opt-in experiment. The pass trace then showed 4 has almost nothing to reuse at Outset, so it is closed without a build |
| 2026-11-10 | Goal set by the owner: full HD look at 3x holds 30 with Smooth Motion. Plan: cloud shadows, ambient occlusion, one pass for the per-pixel effects, then reuse of off-screen copies in the in-between frame. 4x is overkill on the Deck's screen (smoothing, not detail) and comes after, for docked play |
| 2026-11-10 | `[gpu-prof]` per-pass numbers are not comparable on the Deck (double-counted HD zones, leaking encoder timestamps); cost each part by turning it off instead. First code target: cloud shadows (14.5 % at 4x + HD), opt-in in the Performance section |
| 2026-11-09 | A locked 1600 MHz GPU moves the limit to the game thread (shared power) and does not help 4x + HD: drop L3 as a fix. Add L4, the present stall at 4x + HD |
| 2026-11-09 | Next: L1, why in-between passes cost 2–4x the real frame's; then L4 (present stall) and L2 (#53 paced) |
| 2026-11-05 | Started as a follow-up to #524: the render scale and the HD look slow the game on the Deck |
