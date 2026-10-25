# BlueWake goal loop — v22 (2026-09-14)

**User-started. Finish and test the app.** The terminal condition is unchanged:

> From a double-click of `BlueWake.app` with no exported variables, a human
> reaches controllable Outset gameplay with correct video and audio, drives it
> with the keyboard without a crash, and time-to-playable is under five minutes,
> with the five governing product numbers holding on this host.

Run labels: v20 was retired deliberately (it had already emitted its complete
verdict and was then sitting in a frozen cutscene), and v21 was stopped early by
this loop after the defect below was proved from v21's own log. Both runs'
artefacts are kept — `run-v20-final.log`, `run-v21-final.log`,
`acceptance-v20.recorded.txt`, `acceptance-v21.recorded.txt`,
`v21-overlap-evidence.txt` — and the first run of this loop is labelled **v22**.

```
[v22 VOID] the harness died seconds after it launched the app and before its
first key press, so no input was ever delivered; the app sat unattended in the
title screen's attract demo for 21 minutes with demo_type=1 and demo_mode=4,
and the absence of control-admitted in that log measures the dead harness, not
the guest. Evidence: acceptance-v22.out stops at its third line, input.log is
0 bytes (send() never ran), and no app_acceptance_test.sh process existed while
the app ran orphaned under launchd. Kept as run-v22-void.log. The measurement
run is **v23**, launched as the foreground process of a persistent session so
the script outlives the shell that starts it.
```

## The defect that made every previous loop unreachable

v19 and v20 spent three loops on one clause and could not close it. The reason
is not the guest and not the keyboard. It is that **the gate the run was
measured with could not be satisfied by any guest behaviour, because the
instrument inside it never observed its own success condition.**

The gate was:

```c
const bool probe_control_state =
    player_valid && g_play_scene_reported &&
    player_overlap_phase_latch == 6u &&      // <- unsatisfiable
    probe_event_mode == 0u && probe_demo_mode == 0u;
```

`player_overlap_phase_latch` was the probe's own read of the scene-transition
overlap object at `0x803F6160`, taken whenever that object was live
(`mem_read32(0x803F6160) >= 0x80000000 && mem_read16(+0x04) == 1`) and latched so
a completed handover stayed visible after the object stopped being live.

v21's own log, 655 `[player-scene-state]` lines, is the refutation:

| phase | times the probe's latch held it |
| --- | --- |
| 0 | 2 |
| 1 | 3 |
| 2 | 11 |
| 3 | 9 |
| 4 | 23 |
| 5 | 607 |
| **6** | **0** |

At the same time the milestone instrument — the *same guest word*, read in the
overlap handler — reported phase 6 for **three separate handovers**:

| handover | phase history | reaches 6 at |
| --- | --- | --- |
| name/file-select | 3@459 4@463 5@517 → 6 | retrace 519 |
| title → file-select | 0@763 1@763 2@765 3@816 4@831 5@885 → 6 | retrace 887 |
| **play scene** | 0@13899 1@13899 2@13901 3@13951 4@13976 5@14031 → 6 | **retrace 14033** |

The two readings agree at every phase **except the terminal one**. They agree at
5 (probe 14031, milestone 14031) and diverge at 6 (milestone 14033, probe 5
forever after). The probe's read lands late in the retrace, when the handover's
object is no longer live; the handler-instant read sees phase 6 while the object
is still live, in the same retrace the transition completes. So the probe's latch
freezes one phase short, on *every* handover, structurally.

**Consequence.** `control-admitted` could not be emitted by that build, on any
card, in any run, however well the guest behaved. v21's verdict follows from
that and is not product evidence:

* `FAIL the guest's own player record never reached control` — the instrument,
  not the guest.
* `FAIL a real 6 s W hold left the guest's own player position unchanged` — the
  hold landed while the record still read `demo_type=1 demo_mode=512
  event_mode=2`, i.e. the awake cutscene, because the driver had no
  `control-admitted` to stop on and the script gave up when the process exited.
* `FAIL the player-ready capture never scheduled` — the same cause; the capture
  is armed from `player_control_admitted`.

The four clauses that passed in v21 pass for real and are unaffected: 262 real
key presses landed in the guest's own controller record, `gx-core` shut down
with zero rejects and zero failures, the LLE DSP produced nonzero audio, and the
user's own slot came back byte-identical.

The sentence for this loop:

> **A gate whose instrument cannot observe its own success condition reports the
> absence of the product, not the absence of the capability — and it reports it
> identically every time, which is why it survived three loops of "we tried
> again".**

## The fix, built and shipped

`runtime/host/src/main.c`, in the player probe:

* the gate reads `g_overlap_terminal_phase` — the phase latched from the same
  guest object when the overlap handler reports a change, which is the value the
  route's own control gate already reads at `main.c:6131`
  (`player_state_ready`). The probe's gate and the route's gate now agree **by
  construction** instead of by coincidence;
* the probe's own live read is kept and printed as a second field, so the two
  readings can never again be mistaken for one another:

```
[player-scene-state] retrace=697 ... stick=00000000,00000000,00000000 \
    ovl=4294967295 ovl_live=0
```

  The line above is from v22 before the latch is populated: `ovl=` is the
  route latch (`UINT32_MAX` until the name-scene object exists and the handler
  first reports), `ovl_live=` is the probe's own read. Nothing is conflated.

**This is not a weakening.** The criterion is unchanged — a completed handover, a
live player actor, and a cleared event mode. Only the reading of "the handover is
complete" is corrected, from an instrument that provably never reaches 6 to the
one that provably does and that the route already trusts. The 8-retrace hold,
the guard against `demo_mode 512`, and every other clause stand.

Bundle verification: `cmake --build build/runtime-host-dsp --target bluewake_host`
succeeded and re-signed the app; `strings
build/runtime-host-dsp/BlueWake.app/Contents/MacOS/BlueWake` contains
`ovl=%u ovl_live=%u`, and the app binary's mtime is after the patch.

## What v23 decides (running now; v22 is void above)

**Green** is: title → file-select → name-input-complete → new-game-intro →
play-scene; then

```
[player-milestone] control-admitted ... event_mode=0 demo_type=0 demo_mode=0 ovl=6
```

at a retrace at or after ~19,900 (the route's own timing for this card is control
at 19,972, first cutscene page 17,868, last 19,773); then a real 6 s W that moves
the guest's own `pos=` with `stick=y 3F800000`; a real RETURN and a real left
arrow delivered at Outset; a player-ready frame written; `[run] stopped:
normal`; `gx-core ... rejected=0 failed=0`; `dsp-lle ... first_nonzero=1`; and
the user's card byte-identical.

`ovl_live` is expected to read 0 or 5 in that line. That is the old probe
reading behaving exactly as documented above; it is printed for the record and
is **not** the gate.

## Hypotheses this loop owns

* **H-CONTROL-PHASE** — the gate, now on the route's own latch. Passes when
  `control-admitted` appears with `ovl=6 event_mode=0 demo_type=0 demo_mode=0`
  at retrace >= ~19,900 and a real 6 s W then moves `pos=` with
  `stick=y 3F800000`.
* **H-GATE-INSTRUMENT** — new, and it is the finding above. A gate is only as
  good as the instrument inside it; an instrument that cannot reach the value a
  gate requires turns a product question into a permanent false negative. It is
  the reason to prefer the route's already-proven latch over a second, subtly
  different read of the same object.
* **H-PLAYER-CONSUMES-STICK** — the next loop's question if the W hold still
  fails **while** `ovl=6` and `stick=y 3F800000`. That combination puts the
  fault downstream of input, in the player code, and must be reported in exactly
  those words rather than as a keyboard failure.
* **H-TTP-TRACE-TAX** — owns the second clause and is still unpaid. Two traced
  numbers exist: **356.96 s** (v20) and **464.55 s** (v21, play-scene at retrace
  13,960), both taken with `BLUEWAKE_FRAME_TIMING`, `BLUEWAKE_INPUT_PROBE`,
  `BLUEWAKE_TRACE_PLAYER`, `BLUEWAKE_TRACE_PAD` and `BLUEWAKE_TRACE_ROOM0` set
  and megabytes of stderr going to a file on a loaded host. The product claim is
  the double-click with **no** exported variables, so this clause is owed a clean
  run, with the traced numbers reported separately as traced numbers. Deleting a
  trace to make the clause pass would be weakening the test; quoting a traced
  number as the product's would be misreporting it.
* **H-LOAD** — carried. Every timing here is *indicative, not controlled* until a
  clean re-measure on an idle host.
* **H-TURN-COST**, **H-DSP-THREAD**, **H-CORPUS** — carried from v17/v18/v19/v20,
  still unpaid, to be priced as numbers first and changed second, in v17's order.

## Falsification on record

* **H-GATE-STATE** (`event_mode 0 && demo_mode 0` is sufficient) — falsified by
  v20: it fired at 13,985, in the seam.
* **H-DROPPED-KEY** (the W press never reaches the guest) — falsified positively
  and repeatedly: the guest's own decoded stick carries y `3F800000` under a
  held W.
* **H-SHORT-CEILING** — still falsified.
* **H-PROBE-LATCH-REACHES-6** — **falsified by v21** and it is the finding above.
  Four handovers, 655 probe lines, zero phase-6 readings, while the milestone
  instrument read 6 three times.

## The work, in order

1. **Read v22 out.** Do not read it live as a verdict; wait for the ceiling stop.
   Then write the verdict down with the line that carries it.
2. **If the W hold passes**, append one short section to `docs/archive/status/CURRENT.md`
   and send the report this loop owes.
3. **The clean time-to-playable run** (H-TTP-TRACE-TAX): the double-click path
   with no exported variables, read from the host's own `[frame-timing]` stamps,
   traced numbers reported separately.
4. **Then performance**, in v17's order: H-TURN-COST priced as a number before any
   build flag moves, then H-DSP-THREAD, then H-CORPUS.
5. **Owed residue:** the controlled "one press moves Link N units" displacement
   test (the instrument exists — `[player-scene-state]` carries the guest's own
   three position words); `scripts/bench_report.py:87` `window_metrics`
   off-by-one (`KeyError: 14097`); `scripts/bench.sh`'s overlap guard, which
   greps `bluewake_host` and so misses the bundle process `MacOS/BlueWake`;
   and a review of the **duplicate player-ready capture path** still present at
   `main.c` near the `0x80122D30` hook (`demo_type == 1 && demo_mode == 4`),
   which has never fired in any run and is not the path that arms the capture now
   — decide whether it is dead code to delete or a second real path.

## What not to do

* **Do not weaken the W clause**, substitute a synthetic stick, or accept
  `demo_mode 512/4` as control. A synthetic button press is a route driver, not
  evidence that the keyboard works.
* **Do not stop driving A while the cutscene is up.** It does not advance itself.
* **Do not "fix" the probe's live read by loosening its liveness test.** It
  observes what it observes; the gate's reading is the handler-instant one, and
  both are now printed.
* **Do not build the bare `all` target** — `tests/delivery_digest_test.c` has a
  pre-existing unrelated break. Build `bluewake_host`.
* **One BlueWake process at a time.** The acceptance script refuses to overlap
  and its guard matches the bundle path.
* **Do not read a run that is still live as a verdict.** Wait for the ceiling stop.
* **Do not quote a traced timing as the product's**, and do not delete a trace to
  make a timing clause pass.
* **Do not report v21's FAIL clauses as product failures.** They are the
  instrument defect above and are void; the runs are kept precisely so this can
  be checked.

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
  game code, saves, captures, device data, signing material or leaked source. The
  user-owned GZLE01 card `b0163d86...` stays private and out of git; the walk
  runs against a **copy** of the canonical route card `6b43aabd...` and must
  return byte-identical. Every PPM and PNG stays in `/tmp`, and `ref/` is
  gitignored so an edit there ships as a patch under `patches/aurora/`.
* No disk-heavy work — repo-wide scans, `lldb`, `atos`, `otool` — while a
  measurement run is live. Any guard has to match the bundle path too.
* No silent stubs, no no-op substitutes, no weakening or deleting a test or a
  digest because it exposes a failure.
* Never write that no product gate advanced as a successful outcome.

## Appendix — the constants this loop relies on

Keyboard → pad: arrows are the D-pad, **J = A**, K = B, U = X, I = Y, **W/A/S/D
= the left stick**, H/F/T/G = the C-stick, E/R = L/R, Q = Z, RETURN = START.
Physical keycodes used by the test: 38 = J (A), 36 = RETURN, 13 = W, 123 = left
arrow. SDL scancodes: 80 = W, 13 = J, 40 = RETURN, 26 = W.

The two button layouts disagree and must not be "fixed": on the wire A is
`0x0100` and START is `0x1000`; in the guest's own cpad record at `0x803A4E20`
A is `0x0100` and START is `0x0010`.

Guest records: player pointer `0x803CA74C`; `demo_type +0x304`, `demo_mode
+0x314`, `proc +0x31D8`, position `+0x1F8/+0x1FC/+0x200`; `event_mode
0x803C9EA2`, `event 0x803C9EB8`, `msg 0x803CA7D2`; game pad pointer
`0x803A4DE0` with the button word at `+0x18`; decoded left stick
`0x803A4DF0/+4/+8`; scene-transition overlap object `0x803F6160` (live when
`>= 0x80000000` and `+0x04 == 1`), phase at `+0x1C`.

Two latches, and they are not interchangeable:

| name | written by | observed at | reaches 6 |
| --- | --- | --- | --- |
| `player_overlap_phase_latch` | the player probe | probe time, late in the retrace | **no** |
| `g_overlap_terminal_phase` | the overlap handler hook | handler instant, mid-transition | **yes** |

Route timing on this card: title 333, file-select 535, name-input-complete 712,
opening-complete 13,850, play-scene 13,910, cutscene pages 17,868 → 19,773,
control 19,972. Env knobs: `BLUEWAKE_RENDERER`, `BLUEWAKE_FRAME_TIMING`,
`BLUEWAKE_CAPTURE_PLAYER_READY`, `BLUEWAKE_PLAYER_PROBE`, `BLUEWAKE_TRACE_ROOM0`,
`BLUEWAKE_INPUT_PROBE`, `BLUEWAKE_PAD_*`, `BLUEWAKE_MAX_RETRACES`,
`BLUEWAKE_CARD_PATH`.

## The report this loop owes the user

One sentence, in this shape: *Double-click BlueWake.app, press J once when PRESS
START appears, and the file menu comes up; the new-game intro costs X minutes and
Y reaches Outset; here is the screenshot of Outset with Link under your control.*
