# PROGRESS — Why higher render settings drop the Deck below 30 fps

Status: In progress. Measured paced with the owner's build, settings and save state: the GPU fills at 4x and with the HD look. Earlier unpaced conclusions are retracted (see PRD). A locked 1600 MHz GPU is not a fix. Turning each part off at 4x + HD: Smooth Motion +64 %, cloud shadows +14.5 %, AO +7.5 %. Next: cheaper cloud shadows (opt-in), then the in-between frame.
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

## Evidence ledger

| Task | Artifact | Method | Environment | Result |
|---|---|---|---|---|
| M1-T01 | owner's session log `session-20261109-212210.log`, hwmon samples | Read-only SSH: `[perf]` lines, `gpu_busy_percent`, `freq1_input`, slowPPT | Steam Deck Game Mode, stack `afb6c35d8805`, owner's settings, Outset | 3x 30 fps / GPU ~60 % / 810 MHz / 8.4 W; 4x 26–28 / ~97 % / ~1200 MHz / 10.3 W; 3x+HD 25.5–29 / ~95 % / ~1300 MHz / 11–18 W; 4x+HD 19–20 / ~95 % / ~1480 MHz / 13.8 W |
| M2-T03, M2-T04 | owner's session log `session-20261109-222400.log`, hwmon samples | Read-only SSH; segments split by the log's menu open/close and settings-saved lines | GPU clock locked at 1600 MHz in Steam, same build and settings | 3x 29.9 fps / game thread 86 % / GPU 50 % / 15.4 W; 4x 27.4 / 98 % / 75 % / 14.2 W; 3x+HD 28.9 / 98 % / 65 % / 13.9 W; 4x+HD 18.8 / 80 % / 77 % / 13.4 W, `[fps-dip]` waits present=255–282 ms/s |
| M2-T05 | `render-ablate/report.md` | render_bench `--paced --every 0`, 7 setups x 2 launches, each turning one part off | Owner's state, 4x + HD, Game Mode build and settings | 18.25 baseline; Smooth Motion off 29.96; cloud shadows off 20.91; AO off 19.62; bloom off 18.74; far blur off 18.69; edge light off 18.62 |
| M1-T03, M1-T04, M2-T01 | `render-paced/report.md`, samplers | render_bench `--paced`, A/B/C/D interleaved, 2 pairs, no warm-up; samplers tagged by launch settings | Same build; state `quick-1835` (Outset, retrace 1835); 16:9 widescreen + BetterWW (`mask=0x3`), gate and huge pages on | FPS 29.0 / 25.8 / 24.5 / 18.2; game thread 88 / 82 / 79 / 69 %; GPU 85 / 89 / 89 / 93 % at 553 / 913 / 938 / 1266 MHz; 16/16 launches loaded; HD-on checkpoints differ from HD-off (the HD look's culling) |

## Decisions

| Date | Decision |
| --- | --- |
| 2026-11-09 | Measure render settings paced, with the owner's build, settings and save state. Unpaced runs hide a full GPU because the game thread pins the chip at its power limit |
| 2026-11-09 | Retract the unpaced results: power-sharing cause, 1100 MHz cap confirmation, scale 2 over 3, the M6 recheck, and #53's rejection (to be retested). Keep the identical-draw count |
| 2026-11-10 | `[gpu-prof]` per-pass numbers are not comparable on the Deck (double-counted HD zones, leaking encoder timestamps); cost each part by turning it off instead. First code target: cloud shadows (14.5 % at 4x + HD), opt-in in the Performance section |
| 2026-11-09 | A locked 1600 MHz GPU moves the limit to the game thread (shared power) and does not help 4x + HD: drop L3 as a fix. Add L4, the present stall at 4x + HD |
| 2026-11-09 | Next: L1, why in-between passes cost 2–4x the real frame's; then L4 (present stall) and L2 (#53 paced) |
| 2026-11-05 | Started as a follow-up to #524: the render scale and the HD look slow the game on the Deck |
