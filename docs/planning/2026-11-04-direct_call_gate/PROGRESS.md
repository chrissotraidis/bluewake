# PROGRESS — Direct-call gate

Status: **Steam Deck render_bench +5.5% overall** (every scene faster, every gate run ahead of every control run, checkpoints identical); desktop −7.3% cycles per frame; 0 unsafe gate answers in 528 M. Iteration 2 (run-time targets) was not consistent on the Deck and is removed (`7ef8ca5b9`, reverted).
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md) · Stacked on [#503](https://github.com/chrissotraidis/bluewake/pull/503)

Acceptance (#503's rule): the candidate ahead of the control in every interleaved Game Mode launch (at least six), checkpoints identical, zero unsafe gate answers under verify, focused tests in CI. No minimum gain.

## Milestones

### M0: Attribute the remaining cost

- [x] M0-T01: Profile the #437 build at Outset. The direct-call checks are 8.4% of samples, mostly the call's entry; 840 copies of each helper.

### M1: The gate (depends: M0)

- [x] M1-T01: Implement the gate, opt-in (`BLUEWAKE_DIRECT_GATE=1`), with the builder's `--direct-gate` for training.
- [x] M1-T02: Unit test: the gate against the full check in 1,024 states, verify counters, stale-byte detection, every path that closes it; -O0 and -O2. The host compiles cleanly in the build container.

### M2: Build and verify (depends: M1)

- [x] M2-T01: Build the candidate: the #437 recipe plus this change, `--clang22 --direct-gate` (local `bench/503-gate` `e627ece79b59`, not on the PR).
- [x] M2-T02: `BLUEWAKE_DIRECT_SKIP_VERIFY=1` with the gate handed over, in Outset and a scene change: **0 unsafe in 528 M gate answers**.

### M3: Measure (depends: M2)

- [x] M3-T01: Desktop first read: instructions and cycles per frame against the `clang22` control. **−7.3% / −7.3%, rate +8.0%**, all three gate runs ahead of all three control runs, checkpoints identical.
- [x] M3-T02/T03: Steam Deck render_bench in Game Mode's session, five scenes, six interleaved launches of each build: **+5.5% overall**, every gate run (40.26–41.08) ahead of every control run (38.32–38.62); guest checkpoints identical across all 12 runs.
- [x] M3-T04: Iteration 2: the gate also for targets known only at run time (indirect calls, 1.3% of samples with the first gate). Desktop −1.6% instructions, −1.2% cycles, consistent; **Deck +0.73% and +0.66% in two sessions, but not ahead in every launch** (4 of 6 rounds). **No-go by the acceptance rule:** the code is reverted, and it stays in history as `7ef8ca5b9`.
- [ ] M4-T01: Decide on the first gate: propose as the default, with a Config → Performance switch (#531's section).

### M4: Decide (depends: M3)

- [ ] M4-T01: Keep opt-in, propose as the default, or no-go (code removed, record kept).

## Evidence ledger

| Task | Commit / artifact | Command / method | Environment / input | Result |
|---|---|---|---|---|
| M0-T01 | #437 build `clang22` (`perf/clang-runtime-matrix@14c558d`, Clang 22.1.8, libc++); local profile and attribution (private) | `perf record -e cycles:u -F 4000`, headless, unpaced, 2,400 retraces from `1-outset.bwstate`; symbols grouped by layer | Desktop PC (Ryzen 7 5800X3D), Linux | Direct-call checks **8.4%** of all samples (translated chunks 58.6%, natives 12.2%, dispatch loop 5.0%); in `bw_direct_call_ready_plain` most samples sit on the entry and the `bw_host_skip_fast` load; 840 copies of each helper in the module |
| M1-T02 | This branch | `tests/direct_calls_test.c` at -O0 and -O2; host `main.c` syntax check in `bluewake-linux-ci:local`; `tests/test_linux_ci_container.py` | Desktop PC | All pass; the host compiles with no warnings |
| M2-T01 | Candidate `gate503` (`bench/503-gate` `e627ece79b59` = `14c558d` + this change; Clang 22.1.8, libc++, trained with the gate open: provenance `direct_gate_training: true`) | `build_script gate503 bench/503-gate … --clang22 --direct-gate` | Desktop PC | Built in about 55 minutes |
| M2-T02 | `gate503` | `BLUEWAKE_DIRECT_GATE=1 BLUEWAKE_DIRECT_SKIP_VERIFY=1`, headless, `1-outset.bwstate`, 3,000 retraces with a warp to Windfall at +1,200 | Desktop PC | `[direct-gate] verify: answers=528437808 unsafe=0 conservative=310417`; `[direct-skip] verify: inline=544293898 unsafe=0` |
| M3-T01 | Control `clang22` (A, `14c558d`) against `gate503` (B); local `measure-552/` (private) | `perf stat -e cycles:u,instructions:u`, headless, unpaced, 2,400 retraces from `1-outset.bwstate`, order A B B A A B, `BLUEWAKE_DIRECT_GATE=1` for both (A has no gate) | Desktop PC (Ryzen 7 5800X3D) | Per game frame: A 76.31 / 76.38 / 75.98 M cycles, 210.32 M instructions; B 70.63 / 70.75 / 70.52 M cycles, 195.02 M instructions (**−7.3% each**). Median rate A 116.9–117.3, B 125.9–126.5 retraces/s (**+8.0%**). Guest checkpoints identical in all six runs |
| M3-T02/T03 | Control `clang22` (A, `14c558d`) against `gate503` (B, `e627ece79b59`); local `gate-552-deck/` (private) | `render_bench deck@… A B --states test_render/states --display desktop --pairs 3`, `--set BLUEWAKE_DIRECT_GATE=1` (both; A has no gate), render scale 3, Smooth Motion 1 step, widescreen, efficient soft effects | Steam Deck, Game Mode (gamescope's X server) | **B +5.49% overall**: Outset +4.8%, Lava Room +8.2%, Hyrule Castle +3.6%, Butterfly Room +4.8%, Shipwreck +5.5%. Per run, A 38.59 / 38.32 / 38.62 / 38.36 / 38.56 / 38.56, B 40.28 / 40.61 / 41.08 / 40.43 / 40.26 / 40.53. Guest checkpoints identical across every run; picture flags are the known capture-timing ones (Outset PSNR 33.9, Butterfly Room 15.48, as in #437's grid) |
| M3-T04 | With the first gate (`gate503`); local profile | `perf record`, as M0-T01 | Desktop PC | Direct-call checks **2.08%** of samples (were 8.38%): `bw_direct_call_ready_full` 1.33% (run-time targets), `host_can_skip_observation` 0.56%, `_plain` 0.13% (REL targets: not worth a gate of their own) |
| M3-T04 | Iteration 1 `gate503` (A) against iteration 2 `gate552b` (B, `bench/552-gate2` `7ef8ca5b9258`: the first gate plus run-time targets; trained with the gate open); local `measure-552/` (private) | As M3-T01: verify run, then `perf stat`, headless, 2,400 retraces from `1-outset.bwstate`, A B B A A B | Desktop PC | Verify: 0 unsafe in 528,437,808 gate answers (the count covers run-time targets too). Per game frame: A 70.73 / 70.52 / 70.50 M cycles, 195.05 M instructions; B 69.70 / 69.60 / 69.81 M cycles, 192.02 M instructions (**−1.6% instructions, −1.2% cycles**); rate 126.1–126.7 → 127.7–128.1 (**+1.0%**). Checkpoints identical, and equal to the control's |
| M3-T04 | Iteration 1 `gate503` (A) against iteration 2 `gate552b` (B), session 1; local `gate-552b-deck/` (private) | render_bench as M3-T02/T03, three A/B/B/A pairs | Steam Deck, Game Mode | B **+0.73% overall** (Outset +0.7%, Lava Room −0.2%, Hyrule Castle −1.0%, Butterfly Room +1.8%, Shipwreck +0.9%); per run A 40.50 / 40.66 / 40.94 / 40.75 / 40.73 / 40.30, B 41.01 / 41.03 / 40.66 / 40.93 / 40.69 / 40.81: **not ahead in every launch**. Checkpoints identical. A second session follows |
| M3-T04 | Same pair, session 2; local `gate-552b-deck-2/` (private) | As session 1 | Steam Deck, Game Mode | B **+0.66% overall**; per run A 40.45 / 40.37 / 40.54 / 40.70 / 40.45 / 40.67, B 40.73 / 40.71 / 40.26 / 40.73 / 40.99 / 41.08: one gate run behind both control runs of its round again. Checkpoints identical. **No-go:** reverted |
