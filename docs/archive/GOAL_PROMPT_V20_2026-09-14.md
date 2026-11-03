# BlueWake goal loop — v20 (2026-09-14)

**User-started. Finish and test the app.** The terminal condition is unchanged:
from a double-click of `BlueWake.app` with no exported variables, a human
reaches controllable Outset gameplay with correct video and audio, drives it
with the keyboard without a crash, and time-to-playable is under five minutes,
with the five governing numbers holding on this host.

(Run labels: a `v20` acceptance run already exists and was taken on the *old*
binary, before the correction below was written. Its verdict is already decided
— it failed the W-hold clause, and the way it failed is this loop's whole
subject. The first run of this loop is therefore labelled `v21`; the document
governs the work either way.)

## The failure signature, stated precisely enough to act on

v19 got the *shape* of the gate right and the *moment* wrong, and the way it was
wrong is the reason this project has spent three loops on one clause.

v19 replaced a gate on `demo_mode == 4` — a cutscene state — with a gate on
`event_mode == 0 && demo_mode == 0`, which is the state the route's own player
trace shows a stick actually moving Link in. That is the correct destination.
But the play scene is entered through a **handover**, and during that handover
the destination's own words are transiently true. v20's log, this loop's own
run, is the proof:

```
[player-milestone] control-admitted retrace=13985 event_mode=0 demo_type=0 demo_mode=0 pos=C83E9075,44CE4000,4899357C
```

Control was admitted at retrace **13,985**. The authored awake cutscene does not
begin until retrace 13,995, and the W hold that followed landed on its first page:

```
acceptance: player record before the W hold: pos=C83ED1C0,44CEC000,48992180 pad_hold=0x00000000 stick=00000000,00000000,00000000 [1 512 169 2 38 0 ...]
acceptance: FAIL a real 6 s W hold left the guest's own player position unchanged
```

The `[1 512 169 2 38 ...]` is `demo_type=1 demo_mode=512 proc=169 event_mode=2`:
the cutscene, still eating keys, exactly as in v19. The run stopped driving A at
that moment, and since **the cutscene does not advance itself**, the remaining
~10,000 retraces of the pass measured a keyboard against a frozen page.

So the sentence for this loop is:

> **A gate that reads a destination state without reading the handover that
> produced it will fire in the seam between scenes, because in the seam the
> words are the destination's words — and a run that stops there measures the
> transition, not the product.**

## The two facts this loop is built on, both hard evidence

### Fact 1 — the keyboard-to-guest steering path is proven working, end to end

This has been the open question behind every W-hold failure, and v20's log closes
it. At retrace 14,034, with a real 6 s W hold delivered by the OS:

```
[player-scene-state] retrace=14034 player=0x80AADD14 demo_type=1 demo_mode=512 proc=169 event_mode=2 event=38 pad_hold=0x08000000 pos=C83ED1C0,44CEC000,48992180 stick=00000000,3F800000,3F800000
```

`pad_hold=0x08000000` is the W bit arriving in the guest's own pad record, and
`stick=00000000,3F800000,3F800000` is the guest's own **decoded left stick** —
x 0.0, y **1.0**, magnitude 1.0. A held W produces the guest's own steering
value, at the player layer, with no route pulse anywhere in the run. Nothing
above the player exists to fix: the cutscene simply does not read the stick.

This also retires the last instrument defect. v19 proved that `probe_hold`, the
button word at `game_pad + 0x18`, can never carry an axis — a stick lands there
as the single bit `0x08000000`. The decoded stick at `0x803A4DF0/+4/+8` is a
different record, it is the one the guest's player code actually steers with,
and it is now read and printed on every probe line. The two readings "the key
never arrived" and "the key arrived and the scene was not listening" can no
longer be the same bytes.

### Fact 2 — the discriminator that separates the seam from control already
exists in this project, and it is the handover's own phase

The scene-transition overlap object at `0x803F6160` walks a phase `0..6` while a
handover runs and rests at phase 6 once it is complete. It is the same object
the route's own control gate reads, through `g_overlap_terminal_phase`, and this
is the route's gate at `runtime/host/src/main.c:6131`:

```c
const bool player_state_ready =
    g_overlap_terminal_phase == 6u && player_actor >= 0x80000000u &&
    mem_read8(&cpu, 0x803C9EA2u) == 0u;
```

The route latches that phase exactly as this loop's probe now does — from
`mem_read32(0x803F6160 + 0x1C)` whenever `mem_read32(0x803F6160) >= 0x80000000`
and `mem_read16(+0x04) == 1` (`main.c:7109-7128`).

v20's own log shows why this excludes the early window. There are **three**
handovers in the pass, and the phase is reset to 0 at the start of each:

| handover | phases | reaches phase 6 at |
| --- | --- | --- |
| boot | 0 at 453 → 6 | retrace 513 |
| title/file-select | 0 at 778 → 6 | retrace 901 |
| **play scene** | 0 at 13913 → 3 at 13965 → 4 at 13990 → 5 at 14045 → 6 | **retrace 14047** |

The window the v19 gate latched on — `proc=0/4`, `event_mode=0`, `demo_mode=0`
for roughly sixteen retraces from 13,974 to 13,993 — sits at phase **3–4**, not
6. Phase 6 is not reached until 14,047, which is *inside* the cutscene. So the
phase-6 latch excludes the seam for the structural reason, not by a timeout.

The two earlier phase-6 arrivals at 513 and 901 are the obvious objection to
this criterion, and they do not admit control: `g_play_scene_reported` is false
until retrace 13,914, and the gate requires it. The criterion is therefore a
conjunction of three guest-derived facts — a completed handover, a live player
actor, and a cleared event mode — none of which is a timestamp and none of which
is a route pulse.

## What this loop changed before its first measurement

`runtime/host/src/main.c`, in the player probe:

* the phase latch above, mirroring the route's own latched phase from the same
  guest object;
* `probe_control_state` is now `player_valid && g_play_scene_reported &&
  player_overlap_phase_latch == 6u && event_mode == 0 && demo_mode == 0`, held
  **8 consecutive retraces** so a one-retrace flicker during a handover cannot
  be read as control;
* `[player-milestone] control-admitted` now prints `ovl=` alongside
  `event_mode`, `demo_type` and `demo_mode`, so the seam and control are
  distinguishable in the log itself;
* the probe's change test and its print carry the three decoded stick words and
  the latched phase (the slot array is 15 wide), so `stick=y 3F800000` with a
  still position is visible as one line rather than inferred.

`scripts/app_acceptance_test.sh` (landed in v19, verified in v20's parser):

* `player_field()` is an awk `=`-split, so the field count cannot depend on how a
  sed dialect counts capture groups; `player_stick()` reads the new field;
* `drive_play_control` stops on `control-admitted` and its budget is 1,200
  attempts, so a driver that stops at the wrong moment is at least visible;
* the W-hold clause waits for `control-admitted`, prints the discriminators on
  both sides of the hold, and refuses to accept `demo_mode 512/4` as control;
* the ceiling is 24,000 retraces, headroom rather than a fix.

## Falsification on record

* **H-GATE-STATE** (v19: `event_mode 0 && demo_mode 0` is the right gate) — the
  *state* is right and *sufficient on its own* is **falsified by v20**: it fired
  at 13,985, six hundred retraces before the handover completed, and the failure
  landed on the cutscene's first page.
* **H-DROPPED-KEY** (the W press never reaches the guest) — **falsified again,
  and now positively**: the guest's own decoded stick carries y `3F800000` under
  a held W. This is the strongest form the falsification has taken, because it
  reads the value the player code steers with, not a bit in a button word.
* **H-SHORT-CEILING** — still falsified. The run was starved of A presses by a
  driver that stopped in the seam, not cut off by the ceiling.

## Hypotheses this loop owns

* **H-CONTROL-PHASE** — the correction above. Passes when the default game pass
  shows `control-admitted` with `ovl=6 event_mode=0 demo_type=0 demo_mode=0` at a
  retrace at or after ~19,900 (the oracle's control is 19,972), and a real 6 s W
  then moves the guest's own `pos=` with `stick=y 3F800000`. Falsified if
  `control-admitted` still appears before ~19,900, or if the position does not
  move once control is genuinely admitted.
* **H-SEAM** — the early window is a property of the handover, not of the
  keyboard. Passes when the phase history above reproduces on v21 and the gate
  opens at 6. Falsified if v21 shows phase 6 being reached before the cutscene
  on the play-scene handover.
* **H-PLAYER-CONSUMES-STICK** — new, and it is the *next* loop's question if the
  W hold still fails with `ovl=6` and `stick=y 3F800000`. That combination would
  put the fault downstream of input, in the player code, and it must be said in
  exactly those words rather than reported as a keyboard failure.
* **H-TTP-TRACE-TAX** — owns the second failed clause and is unaddressed. Time
  to playable was measured at **382.28 s** with `BLUEWAKE_FRAME_TIMING`,
  `BLUEWAKE_INPUT_PROBE`, `BLUEWAKE_TRACE_PLAYER`, `BLUEWAKE_TRACE_PAD` and
  `BLUEWAKE_TRACE_ROOM0` all set and ~2.2 MB of stderr going to a file. The
  product claim is the double-click with **no** exported variables, so the
  clause must be decided on a clean run with the trace tax reported as its own
  number. Deleting the traces to make the clause pass would be weakening the
  test; quoting a traced number as the product's would be misreporting it.
* **H-LOAD** — new, and it is a constraint on every number this loop takes. On
  the v20 host: BlueWake at 102% CPU, a Codex renderer holding a full core for
  two days, WindowServer at 44%, an iOS Simulator running, load average
  5.05/15.24/18.44. Every timing this loop records is *indicative, not
  controlled*, and must be labelled that way until a clean re-measure.
* **H-TURN-COST**, **H-DSP-THREAD**, **H-CORPUS** — carried from v17/v18/v19,
  still unpaid. Priced first, then changed, in v17's order.

## The work, in order

1. **Build and run the default game pass.** `cmake --build
   build/runtime-host-dsp --target bluewake_host`; confirm the bundle carries the
   new field before trusting a run that lacks it. Then
   `bash scripts/app_acceptance_test.sh --retraces 24000 --frames /tmp/bw-acceptance/frames-game-v21`.
   Green is: title → file-select → name-input-complete → new-game-intro →
   play-scene; `control-admitted` with `ovl=6 event_mode=0 demo_type=0
   demo_mode=0` at retrace ≥ ~19,900; a real 6 s W that moves `pos=` with
   `stick=y 3F800000`; a real RETURN and a real left arrow delivered at Outset;
   `[run] stopped: normal`; `gx-core ... rejected=0 failed=0`; `dsp-lle ...
   first_nonzero=1`; and the user's own card byte-identical.
2. **The clean time-to-playable measurement** (H-TTP-TRACE-TAX), read from the
   host's own `[frame-timing]` stamps on the double-click path with no exported
   variables, with the traced 382.28 s reported separately as the traced number.
3. **Then performance**, in v17's order: H-TURN-COST priced as a number before
   any build flag moves, then H-DSP-THREAD, then H-CORPUS.
4. **Owed residue:** the controlled "one press moves Link N units" displacement
   test — the instrument now exists, since `[player-scene-state]` carries the
   guest's own three position words; `scripts/bench_report.py:87` `window_metrics`
   off-by-one (`KeyError: 14097`); and `scripts/bench.sh`'s overlap guard, which
   greps `bluewake_host` and therefore misses the bundle process
   `MacOS/BlueWake`.

## What not to do

* **Do not weaken the W clause, substitute a synthetic stick, or accept
  `demo_mode 512/4` as control.** A synthetic button press is a route driver,
  not evidence that the keyboard works.
* **Do not stop driving A while the cutscene is up.** It does not advance
  itself.
* **Do not build the bare `all` target** — `tests/delivery_digest_test.c` has a
  pre-existing, unrelated break. Build `bluewake_host`.
* **One BlueWake process at a time.** The acceptance script refuses to overlap
  and its guard matches the bundle path.
* **Do not read a run that is still live as a verdict.** Wait for its ceiling
  stop.
* **Do not quote a traced timing as the product's**, and do not delete a trace
  to make a timing clause pass.

## The iteration contract

An iteration counts only if time-to-playable fell, or fps moved on a named path,
or a human can now see, hear or do something they could not before. One
hypothesis, one small change, one build, one measurement, and at most one short
section appended to `docs/archive/status/CURRENT.md`. **A document is not an iteration**,
and neither is a probe: a probe is a cost paid to find the work. Report to the
user every three iterations in one paragraph. Silence is not acceptable; neither
is a document about why it cannot be done.

## Fences, non-negotiable

* Never commit, publish or expose Nintendo data, original binaries, generated
  game code, saves, captures, device data, signing material or leaked source.
  The user-owned GZLE01 stays private and out of git, every PPM and PNG stays in
  `/tmp`, and `ref/` is gitignored so an edit there ships as a patch under
  `patches/aurora/`.
* Preserve user data. The card at
  `~/Library/Application Support/BlueWake/GZLE01.card`, sha256 `b0163d86...`, is
  hashed before and after every run and must come back byte-identical. Never
  write the user's own slot: the walk runs against a copy of the canonical route
  card, sha256 `6b43aabd...`.
* No disk-heavy work — repo-wide scans, `lldb`, `atos`, `otool` — while a
  measurement run is live. A background app at 100% CPU invalidates a timing
  number, and any guard has to match the bundle path too.
* No silent stubs, no no-op substitutes, no weakening or deleting a test or a
  digest because it exposes a failure.
* Never write that no product gate advanced as a successful outcome.

## Appendix — the constants this loop relies on

Keyboard → pad: arrows are the D-pad, **J = A**, K = B, U = X, I = Y, **W/A/S/D
= the left stick**, H/F/T/G = the C-stick, E/R = L/R, Q = Z, RETURN = START.
Physical keycodes used by the test: 38 = J (A), 36 = RETURN, 13 = W, 123 = left
arrow. SDL scancodes: 80 = W, 13 = J, 40 = RETURN, 26 = W.

The two button layouts disagree and must not be "fixed": on the wire A is
`0x0100` and START is `0x1000`; in the guest's own cpad record at `0x803A4E20` A
is `0x0100` and START is `0x0010`.

Guest records this loop reads: player pointer `0x803CA74C`; `demo_type` at
`+0x304`, `demo_mode` at `+0x314`, `proc` at `+0x31D8`, position at
`+0x1F8/+0x1FC/+0x200`; `event_mode` at `0x803C9EA2`, `event` at `0x803C9EB8`,
`msg` at `0x803CA7D2`; game pad pointer at `0x803A4DE0` with the button word at
`+0x18`; decoded left stick at `0x803A4DF0/+4/+8`; and the scene-transition
overlap object at `0x803F6160` (live when `>= 0x80000000` and `+0x04 == 1`), with
its phase at `+0x1C`.

Route timing from the same card: title 333, file-select 535, name-input-complete
712, opening-complete 13,850, play-scene 13,910, cutscene pages 17,868 → 19,773,
control 19,972. Host environment knobs: `BLUEWAKE_RENDERER`,
`BLUEWAKE_FRAME_TIMING`, `BLUEWAKE_CAPTURE_PLAYER_READY`,
`BLUEWAKE_PLAYER_PROBE`, `BLUEWAKE_TRACE_ROOM0`, `BLUEWAKE_INPUT_PROBE`,
`BLUEWAKE_PAD_*`, `BLUEWAKE_MAX_RETRACES`, `BLUEWAKE_CARD_PATH`.

## The report this loop owes the user

One sentence, in this shape: *Double-click BlueWake.app, press J once when PRESS
START appears, and the file menu comes up; the new-game intro costs X minutes and
Y reaches Outset; here is the screenshot of Outset with Link under your control.*
