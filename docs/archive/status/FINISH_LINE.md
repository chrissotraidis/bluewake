# BlueWake macOS Finish Line

**Recorded:** 2026-08-30  
**Priority (updated 2026-09-23):** iPadOS is the active product target at the
user's direction, proven in the iOS simulator one device at a time; macOS stays
the reference host. See [IPADOS_REORIENTATION_2026-09-23.md](IPADOS_REORIENTATION_2026-09-23.md).
The macOS items below still apply to both platforms because they share the host.

## Honest Product State

> **Current priority — 2026-09-09:** The user resumed the Route B whole-unit
> loop. Follow `REORIENTATION_2026-09-09.md`: reproducible stage/world composition,
> actual J3D output, live input/world session, original scheduler/Painter, then
> normal boot/transition/save/audio. Route A measurements and priorities below
> are historical oracle evidence. Route B has recorded native 2D presentation,
> but native 3D gameplay remains unqualified. Full macOS acceptance precedes mobile.

> **Reoriented 2026-09-01:** BlueWake is a working compatibility-runtime
> research vehicle, not yet a stable playable macOS game. The current promoted
> Outset route runs about 235 seconds of guest time in 343.88 seconds, roughly
> 68% real speed before frame-pacing qualification. Historical Omasao/save evidence
> remains valuable, but that automation route is not currently reachable under
> the promoted cycle contract. See `TECH_DEBT.md` for the governing debt order.

BlueWake has crossed the feasibility boundary: the signed native macOS app
boots original retail code into controllable gameplay, persists a save, and
completes an authentic Outset-to-Omasao scene transition. The remaining work
is no longer best described as "make the game launch." It is to turn a slow,
visibly imperfect compatibility prototype into a dependable application.

The user-observed Omasao presentation, including the August 30 screenshot,
establishes two first-class product defects:

1. Visible frame rate and frame pacing are far below the original game.
2. Link's hair renders with incorrect geometry/material appearance.

Neither is cleared by a long crash-free run. Correctness, responsiveness, and
endurance require separate evidence.

## Definition of a Credible macOS Build

- Clean-machine preparation, translation, composite, host build, signing, and
  launch are reproducible from documented commands and user-owned `GZLE01`.
- Normal visible play reaches title, file selection, Outset, and Omasao without
  test-only state injection or an opt-in correctness workaround.
- Link, major environment materials, transparency, depth, and scene composition
  are visually credible across the accepted route.
- Representative gameplay sustains an explicit frame-time target with stable
  pacing; diagnostics-off release behavior is measured separately from traced
  acceptance builds.
- Audio is intelligible and continuous under human review, with no sustained
  underrun, runaway latency, or obvious synchronization failure.
- Keyboard and a physical controller support normal unrestricted play and
  reconnect/lifecycle behavior.
- Save, quit, relaunch, reload, door transitions, and multiple scene changes
  pass on an isolated copied card.
- A release-scale visible soak completes with bounded memory, acceptable
  thermals, no runtime rejection, and an unchanged card when no save is issued.
- Known limitations are explicit; a distributable build contains no Nintendo
  data and provides a clear user-owned-disc onboarding path.

## Prioritized Work

### 1. Measure the visible frame (completed for current route)

Use one representative Omasao capture window and profile it before changing
code. Attribute wall time among translated dispatch, guest scheduling, DSP,
GX command processing, shader/pipeline creation, texture upload, presentation,
and diagnostic invariants. Record median and tail frame time plus CPU, memory,
and thermal state. Keep correctness diagnostics available, but measure the
shipping configuration independently so instrumentation cost is not mistaken
for architecture cost.

Result: scene profiles selected DSP first, then GX. The promoted exact-eight
DSP path reduced the full route 2.8%; the subsequent GX profile rejected
allocation churn and assigned repeated full texture-content hashing. The
promoted dirty-generation contract removes over 99% of those hashes but yields
only about 1.4% title-route CPU improvement. A fresh profile and fixed
1,024-cycle probe then assigned a broader translated-host dispatch owner, but
failed exact lazy-FPU delivery. Compiling dormant developer probes out instead
preserves the exact 256-cycle route and cuts matched title-route user CPU by
19.4%. The next step is a visible shipping-build remeasurement; a dynamic
budget is only a candidate if that profile still assigns dispatch ownership.

That remeasurement now passes the exact Omasao route. The light opening is
near 29 fps, Outset is 6.0 fps median with a 2.5 fps tail, and Omasao is 13.55
fps median. Whole-route wall time is 22.4% below the prior exact-eight build.
This clears the remeasurement action, not the performance gate; a fresh native
sample must rerank the remaining owner before code changes.

The fresh trace-off sample assigns roughly half of both scenes to translated
dispatch. GX frontend/sink work is about 12% in each; DSP is 11.9% in Outset
but 30.5% in Omasao. The default-on cycle-domain observation/delivery contract
and guest-owned DSP completion now pass the short cap-invariance tier. An exact
DSP idle candidate remained inactive, while the old Omasao automation no
longer reaches its first post-ladder waypoint under the corrected runtime.
Route tuning and DSP signature tuning are closed. The review's chunk-table
publication tier is also closed: it reduced turns only 11.2%, failed the short
digest, and imposed prohibitive optimized compile cost. The next bounded
performance census now closes chunk chaining: 87.1% of returns are budget
exits and only 0.0075% are eligible chunk entries. The dominant one-cycle
cache-flush clamp can remove 86.4% of turns, but first-delivery drift reopens
the cycle contract before that mechanism can ship.

### 2. Repair Link's hair at the GX owner

Treat the visible defect as a focused graphics-correctness blocker. Identify
the affected model draw, material, texture, TEV state, alpha compare/blend,
culling, depth, and matrix state; compare the submitted state with the retail
display list and a trusted reference renderer. Fix the owning GX translation
contract rather than patching Link or the model. Add a small state-level
regression and a repeatable screenshot checkpoint that stores no private game
data in Git.

Exit condition: the accepted Omasao view renders Link's hair credibly without
regressing opaque geometry, transparency, or depth ordering elsewhere.

### 3. Convert performance findings into bounded optimizations

Optimize only measured owners. Preserve an exact correctness route alongside
each candidate, compare matched run order, and reject speedups that alter guest
milestones, state digests, device outcomes, or rendered behavior. Likely work
areas include dispatch overhead, repeated host boundary crossings, GX batching
and state caching, shader compilation/caching, texture residency, and release
diagnostic policy; none is presumed guilty before profiling.

Exit condition: representative visible play reaches the recorded frame-time
budget with matching correctness evidence.

### 4. Broaden product acceptance

Complete physical-controller and unrestricted-play sessions, human audio
review, repeated save/reload, and additional retail scene transitions. Promote
new runtime behavior only when authentic evidence assigns ownership. Keep one
active blocker and use the goal-loop anti-stall rules.

Exit condition: the P4+ acceptance set in `GATES.md` is complete on macOS.

### 5. Package the macOS path

Verify a clean local rebuild, stable configuration defaults, strict signing,
crash diagnostics, and user-owned-disc onboarding. Define supported Apple
Silicon hardware from measurements rather than aspiration. Notarization and
distribution policy follow only after the compatibility build is credible.

Exit condition: another Mac can reproducibly build and launch the same
game-data-free application using its owner's supported disc image.

### 6. Re-evaluate mobile

Only after the macOS gates above pass, profile the authentic route on physical
iPhone and iPad hardware. Simulator evidence cannot certify performance,
thermals, audio lifecycle, controller behavior, touch ergonomics, signing, or
memory pressure.

## What to Do Differently

- Use short, scene-specific correctness reproductions before multi-hour runs.
- Profile representative visible gameplay before selecting optimizations.
- Maintain distinct correctness, performance, and endurance configurations and
  ledgers; do not let one result stand in for another.
- Build a private visual-regression corpus around stable camera/viewpoints and
  publish only game-data-free state assertions and hashes.
- Compare graphics state at the narrow owning draw instead of tracing broad
  downstream symptoms.
- Retire experiments when their shape repeats three times; promote a subsystem
  or reframe the blocker instead of moving the same trace one boundary earlier.
- Keep commits small, signed artifacts identified, status current, and `main`
  backed up whenever a stable evidence point is reached.

## Immediate Sequence

1. Freeze capture timing, route tuning, and closed sub-5% optimization shapes.
2. Preserve the promoted exact hybrid O2 baseline and the scheduler-owned
   interrupt-eligibility contract.
3. Preserve the rejected generated-side filter evidence; all translated-C edge
   chaining is now closed.
4. Perform the one exact-LLVM v27 slowdown profile selected by
   `EXECUTION_ROUTE_REVIEW_2026-09-01.md`, then continue only for a bounded
   architecture-scale owner.
5. After a material gain, run one unscripted human macOS session and record
   input, camera, audio, pacing, and direct Link visual observations.
6. Repair the regressed P0 evidence policy, then proceed to product shell,
   broad compatibility, and final-speed endurance.
