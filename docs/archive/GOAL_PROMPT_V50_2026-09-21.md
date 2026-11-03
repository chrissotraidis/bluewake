# BlueWake goal loop - v50 (2026-09-21)

**User-started: read where the work is at, then continue it.** This page is the
operating document for finishing the project. It supersedes
[GOAL_PROMPT_V49_2026-09-18.md](GOAL_PROMPT_V49_2026-09-18.md) and the v25-v48
continuity pages, because v49 closed its own workstream: the emitter-trim hunt
it opened is finished, with two landed changes worth 0.79 percent between them
and every other candidate priced at or near zero.

## Terminal condition

Unchanged, and it is the PRD's, not this loop's:

> From a double-click of a signed BlueWake build with no exported variables, a
> human plays the user-owned `GZLE01` disc from the original boot through the
> ending and representative optional content at authentic speed, with correct
> video and audio, real input, preserved saves, and no interpreter-executed
> guest instructions on the audited route; the same core then ships to iPhone
> and iPad with idiom-appropriate touch interaction.

Every clause of that is a gate in section 4. Reducing the scope, the platforms,
the acceptance coverage or the definition of done is the user's decision and
not a bot's.

## 1. Where the project is, earnestly

**The product half of macOS is met.** The signed app validates and privately
prepares the disc, translates the retail DOL and all 415 RELs ahead of time into
one arm64 composite, boots through the original boot flow, reaches title, file
select, new game, name entry, the intro and controllable Outset gameplay, takes
real key input, renders through Aurora/Metal, plays audio through the DSP
adapter, writes and reloads the game's own save, makes the retail Outset-to-Omasao
transition, and stops normally at pc `0x80307EF4` after 32,203,791 host turns
with the route digest `92dd816c...`. Route A (static recompilation) is the
shipping route; Route B (source reconstruction over Aurora) is far behind - it
has headless `Link` diagnostics and native 2D presentation, not a playable
world - and stays an oracle, not a parallel product.

**What is not met.** In the order it blocks the terminal condition:

| # | gate | state | evidence |
| --- | --- | --- | --- |
| P1 | authentic play-scene speed | **open, large** | certified window is 302.3 M host instructions per play retrace headless / 403.3 M rendered against a 60-retraces-per-second budget |
| P2 | renderer cost and tail | open | renderer is +101.0 M instructions/retrace (+33.4%) and p99 frame time 77.6 ms rendered against 47.3 ms headless |
| P3 | graphics correctness | open | Link's hair/material state is visibly wrong; wider GX fidelity unqualified |
| P4 | physical controller | open | keyboard proven; GameController unproven |
| P5 | compatibility campaign | open | no named-scenario coverage from new game to ending, no endurance soak |
| P6 | reproducibility | **partly broken** | the shipped composite cannot be regenerated end to end from the tree (REL namespace overlap); the artifact in `build/` is a *screened* mixture, not a full rebuild |
| P7 | iOS / iPadOS | deferred | begins after P1-P5 |
| P8 | acceptance definitions | user decision | the five-minute time-to-playable bar is unreachable for a cold new game by arithmetic (337.6 s of authored content); the save-continue reading is reachable and unbuilt |

**Where the money is.** P1 is instruction-bound: the play window retires 393.4 G
instructions in 28.62 s of user CPU at IPC 4.01, which is the machine's peak, so
nothing but fewer host instructions per guest cycle buys speed. The emitted body
costs 27.0 (synthetic state) to 30.4 (real in-game state) host instructions per
guest cycle against roughly 20 for the whole authentic-speed frame, and the
ablation suite prices its removable parts at 10-15 percent - the cycle-accounting
suite - with the register-file hypothesis refuted at 0.6 percent and the access
path worth 0.6 percent at the route.

## 2. The contradiction this loop must resolve before it plans

The ledger carries two different instruction counts for the play window and
never reconciled them:

| source | window | instructions per retrace |
| --- | --- | --- |
| the certified rendered split (two runs, matching turn counts) | retraces 13,800-14,100 | **302.3 M** |
| `scripts/bench_instructions.sh` (two tiers, differenced) | retraces 13,900-14,700 | **491.7 M** |

Both are described as "the play window" in different documents, and the two
speed claims in the tree follow from the two numbers: 302.3 M against a 10 G
instructions-per-second host rate gives 1.81x, and 491.7 M against the measured
13.75 G instructions per second gives 2.15x. The stretch that closed with "1.81x
is not reachable by translation quality" rests on the first; the second is 19
percent further away and would argue even harder for the same conclusion.

It matters because 1.3x, 1.8x and 2.2x are different projects. The first is a
bounded trims-and-accounting program. The third is an emitter-shape redesign
measured in weeks. The loop does not get to pick the comfortable one.

**The reconciliation, done 2026-09-21.** Both numbers are right and they are
different windows. A three-ceiling ladder (13,800 / 14,100 / 14,700) on one
build gives: opening 53.6 M per retrace, early play 13,900-14,100 **402.1 M**,
the certified window 13,800-14,100 **285.9 M** (a third of it cutscene), steady
play 14,100-14,700 **492.1 M**, and the bench window 465.1 M. The play scene is
not stationary: turn density per retrace runs 1,998, then 11,330, then 30,498 as
Outset fills in. Authentic speed needs 24 G instructions per second in the early
window and 29.5 G steady; the host retires at most about 14 G. So **1.83x early,
2.2x steady** - the certified 1.81x is the number for the window it came from
and is an understatement for the scene a player spends time in. The ladder also
reproduced the certified stop and the certified digest `92dd816c...`, so the
harness is calibrated against certification.

Two properties of that ladder are now standing rules. Wall clock varies by a
third between processes on this machine (the same 190 retraces measured 30.47 ms
in one process and 40.74 ms in another) while instruction counts differ by 0.29
percent, so **a speed claim is an instruction count at a certified ceiling with
the digest green, and the milliseconds are colour**. And same-ceiling
reproducibility across processes is 0.29 percent, not the 0.02 percent that
repeat runs of one configuration suggested - the card left by the previous
ceiling is part of the configuration.

## 3. The program

Ordered. Each item states its entry condition, its iteration, and the gate it
closes. Nothing here is a promise about weeks; the sequence is what matters.

### P0 - Truth and hygiene (this loop's first act)

The tree must build and its central number must mean one thing before any
optimisation work is scheduled.

- **Fixed 2026-09-21:** `tests/delivery_digest_test.c` was never updated when
  `bluewake_delivery_digest_record_external` gained its `in_play` argument, so
  `scripts/build_macos_dsp_host.sh` failed to compile the test suite and
  `--output-on-failure` never ran. The test now passes the argument, asserts the
  play-scene accumulators (which had no coverage at all), and the suite is
  217/217 green. A tree whose tests do not build cannot gate anything.
- **Landed 2026-09-21:** the edge fast-reject, at **-4.42 percent** on the play
  window (486.6 M to 465.1 M per retrace, digest `37e1c8b5...` identical,
  certified stop and turn counts identical at both ceilings). It arrived broken:
  the generator's placement loop never wrote the key it had accounted for, so it
  reported a perfect hash it had not placed, three keys were displaced past the
  single probe, and the guest never reached the title screen. The lookup is now
  open addressing terminated by an empty slot - sound by construction - the
  generator writes the header and verifies every key, and the test asserts the
  lookup finds every key it emits.
- **Open:** write the one-page position into `docs/archive/status/CURRENT.md` and keep
  it small; the composite still has to regenerate from the tree.
- **Open:** the composite must regenerate from the tree. Until it does, no
  artifact in `build/` is reproducible and every "the generator change is
  durable" claim is an inference.

### P1 - Authentic play-scene speed

Entry: met. P0's reconciliation closed 2026-09-21 with the gap stated in
instructions per retrace, which is the unit that reproduces across processes.
Iteration: one measured candidate at a time, digest green, run verified before
its number is read, measured on the steady window 14,100-14,700 as well as on
the certified pair. Gate: 16.667 ms per retrace in the certified window,
headless and rendered.

Work, in the order the current evidence supports:

1. **Host-side trims** (cheap: seconds to rebuild, minutes to measure). The
   dynamic census puts device sync at 6.4 percent and edge service at 4.3, with
   the pc lookup 2.2 and the card intercept test 1.3. These are the loop's
   fastest iterations and they are real, if small.
2. **The cycle-accounting design** (the largest *measured* lever, 10-15 percent
   of the emitted body). Per-instruction charging at 9.8 percent residual after
   the out-of-line move, the observation suffix at 6.4, the budget test at 3.3
   already landed. The design question is narrow: can the same bounded-drift
   guarantee hold with a charge granularity coarser than per instruction and a
   deadline check that is not at every charge site? D2 already admits bounded
   per-delivery drift with the route digest as the gate, so this has an
   acceptance test rather than a hope. The lean form measured -22.4 and -18.0
   percent on two chunks; the `prepaid` ablation is what that costs.
3. **The GX FIFO path off the main thread** (rendered only; ~79 M of the
   renderer's 101 M; Aurora's render worker is idle 94 percent of the time).
   Answer first what pins translation to the main thread. If it is the observer
   reading guest state, the fix is a snapshot and a handoff. This is also the
   only candidate that touches the p99 tail, which is what a player feels.
4. **The emitter shape**, if 1-3 leave a gap after being measured rather than
   assumed. The ablation prices are the specification: everything identified is
   10-15 percent, the register file is refuted, the access path is worth 0.6 at
   the route, and the per-instruction pc store is worth 0.7 - so a shape that
   reaches the target cannot be the current shape with trims. Repricing that
   conclusion against the reconciled number from P0 is the entry condition, and
   the honest outcome of this item may be a smaller emulated-cycle target
   presented to the user rather than a redesign. Presenting it is the user's
   call, not the loop's.

### P2 - The renderer's cost and the tail

Entry: P1 items 2-3 measured. The rendered tier needs a per-phase instruction
or time counter compiled into the GX frontend - host code, twelve-second
rebuilds - because five sampling-profile methods have now produced refuted or
retracted answers. Gate: rendered play window within the same budget as headless
plus a presentation cost that is bounded and stated, with p99 within 1.5x of the
median.

### P3 - Graphics correctness

Entry: P1's accounting change landed or abandoned (both are large and touch the
same generated code, and a correctness hunt inside a moving emitter is twice the
work). Candidates the ledger already names: Link's hair/material state, and the
`GXPeekARGB`-shaped EFB read-back the Aurora audit recorded as the Picto Box
gap. Gate: the PRD's visual acceptance list with per-item evidence.

### P4 - Input

Entry: independent of P1. GameController support and the coexistence rules
SunPad already demonstrates; the touch layer waits for P7. Gate: one unscripted
human session recorded with a physical controller.

### P5 - Compatibility campaign

Entry: P1 gate passed, because the campaign cannot be run at 45 percent speed
and cannot be timed while the frame rate is the defect. Build the coverage
manifest (scenarios, platforms, form factors) and the tester flow the PRD
section 7.3 describes, then run it from new game to the ending plus optional
content. Gate: every scenario pass/fail with deterministic milestones, plus an
endurance soak that produces a stability number.

### P6 - Reproducibility

Entry: none; can run in parallel with P1-P5, and should, because it is the
cheapest way to make every later result trustworthy. Regenerate the composite
from the tree, fix the REL namespace overlap that blocks it, register every
`ref/` patch in `config/dependencies.lock.json`, and make one command rebuild
the artifact that ships. Gate: a from-scratch build reproduces the shipped dylib
byte-for-byte, as the 756-object rebuild already did once on 2026-09-18.

### P7 - iOS / iPadOS

Entry: P1-P5 gates. Inherit the core, add SunPad's interaction language (three-dot
menu, editable touch controls, touch/controller coexistence, staged Files
import, lifecycle handling, guided diagnostics) and idiom-specific layouts.
Gate: the shared-core product passes the same coverage manifest on device.

### P8 - The decisions that are the user's

Named here so they are never silently made by a bot:

1. **Time-to-playable.** The five-minute bar cannot hold for a cold new game
   (337.6 s of authored content at authentic speed). The save-continue reading
   can, and the engineering for it is staged.
2. **Emulated-cycle target.** If P1's item 4 shows the honest ceiling of this
   architecture below 60 retraces per second, the choice between a redesign and
   a lower play-scene target is the user's.
3. **Scope, platforms, acceptance coverage, definition of done.** Unchanged from
   the PRD.

## 4. The iteration contract

An iteration counts only when a measured number moved on a named path with the
route digest green and the run verified, or when a new instrument answered a
question that changed a decision. A document is not an iteration and neither is
a probe. Report one paragraph to the user every three iterations.

Instruments, and what each is for:

- `scripts/bench_instructions.sh` - two headless tiers differenced; resolves
  0.02 percent of the instruction stream in about three minutes. The primary
  metric of P1.
- `scripts/bench_rendered.sh` - the rendered split; refuses to report unless both
  runs stopped normally inside the certified window.
- `scripts/recomp_chunks.sh` - screens a candidate on the hot ten chunks in
  8-15 minutes against a 90-152 minute DOL-only rebuild and a 2h15m full rebuild.
  It relinks directly, because `cmake --build` re-checks every object and turns a
  screen into a full rebuild.
- `scripts/ablate_chunk.py` + `scripts/bench_chunk.sh --snapshot` - prices a
  construct in about two minutes on the real in-game state. Two minutes per
  candidate is why the ablation suite found things the 90-minute rebuild loop
  never could.
- `scripts/profile_play.sh` + `scripts/sample_owners.py` - owner shares on the
  rendered path; self-normalising against machine load, reproducible to 0.6-0.9
  points. A share includes children: subtracting is not a self time.
- The route digest and the normal-stop pc are the correctness gate. Read the run
  before reading its number.

## 5. Fences, non-negotiable

Never commit or publish Nintendo data, original binaries, generated game code,
saves, captures, device data, signing material or leaked source. One BlueWake
process at a time; no disk-heavy work while a run is live; no timing measurement
while a build is running. Everything under `ref/` is a pinned dependency whose
uncommitted state *is* the project's patch series: to back out an edit, reverse it
or restore from `patches/` - **never `git checkout` a file there**. Every patch
under `ref/` must be registered in `config/dependencies.lock.json`.

Do not re-run the refuted candidates: the access-path mirror clause, the GX sink
per-draw payload copy, `DOL_GX_CORE=0` as a rendered split, `-O3` on the DOL
chunks, per-instruction pc removal, `__restrict` on `ctx`, branch hints on the
hot guards, the observation reconcile, or the duplicated block-leader tail.

Do not plan against W4's attribution: its shares are code size, and the one that
was tested (18 percent access path) was worth 0.6 percent at the route. Do not
measure the rendered product with the headless metric. Do not treat a pad-driven
run as evidence - it is a measurement. Do not weaken a clause to make a log pass,
and do not propose a save-state warm start; the PRD forbids captured
initialization state.

## 6. Appendix - constants this loop relies on

- Certified artifact: `build/composite-cycle-hybrid-o2-v2/gGZLE01_recomp.dylib`.
  The full 756-object rebuild reproduces the `39e05177...` artifact
  (518,235,064 bytes) byte-for-byte, in 2h15m at 8 workers. The file currently in
  `build/` is the *screened* mixture (hot ten rebuilt, `c3b88249...`) that carries
  the two landed increments, so it is a measuring artifact and not a shipping
  one.
- Route: normal stop at pc `0x80307EF4`, host turns 32,203,791, digest
  `92dd816c...`, 1,050 records; `play_scene=1` at retrace 13,910,
  `opening_complete=1` at 13,850, control admitted at retrace 20,256.
- Authentic speed: 8.1 M guest cycles per retrace, 60 retraces per second,
  16.667 ms per retrace. Measured host throughput: 13.75 G instructions per
  second at IPC 4.01.
- Certified play window: retraces 13,800-14,100. Bench window: 13,900-14,700.
  They are not the same window and the numbers are not interchangeable.
- Emitted body: 27.0-30.4 host instructions per guest cycle against ~20 for the
  authentic-speed frame. Hot chunks: `0201` 15.7 percent, `0144` 3.6, `0015` 1.5,
  `0145` 1.2, `0181` 1.1.
- Landed increments: patch `0017` (duplicated block-leader guard, +2.8 percent
  median fps), the budget-test fallback (-0.52 percent instructions), and the
  out-of-line precise charge (-0.27 percent) - 0.79 percent cumulative.
- Rebuild costs: host `.c` edit ~20 s; regeneration 1.8 s; hot ten 8-15 min;
  DOL-only 206 objects 90-152 min; full 756 objects 2h15m; bench pair ~3 min.
