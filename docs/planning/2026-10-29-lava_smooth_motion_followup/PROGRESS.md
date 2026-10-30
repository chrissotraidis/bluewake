# PROGRESS — Lava Room Smooth Motion follow-up

Status: In progress (M0 done; M1 code in RecompCore #49)  
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md)

## Milestones

### M0: Reproduce and freeze the Lava Room control

Architecture: [graphics synchronization](../../architecture/Graphics.md#synchronization-and-completion).

- [x] M0-T01: Record #501/RecompCore/AppImage hashes, Deck mode, display rate, renderer and Smooth Motion settings.
- [x] M0-T02: Capture at least five fixed windows each with automatic pacing, never-pause pacing and Smooth Motion off.
- [x] M0-T03: Record game/display FPS, GPU busy/clock, present/slot/staging waits and current pacing logs.

### M1: Separate pacing, verdict and overload causes (depends: M0)

RecompCore counters/tests and BlueWake log parsing may proceed in parallel and integrate before M2.

- [x] M1-T01: Add typed overload cause and wait-duration accounting without changing policy.
- [x] M1-T02: Count delivered, budget-skipped and content-verdict-rejected frames separately.
- [x] M1-T05: Time the in-between replay in `gpu_prof` (today only the real frame is profiled).
- [x] M1-T03: Extend frame-interpolation tests for cause classification, drop threshold and recovery.
- [x] M1-T04: Select one cause only when the fixed-window totals reconcile.

M0 finding: in this view automatic pacing is correct protection, not a false overload. Keeping the in-between frames (never pause) costs about 10% of game speed with the GPU clocked up, while automatic pacing holds full speed by pausing. Each retry drops again within about 0.25 s ("a drawable came late", "the game waited for the render worker"). M1 therefore separates GPU time from presentation timing for those late drawables, and the likely M2 is a cheaper in-between frame rather than a pacing-rule change.

### M2: Correct the proven cause (depends: M1)

- [ ] M2-T01: Change one pacing, presentation or GPU-resource behavior selected by M1.
- [ ] M2-T02: Preserve camera-cut/content rejection and genuine-overload game-speed protection.
- [ ] M2-T03: Produce matched control/candidate builds with explicit dependency identities.

### M3: Qualify and deliver (depends: M2)

- [ ] M3-T01: Run fixed-view Lava Room A/B with Smooth Motion on and forced-overload protection checks.
- [ ] M3-T02: Run the full rendered scene tour, pictures/checkpoints and Linux/Windows CI.
- [ ] M3-T03: Open the RecompCore change, pin/export it in BlueWake and record retain/reject evidence.

## Evidence ledger

| Task | Commit / artifact | Command / method | Environment / input | Result |
|---|---|---|---|---|
| M0-T01 | AppImage `6fe3d1a7868b` (#501 alias-lookup build, RecompCore `2e4e599`) | `/proc/<pid>/environ`, launcher script, `pp_dpm_sclk` | Steam Deck LCD, Game Mode, 60 Hz panel, 8 threads; Smooth Motion 60 Hz; never-pause via `DOL_AURORA_FRAME_INTERP_PACING=0`, automatic via the same launcher without it | Identities and settings recorded |
| M0-T02 | Local samples `off.txt`, `never.txt`, `auto.txt` (private, not committed) | 5 × 10 s windows per mode, same Lava Room (`SubD51` room 1) view, camera still | Same build and launch path, modes captured back to back | Off: game ≈100% (one entry hitch), GPU 69% at 1040 MHz, CPU 142%. Never pause: game 74–98% (mostly 84–92%), nearly all frames interpolated, GPU 96% at up to 1.5 GHz, CPU 151%. Automatic: game ≈100%, interpolation paused nearly throughout, GPU 66%, CPU 145% |
| M0-T03 | Same samples plus `[fps-dip]` and `[interp-pace]` session-log lines | Per-second sysfs GPU busy/clock, process CPU, fps-watch waits | Same windows | Content rejected 0.2–0.3% in every mode; gx wait ≈5 ms in every mode; present wait 6–11 ms/s off, 6–29 ms/s automatic, 10–156 ms/s never pause. Automatic retried 3 times, dropping again within 0.25 s each time (late drawable twice, render-worker wait once; calm 12 → 24 → 30 s) |
| M0-T03 | Deck `~/bluewake-render/runs/gpu-075439/` (private) | `rendered_run.py` headless 1280×800, `--state 2-lava-room --gpu-prof --max-retraces 138013 --repeat 3`; off vs `--smooth 1` + `DOL_AURORA_FRAME_INTERP_PACING=0` | Same AppImage, gamescope headless | Off: 5.26 ms GPU busy/frame (EFB 4.64), frame 5.3 ms avg, 7–11 ms max. Never pause: 3.4–4.3 ms per profiled frame (in-between replay not profiled), present gaps 27–29 ms avg and 362–498 ms max, present wait 41–87 ms/s. Reproduces the Game Mode cost headless |
| M1-T01 | RecompCore [#49](https://github.com/chrissotraidis/RecompCore/pull/49) `45d0c05`; pinned in `0b5c60c` | Source review; `[interp-pace]` reasons unchanged in test output | Linux CI container (clang-18, libc++) | `note_overload(OverloadCause, waitMs)` at all four producers; pacing unchanged |
| M1-T02 | Same | `frame_interp::stats()`, `DOL_AURORA_FRAME_INTERP_STATS=1` → `[interp-stats]` each second | Same | Delivered/skipped/cut/rejected/idle/unstaged per presenting frame, drops and raises |
| M1-T03 | Same; Linux CI registration `4fbe8cf` | `bluewake_frame_interp_test` | Same | All checks pass, including cause counts and waits, 3-of-8 drop, skipped/cut/unstaged and raise after calm |
| M1-T05 | Same | `[gpu-prof]` frame plus `In-between replay` zone on both present paths | Source review; Deck run pending | Replay cost becomes visible; measurement follows with the #504 AppImage |
| M1-T05 | AppImage `4fc7e7988e1d` built from #504 `0b5c60c` (RecompCore #49 `45d0c05`); Deck `~/bluewake-render/runs/diag-085510/` (private) | `rendered_run.py` headless, `--state 2-lava-room --smooth 1 --gpu-prof --set DOL_AURORA_FRAME_INTERP_STATS=1`, 3 runs never pause and 3 automatic | Steam Deck, gamescope headless 1280×800 | Replay ≈ 5 ms GPU per game frame (2.3–3.1 ms × 0.5 per profiled frame). Staging copies 3.4–5.5 ms per profiled frame (×1.0–1.5) against 0.40 ms with Smooth Motion off, for the same 9.5 MB of vertices per game frame. Never pause: 867–869 delivered of 870, 0 rejected, 0 skipped; overloads: render worker 42–101 (462–1671 ms), late drawable 3–12 (72–386 ms), GPU behind 0–2, GPU staging 0. Automatic: 464–597 delivered and 270–404 skipped; drops on render-worker waits (2) and late drawables (2) |
| M1-T04 | Same AppImage; Deck `~/bluewake-render/runs/diag-085510/never-mailbox/` (private) | Never pause with `BLUEWAKE_VSYNC=0` (Mailbox) against the FIFO never-pause runs, 3 runs each | Headless gamescope, 3-image swapchain, 60 Hz; FIFO is the default because gamescope runs as X11 (`main.c` selects Mailbox only under Wayland) | Headless only: presentation backpressure. Render-worker waits 42–101 → 0, late drawables 3–12 → 1–3, present wait 38–117 → 8–10 ms/s, with 868–869 frames delivered either way. Staging copies stay elevated (≈ 2.7 ms × 0.5) but do not trigger pacing |
| M1-T04 | Same AppImage in Game Mode (`~/BlueWake-smooth-diag-x86_64.AppImage`); local samples `mbx-auto.txt`, `mbx-never.txt` (private) | 5 × 10 s windows per mode in the Lava Room, `BLUEWAKE_VSYNC=0`, stats on | Steam Deck Game Mode, live gamescope at 60 Hz | **Headless selection withdrawn.** Mailbox in Game Mode: automatic pacing still pauses (15 delivered, 1483 skipped; render-worker and late-drawable drops); never pause runs at 75–93% game speed, with 178 late drawables (4.8 s) and 130 render-worker waits (2.0 s) in 50 s. Headless gamescope has no real scanout and hid this. Cause still open: GPU and compositor capacity in Game Mode |
| M1-T04 | Same AppImage in Game Mode; local samples `fifo-never.txt` (private) | 5 × 10 s windows, FIFO, never pause, stats on | Same view, live gamescope | FIFO matches Mailbox: game speed 75–92%, late drawables 200 (5.5 s), render-worker waits 154 (2.4 s), GPU behind 24, 95% GPU busy. Present mode is not the lever in Game Mode; the main trigger is the drawable acquire taking 20 ms or more |
| M1-T04 | Same Game Mode session; local samples `fifo-never-1600.txt` (private) | 5 × 10 s windows, FIFO, never pause, stats on, GPU clock fixed at 1600 MHz (manual, confirmed in `pp_dpm_sclk`) | Same view, live gamescope; CPU cores averaged 3.4 GHz | All GPU-side overloads gone (late drawable, render worker, GPU behind: 0 / 0 / 0), GPU 66% busy, yet game speed 74–82% with the game thread 98% busy (cause `game-thread`). The limit is game-thread time, not GPU capacity; the late drawables at automatic clocks come from the GPU governor holding 1040–1500 MHz |
| M1-T04 | Deck `~/bluewake-render/runs/cpuprof-092333/` (private) | `rendered_run.py --cpu-profile --profile-window lava:136600:137800`, Smooth Motion off vs never pause | Headless, same AppImage and state; CPUs 0–1 share a core | Game thread: same instructions (115.5 G vs 114.7 G), cycles +8.3%, IPC 1.89 → 1.73, same 3.35 GHz, busy 91% → 99%. No Smooth Motion function on the game thread; the other threads add ≈ 5.8 s CPU per 20 s. The slowdown is contention (SMT sibling or cache), not added game-thread work |
| M1-T04 | Deck `~/bluewake-render/runs/pin-*` (private); helper `pin.sh` (game/main thread alone on CPU 0, CPU 1 idle, other BlueWake and gamescope threads on 2–7) | Same `--cpu-profile` window, Smooth Motion off vs never pause | Headless; the first attempt pinned a load-time worker by mistake and was discarded | IPC off/on 1.85 / 1.77 (−4.6%, unpinned −8.4%); game speed 29.84 / 28.86 FPS. The SMT sibling explains about half the IPC loss and none of the speed. Remaining suspects: shared L3 and memory traffic (cache misses +4–6%) and spin-waiting on frame slots in `camera_steps` (instructions +3%) |
| M1-T04 | Deck `~/bluewake-render/runs/memhog-*` (private); `memhog.py` (two Python processes copying 64 MB buffers, pinned to CPUs 4–5 and 6–7, ≈ 12 GB/s together) | Smooth Motion off, same `--cpu-profile` window, memory load started 6 s in | Headless, same AppImage and state | Game thread IPC 1.89 → 1.18 (−38%) with the same instructions (115.5 G vs 116.6 G); game speed 29.97 → 19.1 FPS. The game thread is highly sensitive to shared memory bandwidth. Spin-waiting ruled out by source: frame-slot, staging and pacing waits sleep in `wait_for_gpu_progress` |
