# BlueWake goal loop — v16 (2026-09-14)

**User-started. Finish and test the app.** The terminal condition is unchanged:
from a double-click of BlueWake.app with no exported variables, a human reaches
controllable Outset gameplay with correct video and audio, drives it with the
keyboard without a crash, and time-to-playable is under five minutes, with the
five governing numbers holding on this host.

## What changed since v15: the pair is finished, and the fork resolved against
## its own instrument

V15 registered five predictions for the dynamic-cap run in flight at
`/tmp/bw-dyncap-b` and named a fork. The run is complete and the control at
`/tmp/bw-cap256-b` is complete. Reading them against each other does not
produce either branch v15 wrote. It produces a third, and it is the one that
unblocks the project.

**Over the whole route the two runs are the same run, except for the one line
the project already decided must not gate it.**

Both logs were reduced to their route records with the shipped
`scripts/route_digest.py` selector — 1,049 records each. The multiset
difference is one record each way. Removing that single record makes the two
sequences **byte-identical, order included: 1,048 of 1,048.** The record that
differs is `[cycle-delivery] summary`, whose `hash=` field is an aggregate
over `(cycle, cause, pc, context)`.

That is the circularity v14 tried to remove and did not finish removing. V14
introduced `[delivery-hash]` and asserted it "may not alter the accepted route
digest" — but by then the delivery-timing aggregate was already reachable inside
the record set, because `[cycle-delivery] summary` carries the same tag the
selector matches on. **So any change to interrupt-delivery timing fails the
route digest by construction, and v14's own analysis says the cap *is* delivery
granularity.** The gate could not have distinguished "the cap is wrong" from
"the cap moved an interrupt by two cycles." It was asked to.

The residual is exactly that small. Across all 146,755 external deliveries the
difference between the two runs is **2 cycles out of 114,210,000,002** — a
relative shift of 1.7e-11 — and 1,024 of 1,024 individually traced deliveries
are identical in cycle, cause, PC, context and order.

## The numbers, both runs, same host and composite and card

The two runs are admissible as a pair on their own evidence: identical
`host-sha256` `c99fc405…`, identical `comp-sha256` `6d7bca96…`, identical
card `6b43aabd…`, identical 14,100 retraces, and the *only* environment
difference is `BLUEWAKE_CYCLE_CAP`.

| number | cap-256 control | dynamic | reading |
| --- | --- | --- | --- |
| route records | 1,049 | 1,049 | — |
| records differing | — | **1** | `[cycle-delivery] summary` only |
| records identical | — | **1,048 / 1,048** | sequences equal |
| host turns | 820,034,526 | 521,121,740 | **−36.5%** |
| wall seconds | 757.04 | 661.34 | **−12.6%** |
| real-speed ratio | 0.3104 | 0.3553 | +14.5% |
| play-window wall (13910:14100) | 61.21 s | 25.07 s | **−59.0%** |
| play-window mean fps | 3.10 | 7.58 | **2.44×** |
| play-window median fps | 3.03 | 7.52 | **2.48×** |
| play-window p99 frame | 433.21 ms | 181.38 ms | **2.39×** |
| worst frame in window | 2.17 fps | 5.46 fps | control's p99 is 2.31 |
| time-to-playable | 695.8 s (11.6 min) | **636.3 s (10.6 min)** | −8.6% |
| peak RSS | 262,799,360 B | 258,899,968 B | none |
| final PC | `0x80307EF4` | `0x80307EF4` | identical |
| `play_scene` retrace | 13,910 | 13,910 | identical |
| delivery `external` | 146,755 | 146,755 | v15 pred 1 holds |
| delivery `no_cycle` | `D30E92D828D75F6B` | `D8174355C82195E7` | v15 pred 2 fails |
| delivery `cycle_sum` | `…758348` | `…758346` | v15 pred 3 fails (−2) |
| traced deliveries | — | **1,024 / 1,024 identical** | incl. the cycle |
| median fps floor | 3.03 | 7.52 | v15 pred 5 holds |

V15 predictions 1, 4 and 5 hold. Predictions 2 and 3 fail — and they fail by
**two cycles**, which is the measurement, not a route change. Prediction 2
conflated identity with the PC an interrupt lands on: an interrupt accepted two
cycles earlier sits on a different instruction, so `no_cycle` moves even when
the interrupt sequence, the milestones and the drawn frames do not.

## Hypotheses this loop owns

* **H-DELIVERY-IDENTITY** — status: **resolved, in favour of the cap.** The
  identity sequence is identical on every individually traced delivery and every
  guest-observable record; the whole-route residual is a bounded 2-cycle
  acceptance shift in the 145,731 untraced deliveries.
* **H-CAP-TRADE** — status: **holds.** The cap returns 36.5% of the host turns
  and 2.48× the play-window median fps for no change any route record can see.
* **H-TWO-GATE** — new, and the instrument consequence. The route gate must
  gate guest state; delivery timing is a *bounded* quantity with its own gate
  (`external` equal, DSP cadence equal, traced records equal, `cycle_sum`
  within `external × cap-difference`). A single hash that mixes both cannot do
  either job.
* **H-GUEST-DISPATCH** — unchanged and now unobstructed: 58–62% of both phases,
  flat, thin (top symbol 14.0%), and the only lever large enough to reach the
  terminal fps.
* **H-DSP-THREAD** — unchanged: 28% of the intro, ~7% of the play scene.

## The fork, and the decision it forces

**D1 — the delivery-timing aggregate leaves the route record set.** The
`hash=` field of `[cycle-delivery] summary` is not a guest-state record; it
is the interrupt-acceptance schedule, and v14 already decided that schedule must
not gate the accepted digest. The counts and the cadence fields stay in the
record; only the timing hash moves to `[delivery-hash]`, where v14 put it. The
baseline is then re-derived **once**, on this evidence, and must be produced by
**both** caps.

This is the 2026-08-31 class of decision — an owner fork with the evidence
attached — and it is *not* a re-record to pass: nothing else in the selector is
relaxed, and no run is accepted whose 1,048 guest-state records differ.
Weakening, skipping or re-recording a digest to make a specific run pass stays
forbidden on every branch.

## The iteration contract

An iteration counts only if time-to-playable fell, or fps moved on a named path,
or a human can now see, hear or do something they could not before. One
hypothesis, one small change, one build, one measurement, and at most one short
section appended to `docs/archive/status/CURRENT.md`. **A document is not an
iteration.** A profile is a cost paid to find the work, never progress by
itself. Report to the user every three iterations in one paragraph.

## Fences, non-negotiable

* Never commit, publish or expose Nintendo data, original binaries, generated
  game code, saves, captures, device data, signing material or leaked source.
  The user-owned GZLE01 stays private and out of git, every PPM and PNG stays in
  `/tmp`, and `ref/` is gitignored so an edit there ships as a patch under
  `patches/aurora/`.
* Preserve user data. The card at
  `~/Library/Application Support/BlueWake/GZLE01.card`, sha256 `b0163d86…`,
  is verified byte-identical to its backup before every run.
* One BlueWake process at a time. No disk-heavy work — repo-wide scans,
  `lldb`, `atos`, `otool` — while a measurement run is live.
* No silent stubs, no no-op substitutes, no weakening or deleting a test because
  it exposes a failure. **A synthetic button press is a route driver, not
  evidence that the keyboard works.**
* Never weaken, skip or re-record a digest to pass.
* Never write that no product gate advanced as a successful outcome; that
  sentence is the failure signature of this project.

## The work, in order

1. **Implement D1 and verify it on the two completed runs, before any new run.**
   The instrument change is script-only and needs no rebuild: the two logs
   already on disk must yield one identical digest, and it becomes the new
   baseline. If they do not, D1 is wrong and the cap is rejected on the
   evidence — that is the falsifier.
2. **Adopt the cap on the product path and re-run the benchmark once.**
   `scripts/bench.sh` and `scripts/play.sh` currently hardcode 256; the
   measured default becomes `dynamic`. Confirm the pass and record
   time-to-playable, which v16 expects near 636 s against 696 s.
3. **The play-window identity check that v15 owed**, now bounded rather than
   open: trace the deliveries that actually fall in 13910:14100 with
   `BLUEWAKE_DELIVERY_TRACE_PLAY` and confirm the recorded window is identical
   under both caps, closing the last window the recorded history cannot reach.
4. **H-GUEST-DISPATCH.** First measurement before any recompiler edit: the cost
   of a generated block and of one memory operation on this M2. The generated
   loop accounting (`ctx->downcount -= N` with `DOLRECOMP_C_LOOP_CYCLE_BUDGET`)
   and the per-instruction memory helpers (`mem_write32` → `get_ram_ptr` +
   `clear_matching_reservation` + a `g_mem_write_journal` global check +
   `write_be32`, `ref/recompcore/GXRuntime/include/core/cpu.h:291`) are the
   two named targets. Measure, then choose.
5. **H-DSP-THREAD** — 28% of the intro, about 7% of the play scene, holding the
   digest and the paced-PCM fingerprint.
6. **The owed v13 residue:** the controlled "one press moves Link N units"
   displacement test, which needs a player-position trace; then M1c, M2, M3 and
   H1/H2/H3 as recorded in v4.

## The report this loop owes the user

One sentence, in this shape: *Double-click BlueWake.app, press J once when PRESS
START appears, and the file menu comes up; the new-game intro costs X minutes
and Y reaches Outset; here is the screenshot of Outset with Link under your
control.* Silence is not acceptable; neither is a document about why it cannot
be done.
