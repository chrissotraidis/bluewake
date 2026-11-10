# PROGRESS — Why higher render settings drop the Deck below 30 fps

Status: In progress. Measured paced with the owner's build, settings and save state: the GPU fills at 4x and with the HD look. Earlier unpaced conclusions are retracted (see PRD). Next: L1, in-between pass cost.
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md)

## Milestones

### M1: Measure as played
- [x] M1-T01: Profile the owner's real play over SSH (session `[perf]`/`[fps-dip]` lines, GPU busy, clock, power): 3x holds 30 at ~60 % GPU busy; 4x 26–28 fps at ~97 % busy; 3x + HD 25.5–29; 4x + HD 19–20 with in-between frames skipped; the game thread waits in `acquire-drawable` 30–42 ms
- [x] M1-T02: render_bench `--paced` and copy-once for identical AppImages (uncommitted in the #524 worktree's `scripts/`; to land in its own PR)
- [x] M1-T03: Paced bench from the owner's save state with the Game Mode build and settings, 4 launches per setting: 29.0 / 25.8 / 24.5 / 18.2 fps for 3x / 4x / 3x+HD / 4x+HD, each setting's launches within ±1.5 %
- [x] M1-T04: Power does not set the GPU clock: GPU ≥ 95 % busy runs at about the same clock below 12 W as at 15 W

### M2: Attribute the GPU time
- [x] M2-T01: Per-pass `[gpu-prof]` for the four settings: in-between replay 5.3–11.9 ms, staging copies 5.6–9.0 ms; in-between EFB copy 2, cloud shadows and ambient occlusion cost 2–4x the real frame's
- [ ] M2-T02: L1 — source reading: why in-between passes cost more (different path, redone work, or profiler attribution)
- [ ] M2-T03: L3 — paced run with the GPU clock at 1600 MHz (needs the owner to set Steam's slider)

### M3: Source experiments
- [ ] M3-T01: L1 fix, if the reading finds avoidable work; opt-in, picture identical, judged paced
- [ ] M3-T02: L2 — retest RecompCore #53 staging-copy batching paced with these settings

## Evidence ledger

| Task | Artifact | Method | Environment | Result |
|---|---|---|---|---|
| M1-T01 | owner's session log `session-20261109-212210.log`, hwmon samples | Read-only SSH: `[perf]` lines, `gpu_busy_percent`, `freq1_input`, slowPPT | Steam Deck Game Mode, stack `afb6c35d8805`, owner's settings, Outset | 3x 30 fps / GPU ~60 % / 810 MHz / 8.4 W; 4x 26–28 / ~97 % / ~1200 MHz / 10.3 W; 3x+HD 25.5–29 / ~95 % / ~1300 MHz / 11–18 W; 4x+HD 19–20 / ~95 % / ~1480 MHz / 13.8 W |
| M1-T03, M1-T04, M2-T01 | `render-paced/report.md`, samplers | render_bench `--paced`, A/B/C/D interleaved, 2 pairs, no warm-up; samplers tagged by launch settings | Same build; state `quick-1835` (Outset, retrace 1835); 16:9 widescreen + BetterWW (`mask=0x3`), gate and huge pages on | FPS 29.0 / 25.8 / 24.5 / 18.2; game thread 88 / 82 / 79 / 69 %; GPU 85 / 89 / 89 / 93 % at 553 / 913 / 938 / 1266 MHz; 16/16 launches loaded; HD-on checkpoints differ from HD-off (the HD look's culling) |

## Decisions

| Date | Decision |
| --- | --- |
| 2026-11-09 | Measure render settings paced, with the owner's build, settings and save state. Unpaced runs hide a full GPU because the game thread pins the chip at its power limit |
| 2026-11-09 | Retract the unpaced results: power-sharing cause, 1100 MHz cap confirmation, scale 2 over 3, the M6 recheck, and #53's rejection (to be retested). Keep the identical-draw count |
| 2026-11-09 | Next: L1, why in-between passes cost 2–4x the real frame's; then L2 (#53 paced) and L3 (clock at 1600 MHz) |
| 2026-11-05 | Started as a follow-up to #524: the render scale and the HD look slow the game on the Deck |
