# BlueWake goal loop — v18 (2026-09-14)

**User-started. Finish and test the app.** The terminal condition is unchanged:
from a double-click of BlueWake.app with no exported variables, a human reaches
controllable Outset gameplay with correct video and audio, drives it with the
keyboard without a crash, and time-to-playable is under five minutes, with the
five governing numbers holding on this host.

## Why seventeen loops did not get this going, stated as a single defect

Every loop has measured the product with instruments that were built for the
route. The route is driven by synthetic pad pulses armed at guest-state
transitions, and the code that notices "the walk worked" was written inside those
same armed blocks. So the milestones were properties of the driver, not of the
guest, and a real key run could satisfy the product and still report nothing.

This loop found the defect twice more, in the same shape, before writing
anything:

* **`name-input-complete` and `name-scene-change-ready` were unreachable
  without a route pulse.** Both were emitted inside `if (g_file_start_pulse.triggered
  && ...)` (`runtime/host/src/main.c:6834`), and `g_file_start_pulse` is
  `configure_latched`-ed only when the route's pulse environment is set. No
  live run could ever report the name milestone, whatever the human typed.
  Fixed: both now report from guest state when the pulse fired **or** when no
  route pulse is configured at all. The route's timing is untouched, because the
  pulse-driven scheduling below it stayed gated by the pulse itself and
  `trigger()`/`release()` are inert when nothing is configured.
* **`name_char_jut` and `name_char_cpad` are still unreachable without a
  route pulse**, and are printed in the milestone summary as `0/0` in every
  live run (`main.c:6681`, `6709`). This loop read `name_char_cpad=0/0` as
  "no character was ever typed" and briefly believed it. It is not evidence of
  anything on the product path.

The third instance is the same defect in the bit layer rather than the gate
layer, and it produced a false failure of the opposite sign:

* **Two button layouts disagree, and the test read the wrong one.** On the wire
  A is `0x0100` and START is `0x1000`
  (`ref/recompcore/GXRuntime/graphics/aurora/include/dolphin/pad.h`). In the
  guest's own GZLE01 cpad record at `0x803A4E20`, A is `0x0100` and START is
  `0x0010`: `host_guest_cpad_start_released` reads `0x803A4E21 & 0x10`. The
  acceptance test counted presses with the **wire** mask against the **guest**
  record, so a real RETURN landed and counted as nothing. The counters now
  `grep cpad_trig=0x0010` for START and `0x0100` for A, and the same run
  proved them: `count_start 1 -> 2` on the RETURN press. **The name-entry
  screen's own legend reads "Choose = A" and "Return = B"**, so the screen's
  naming and the runtime's key naming must not be assumed to agree either.

## What v17 got wrong, and what that costs the plan

**H-CAP-SWEEP is falsified.** v17's registered prediction was that a flat far
bound of 1,024, then 2,048, then 4,096 would land between 130M and 240M turns
with the digest unchanged and time-to-playable at or under 500 s. The sweep did
not produce that: above 256 the cap is not a lever, and the only record that
moved was the timing aggregate. Keep `dynamic`. Do not spend another run on the
cap. `256/1024` stays as the shipping default because it is the measured
policy, not because it is fast.

**The v16 baseline was contaminated.** A stale app at 100% CPU (pid 5493) was
alive during it; `scripts/bench.sh`'s guard only greps for `bluewake_host`, so
an app-bundle process slips past it. Any number compared against the v16
baseline inherits the error. The one clean figure that survives is the rendered
path: **time-to-playable 266.23 s** (play-scene at retrace 13,910, from the
host's own `[frame-timing]` stamps, `(13910-1)` frames at the rendered
cadence), which is already **under five minutes**. Re-derive the baseline once,
cleanly, before quoting any ratio against it, and fix the guard to match the
bundle path `MacOS/BlueWake` as well as `bluewake_host`.

## Where the product path actually stands

Two live acceptance runs this session, both against a copy of the canonical
route card, both with a real OS-delivered key path and no synthetic pulse armed
anywhere:

* the walk drives the title to `file-select` (retrace 537) and **reaches the
  guest's Name Entry screen** — the frames are unambiguous: File Selection at
  retrace 601, Name Entry at 1501, 2252 and 2852;
* **it does not leave it.** Rounds 1 and 2 delivered all five presses — choose
  New Game, begin the file, type a character, RETURN, confirm — with no
  `name-input-complete` and no `new-game-intro`; round 3 ran out of the
  3,000-retrace ceiling with the app already stopped, so its press had no process
  to land in. The stop itself was clean: `[run] stopped: normal`,
  `gx-core … rejected=0 failed=0`, `dsp-lle … first_nonzero=1`, and the
  user's own slot byte-identical.

So the failure is now **one screen wide**, and it is the first place a real key
run has to drive something the route only ever drove with pulses. The route's own
log says what that screen wants, in the order it wanted it (`run1.log`):
`file-select 535 -> slot 576 -> start 624 -> name-char 684 -> name-end 691 ->
name-confirm 696 -> name-input-complete 712`. Read against the source, the
route's own naming is a trap: the pulse called **name-end** carries
`buttons=0x1000` (`main.c:2926`), i.e. **START**, and it is armed the moment
`cur_pos` becomes nonzero — one A press to type, then START, then A to confirm.
RETURN therefore is the right wire button, and the open question is why the guest
does not take it live.

## The instrument this loop added, and why it is the loop's first move

`BLUEWAKE_INPUT_PROBE` now prints a `[name-scene-state]` line on change: the
scene's active proc index (`+0x554`), the name record (`+0x424`), and inside it
`sel_proc +0x2903`, `sel_menu +0x2904`, `cur_pos +0x2907` and the
`name-input-complete` byte `+0x290B` (`main.c:2471`). Those are the exact
values the route's name driver gates on, so they are the exact values that decide
whether a real key run can pass the screen — and unlike the driver, this probe
runs with **no** pulse configured. It is already confirming the shape of the
flow (`main_proc 0 -> 2 -> 3` at retraces 472, 510, 543).

## Hypotheses this loop owns

* **H-DRIVER-GATE** — new, and it is the class, not the instance: *every*
  milestone the acceptance test reads must be shown reachable with no route
  pulse configured. Audit each `boot-milestone`, `pad-milestone` and
  `scene-milestone` emission in `main.c` for a `configured`/`triggered`
  guard on a route pulse. Each hit is either moved onto guest state or explicitly
  labelled route-only and struck from the product-path clause set. Falsified if
  an audit finds no remaining driver-gated milestone that the product path reads.
* **H-NAME-LIVE** — new, and it is the one screen in the way: the live walk
  reaches Name Entry and cannot leave it. Decide from `[name-scene-state]`
  whether the guest is stuck before the character is typed, between the character
  and the END cell, or on the END cell, and drive that one transition with real
  keys. The route's own gate order says the screen has three distinct states, so
  a walk that presses all five buttons blind is not a walk.
* **H-TURN-COST** — carried from v17, still unpaid: the per-turn host wrapper is
  26-30% of wall time and runs 521,121,740 times, so cost per turn is a quantity
  to report, not a headline fps.
* **H-DSP-THREAD** — unchanged, and still the largest removable share on the
  phase that owns the metric (28% of the intro, and the intro owns the number):
  worth up to ~1.2x, at the price of a real determinism argument.
* **H-CORPUS** — unchanged: the flat 40% corpus share, the per-instruction memory
  helpers, and every guest store going through a pointer the compiler cannot
  prove does not alias `CPUState` (`ref/recompcore/GXRuntime/include/core/cpu.h:291`).
  One hot function priced before any build flag moves.

## The work, in order

1. **Read `[name-scene-state]` and finish the live name entry.** One
   instrumented menu pass decides H-NAME-LIVE. Drive the screen by its own state,
   not by a fixed five-press loop: tap A until `cur_pos` moves, tap START until
   `sel_proc/sel_menu` walk to the END cell, tap A until `name_done` reads 1.
   A round that cannot advance is a screen state to print, never a press to
   repeat harder.
2. **The H-DRIVER-GATE audit**, before trusting another green run. It is a
   source read, it needs no build, and it is what makes every later clause mean
   something. The two `name_char_*` counters are already known hits.
3. **`scripts/app_acceptance_test.sh --menu` green**, with the ceiling raised
   far enough that six walk rounds fit (3,000 retraces fit about 2.6 rounds).
   `name-input-complete` or `new-game-intro`, at least three frames, zero
   failed clauses.
4. **The default `game` pass**, unchanged in what it must show: title →
   file-select → name-input-complete → new-game-intro → play-scene; a real 6 s W
   that moves `pos=` in the guest's own player record with the stick word
   present; a real RETURN and a real left arrow at Outset; the player-ready
   frame; `[run] stopped: normal`; `rejected=0 failed=0`;
   `first_nonzero=1`; time-to-playable under 300 s from the host's own stamps;
   the user's card byte-identical.
5. **Then, and only then**, the performance work, in v17's order: H-TURN-COST as
   a number, H-DSP-THREAD, H-CORPUS measured first.
6. **Owed residue:** the controlled "one press moves Link N units" displacement
   test; `scripts/bench_report.py:87` `window_metrics` off-by-one
   (`KeyError: 14097`); the `bench.sh` overlap guard above.

## The iteration contract

An iteration counts only if time-to-playable fell, or fps moved on a named path,
or a human can now see, hear or do something they could not before. One
hypothesis, one small change, one build, one measurement, and at most one short
section appended to `docs/archive/status/CURRENT.md`. **A document is not an
iteration**, and neither is a probe: a probe is a cost paid to find the work.
Report to the user every three iterations in one paragraph.

**And a gate that cannot fire on the product path is not a gate.** That sentence
is this project's failure signature, and it has now cost seventeen loops. A
milestone the driver can produce and the guest cannot is worse than no milestone
at all, because it reads as a failure of the app.

## Fences, non-negotiable

* Never commit, publish or expose Nintendo data, original binaries, generated
  game code, saves, captures, device data, signing material or leaked source.
  The user-owned GZLE01 stays private and out of git, every PPM and PNG stays in
  `/tmp`, and `ref/` is gitignored so an edit there ships as a patch under
  `patches/aurora/`.
* Preserve user data. The card at
  `~/Library/Application Support/BlueWake/GZLE01.card`, sha256 `b0163d86...`,
  is hashed before and after every run and must come back byte-identical. Never
  write the user's own slot: the walk runs against a copy of the canonical route
  card, sha256 `6b43aabd...`.
* One BlueWake process at a time; the acceptance script refuses to overlap. A run
  launched through `open` needs its shell alive for the whole run.
* No disk-heavy work — repo-wide scans, `lldb`, `atos`, `otool` — while a
  measurement run is live. A background app at 100% CPU invalidates a timing
  number, and the guard has to match the bundle path too.
* No silent stubs, no no-op substitutes, no weakening or deleting a test or a
  digest because it exposes a failure. **A synthetic button press is a route
  driver, not evidence that the keyboard works.**
* Never write that no product gate advanced as a successful outcome.

## The report this loop owes the user

One sentence, in this shape: *Double-click BlueWake.app, press J once when PRESS
START appears, and the file menu comes up; the new-game intro costs X minutes and
Y reaches Outset; here is the screenshot of Outset with Link under your control.*
Silence is not acceptable; neither is a document about why it cannot be done.
