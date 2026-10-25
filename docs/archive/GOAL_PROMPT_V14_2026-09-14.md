# BlueWake goal loop — v14 (2026-09-14)

**User-started. Finish and test the app.** The terminal condition is unchanged:
from a double-click of BlueWake.app with no exported variables, a human reaches
controllable Outset gameplay with correct video and audio, drives it with the
keyboard without a crash, and time-to-playable is under five minutes, with the
five governing numbers holding on this host.

## What changed since v13, and why this loop exists

V13 built on **H-DSP-DEADLINE**: make the device deadline terms bind inside the
cycle cap, and the measured 36.5% turn reduction becomes free. Reading that path
to the end shows the hypothesis as written cannot deliver it, and the reason is
more interesting than the DSP quantum. The coupling between the cycle cap and
the route digest is not a stale timestamp. It is the **turn boundary itself**,
and the deadline terms are a bystander.

## The coupling, stated exactly

* **The recorded cycle is already the true guest cycle.** The end of every turn
  runs `bluewake_cycle_domain_end_turn` (`main.c:10814`) and then
  `host_sync_cycle_devices_end_turn` (`main.c:10815`). The first zeroes
  `cpu->downcount` and folds the turn's elapsed cycles into
  `g_cycle_domain.absolute_cycles`; the second publishes device state. When the
  next turn then evaluates the interrupt guard
  (`main.c:4813`, `deliver_external_interrupt` at `main.c:4816`), `downcount`
  is zero and `absolute_cycles` is exactly the cycle the CPU is standing on.
  Nothing is stale and nothing needs adding: the digest is not reading a
  turn-start artifact.
* **The delivery decision is evaluated once per turn, on the PC the previous
  turn left.** The guard at `main.c:4813` is the only call site of
  `deliver_external_interrupt`, and it sits at the top of the turn, before that
  turn's `mod->dispatch` (`main.c:9198`). So the guest is asked *"do you accept
  an interrupt now?"* once per host turn, at whatever PC the previous turn
  happened to end on.
* **The cap therefore sets the granularity of that question.** A 256-cycle cap
  asks it four times as often as a 1,024-cycle cap. The same interrupt is
  therefore taken at a different guest cycle, and because the digest hashes
  `(cycle, cause, pc, context)` per delivery (`delivery_digest.c`,
  `main.c:1329`), the record moves. The DSP quantum is irrelevant to this: the
  deadline term reports `DSP_LLE_UPDATE_RATE - elapsed` with
  `DSP_LLE_UPDATE_RATE = 12600` (`main.c:403`, `main.c:448`), which cannot bind
  inside any cap this project allows, and making it bind would still leave the
  *external* delivery granularity equal to the cap.

The consequence is the point of this loop: **"make the device terms bind" cannot
make delivery cap-invariant, because the cap is what delivery granularity *is*.**

## H-DELIVERY-IDENTITY — the hypothesis this loop is built on

**The route invariant is the delivery *identity* — which interrupt, on which PC,
into which context, in what order — and not the cycle stamp of the turn boundary
that happened to accept it. If the identity sequence is identical under both
caps and the per-delivery cycle shift is bounded by the cap difference, then the
cap buys turns at a bounded, measurable interrupt-latency cost and nothing about
the guest's behaviour has changed.**

This is falsifiable in one pair of runs, and the instrument now exists:
`[delivery-hash]` reports a whole-route hash over `(cause, pc, context)` only,
plus the sum of all delivered cycles, and `BLUEWAKE_DELIVERY_TRACE=LO:HI` prints
individual deliveries so the two runs can be diffed record by record. Both are
deliberately tagged outside the route-record set, so neither can alter the
accepted digest.

## The test, and the predictions, written before the runs

Two runs of `scripts/bench.sh`, one process at a time, headless, same card
sha256 `6b43aabd…`, same 14,100 retraces, same host binary,
`BLUEWAKE_DELIVERY_TRACE=1024:1600`: one at cap 256 (control), one at
`BLUEWAKE_BENCH_CYCLE_CAP=dynamic`. Predictions, registered so the result can
falsify them rather than be narrated into them:

1. `[delivery-hash] external=` is identical between the runs.
2. `no_cycle=` is identical between the runs. **This is the hypothesis.**
3. `cycle_sum=` is *larger* under dynamic — a later acceptance — and the excess
   is bounded by `external × 1024`.
4. The cap-256 control reproduces the 2026-09-01 baseline *exactly*: digest
   `2b7a1fa5…`, **820,034,526 turns**. This is the control on the instrument
   itself: the pointer-based hash refactor must be behaviour-identical.

## The fork this loop resolves

* **Identity holds, shift bounded** → the divergence is a bounded
  interrupt-latency quantization, not a route change. Then the cap is adoptable
  *if and only if* it shows an fps win on the play scene against a live control
  (still owed from v13, and still owed here), and the promoted follow-on
  hypothesis becomes **H-DELIVERY-PHASE**: decouple the delivery-question
  granularity from the turn length, so long turns keep the delivery point a
  256-cap run would have had and the 36.5% is free rather than traded.
* **Identity does not hold** → the cap changes which interrupts the guest takes
  and where. Dynamic is rejected on the evidence, the result is recorded in one
  short section, and the work returns to the long pole (H-GUEST-DISPATCH).

Re-recording a digest so a run passes stays forbidden either way. A digest may
change only by the owner's explicit decision, framed as a fork, the way the
2026-08-31 rebaseline was.

## The numbers on this host

| number | baseline (cap 256) | dynamic, this loop |
| --- | --- | --- |
| route digest | `2b7a1fa5…` | pending |
| host turns | 820,034,526 | pending |
| delivery identity (`no_cycle`) | pending | pending |
| median fps, window 13910:14100 | 3.10 fps | pending, live control |
| **time-to-playable** | **12.5–15.3 min** | pending |

The turn count and the two delivery hashes are host-independent and are the
result. fps is quoted only from a run with a live control of its own.

## The iteration contract

An iteration counts only if time-to-playable fell, or fps moved on a named path,
or a human can now see, hear or do something they could not before. A profile is
a cost paid to find the work, never progress by itself. One hypothesis, one
small change, one build, one measurement, and at most one short section appended
to `docs/archive/status/CURRENT.md`. No status document may be longer than the change it
describes. Report to the user every three iterations in one paragraph.

## Fences, non-negotiable

* Never commit, publish or expose Nintendo data, original binaries, generated
  game code, saves, captures, device data, signing material or leaked source.
  The user-owned GZLE01 stays private and out of git, every PPM and PNG stays in
  `/tmp`, and `ref/` is gitignored so an edit there ships as a patch under
  `patches/aurora/`.
* Preserve user data. The card is at
  `~/Library/Application Support/BlueWake/GZLE01.card`, sha256 `b0163d86…`,
  backed up byte-identical to `/tmp/bw-usercard-backup.card`, and verified before
  every run.
* One BlueWake process at a time. No disk-heavy work — repo-wide scans, `lldb`,
  `atos`, `otool` — while a measurement run is live; it has dragged a live
  window to 1.94 fps before.
* No silent stubs, no no-op substitutes for required services, no weakening or
  deleting a test because it exposes a failure. **A synthetic button press is a
  route driver, not evidence that the keyboard works.**
* Never weaken, skip or re-record a digest to pass. A change that buys fps by
  moving the digest is a behaviour change until the delivery question above is
  settled by measurement.
* Never write that no product gate advanced as a successful outcome; that
  sentence is the failure signature of this project.

## The work, in order

1. **Run the pair and read `no_cycle` and the cycle shift.** Settle
   H-DELIVERY-IDENTITY. This is the loop's first measurement and it is one build
   and two runs.
2. **Re-measure the cap's fps properly, with a live cap-256 control**, and
   explain the play-scene window (2.41 against 3.10 fps). A cap that wins turns
   and loses the play scene is not a win on the phase that owns playability.
3. **H-DELIVERY-PHASE**, if and only if step 1 says identity holds: make the
   delivery-question granularity independent of the turn length, so the 36.5% is
   free instead of traded against interrupt latency.
4. **H-GUEST-DISPATCH.** 58–62% of both phases, flat, spread thin (top symbol
   14.0%). The long pole, and the only lever big enough to reach 46 fps. Measure
   per-instruction cost before touching the recompiler.
5. **H-DSP-THREAD.** 28% of the intro, about 7% of the play scene. Parallelize
   the adapter off the emulation thread, holding the digest and the paced-PCM
   fingerprint.
6. **The owed residue from v13:** a controlled "one press moves Link N units"
   displacement test, which needs a player-position trace; then M1c, M2, M3 and
   H1/H2/H3 as recorded in v4.

## The report this loop owes the user

One sentence, in this shape: *Double-click BlueWake.app, press J once when PRESS
START appears, and the file menu comes up; the new-game intro costs X minutes
and Y reaches Outset; here is the screenshot of Outset with Link under your
control.* Silence is not acceptable; neither is a document about why it cannot be
done.

