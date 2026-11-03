# BlueWake - Measured Performance Results

**2026-09-24 actor search by id runs natively (host):** certified 280.0 -> 218.7 M instructions per play
retrace (-21.9%), digest 83d2590d and stop rows unchanged; rendered 482 -> 450 M; simulator heavy view 54.0 ->
57.4 retraces a second (median 16.8 ms). See CURRENT.md.

**2026-09-24 the GroundCross observation leaves the turn loop (host):** certified 291.8 -> 280.0 M instructions per
play retrace (-4.0%), turns in the window 5.25 M -> 0.62 M, digest unchanged; rendered 497 -> 482 M. See CURRENT.md.

**2026-09-24 frameless edge-service front (host):** certified 300.7 -> 291.8 M instructions per play retrace
(-3.0%), digest 83d2590d and turn counts identical; rendered 509 -> 498 M. See CURRENT.md.

**2026-09-24 GX FIFO writes without the device sync (host):** certified play window 334.0 -> 300.7 M
instructions per retrace (-10.0%), route digest 83d2590d unchanged; rendered save view 541 -> 509 M (-5.9%),
frames identical; host_mmio_write 10.1% -> 1.0% of the simulator's game thread. See CURRENT.md.

**2026-09-24 GX worker (recompcore 0073, 0074):** reused render packet 568 -> 542 M rendered instructions per
retrace in the save's Outset view (-4.6%); HUD texture re-uploads 28,144 -> 194 per 1,800 retraces. Simulator
worker idle 14 -> 28.5%, game-thread wait on it 4.5 -> 1.7%. **Refuted:** native dispatch of __save_gpr /
__restore_gpr (334.1 -> 334.0 M certified, digest unchanged). See CURRENT.md.

**2026-09-24 gather-pipe batch buffer (recompcore 0072):** 570/571 -> 568/568 M rendered instructions per retrace
in the save's Outset view. **Refuted:** exact inline FP fast paths (+0.2% certified window, null rendered). See CURRENT.md.

**2026-09-23 palette fix without a second texture event (recompcore 0069):** simulator heavy Outset view
48.1 / 47.3 retraces a second (median 21.1 ms) after 0066 had dropped it to about 33; 0068 render-scale copies
measured free. See CURRENT.md.

**2026-09-23 runtime/dispatch-object PGO:** 340.4/340.7 -> 333.3/332.9 M instructions per play retrace (-2.2%),
cycles 58.4/58.3 -> 57.9/56.9 G, guest digest unchanged. See CURRENT.md.

**2026-09-23 PGO host:** play-window cycles 60.7/61.7 -> 56.6/57.9 G (-6.5%), digest unchanged; simulator heavy
Outset view 47.5 retraces a second. See CURRENT.md.

**2026-09-23 inline FP/paired-single helpers + wider PGO:** 362.6 -> 341.6 M instructions per play retrace
(-5.8%), digests unchanged; simulator heavy Outset view 46.3 retraces a second. Supersedes the
2026-08-29 wall-time rejection of the inline FP check. See CURRENT.md.

**2026-09-23 PGO of the hot chunks:** play-window cycles 76.6/79.5 G -> 61.6/64.0 G (-19.6%), IPC 3.93 -> 4.71,
digests unchanged; bench.sh headless median 53.6 retraces a second. See CURRENT.md.

**2026-09-23 uniform reuse and vertex staging (recompcore 0062):** Aurora staging splits 849 -> 0 in the
simulator's heavy Outset view, about 21 -> 34.5 retraces a second; macOS windowed bench window 40.6 a
second (median 26.6 ms). See CURRENT.md.

**2026-09-23 high-level DSP (iOS default):** play-window instructions 376.7 M -> 321.5 M per retrace
(-14.6%), cycles 81.8 G -> 69.7 G (-14.8%); audio envelope r = 0.999 against LLE. See CURRENT.md.


**2026-09-23 guest alias index (recompcore 0060):** play-window instructions 396.7 M -> 376.7 M per
retrace (-5.0%), cycles 88.9 G -> 81.8 G (-8.0%), digests unchanged; iPad simulator heavy Outset view
17.6/18.4 -> 20.2/20.4 retraces a second. See CURRENT.md for the table.

**2026-09-09 sample-clock diagnosis:** three headless paced cursor runs each
return 4,262 nonzero samples and the same private PCM fingerprint with no
computed original load-shedding threshold crossings. Three burst runs vary
(1,256 / 3,862 / 4,262 samples; 7 / 5 / 2 crossings). Consuming 8,960 frames at
the original 32,028.5-Hz cadence takes approximately 0.2874 seconds including
last worker completion, versus approximately 0.10 seconds in burst mode.
The runtime clock and load-shedding rule are unchanged. This supports the burst
consumer as the source of observed variability; it is not device timing,
independent fidelity, video/audio synchronization or original-speed acceptance.

**2026-09-09 original requested PCM (0260–0264):** cursor sound admission,
track execution and retirement pass through 16 DMA buffers / 112 DSP subframes.
Recorded positive runs contain 1,072, 4,262 and 1,636 nonzero samples; this is
variable output, not a deterministic fingerprint or audio fidelity result.
Idle/suppressed controls contain zero. Original DSP load-shedding messages remain
visible. Its relationship to the host clock and burst consumer needs measurement
before a device-clock or original-speed claim. Twenty-one strict regressions
pass, including 180 world frames. No output device or GUI was opened.

**2026-09-09 original game audio initialization (0251):** the actual SE
sequence is 115,456 bytes. It exceeds the generic 65,536-byte stay heap but fits
the original game initializer's 118,784-byte setting. The game probe uses the
same diagnostic 8-MiB parent arena and the framework's original ARAM partitions
(8 MiB audio, 6 MiB graphics) and service priorities. Fourteen strict regressions
pass; the game probe loads and byte-compares the sequence but stops before track
execution. No device clock, throughput, product heap-budget, full-interface
teardown or original-speed acceptance is implied. The older unguarded base
interface run's parent-heap recovery failure remains unresolved.

**2026-09-09 original AAF ownership (0245–0246):** the complete original
driver consumes 474,844–474,944 bytes across the recorded runs. Complete private
AAF loading alone consumes 1,097,176 bytes; driver + AAF + FX retain
1,600,528–1,600,592 bytes across both startup modes. File mode also holds
a temporary 540,416-byte AAF while constructing its owners, explaining the
observed exhaustion of the 2-MiB driver-only arena. Its diagnostic uses 8 MiB;
the game-style externally supplied memory path passes in 2 MiB and retires that
external buffer before effects/playback. These are measured subsystem allocations,
not full interface/game memory acceptance or a change to the product heap budget.
Original DSP load-shedding remains enabled; burst-driven nonzero sample counts
vary. No new device timing, audio fidelity, presentation or original-speed claim.

**2026-09-09 original effects (0244):** all four native effect/filter modes
pass exact scalar controls over four ring wraps. The synthetic original-thread
run first returns wet output after 168 samples, including the inherited eight
history samples; the private effect configuration produces a left-channel tail.
All three lifecycle modes complete 16 DMA buffers / 112 DSP subframes with
producer backpressure. These checks establish processing and ownership, not
real-time latency, hardware parity or device-clock performance. Original DSP
load-shedding remains enabled; burst-driven sample counts can vary. The rebuilt
headless world completes 180 iterations; no new presentation or speed claim.

**2026-09-09 original audio lifecycle (0243):** strict headless execution
completes 16 DMA buffers and 112 DSP subframes through original JAIBasic/JAS.
The consumer waits for producer completion; this establishes ordering, not a
sample-clock rate or real-time deadline. The unpaced initial test underruns and
repeats samples through the original DAC fallback. Original DSP load-shedding
also reports attempts to break an already free channel after the voice ends.
No heuristic was disabled; device cadence, real-time load and audible quality
remain unqualified.

**2026-09-09 native DSP candidate:** 32 original waves produce 163,840 stereo
PCM frames in the private probe. This is data-flow evidence; the run does not
measure real-time deadlines, output latency, underruns, device behavior or full
audio/game speed. No new performance gate or throughput claim.

**2026-09-09 Aurora 0008:** original display depth and 16 GPU Z-texture controls
pass, and the rebuilt scripted world submits 180 Metal frames. No original-speed
claim. Z bias is uniform state rather than a distinct pipeline per value. Required
shader compilation still blocks; an exploratory original-display run hit its
one-second watchdog, while the final isolated-cache run passes. Cold-load
watchdog reliability under host contention remains unqualified.

## 2026-09-09 — Required shader compilation

Aurora 0007 waits for required GX and clear pipelines, using the existing blocking
pipeline path. A traced original display run dropped draws while their shaders
compiled and copied a black frame; an empty-cache replay now preserves the first
red clear. This favors correct pixels and can introduce cold-cache stalls.
No original-speed or frame-pacing acceptance is claimed. The original display
probe still fails its separate clear-depth oracle; see the world integration log.


## 2026-09-09 — Native display-copy pixels

The synthetic Metal display-copy probe submits five frames and reads GPU texture
pixels at the observed 1280×960 backing resolution. Window captures independently
confirm selected-XFB red output and VI black state. Ten-second observation holds
are diagnostic only. This establishes copy/presentation behavior for this fixture;
it provides no FPS, original-speed gameplay, filtering or full-frame fidelity claim.


## 2026-09-09 — Cooperative native retraces

The native VI bridge uses a 60 Hz cadence for non-PAL modes and 50 Hz for PAL,
measured against Aurora game time. It coalesces missed retraces at owner-thread
service boundaries. Worker waits do not execute video callbacks. Focused tests
verify paused time and lifetime/order, not physical scanline timing, exact NTSC
frequency, interrupt latency, original-speed gameplay or presentation pacing.
The later display-copy experiment verifies pixels; full original frame ownership
and presentation pacing remain open.

## 2026-09-09 — Native startup alarm timing

Focused strict tests verify deadline ordering, pause/scale behavior and sleeping
thread wakeup, including actual Aurora game time. Pending alarms resample that
clock at one-millisecond host intervals; native scheduling may deliver later.
No interrupt-latency, CPU-overhead or original-game-speed qualification is claimed.
Full display/frame ownership and sustained gameplay performance remain open.
See `ROUTE_B_WORLD_INTEGRATION_2026-09-09.md`.

## 2026-09-09 — Route B renderer bring-up is not performance qualification

The current 180-frame world diagnostic submits to Metal with an explicit 30 Hz
sleep and sanitizer instrumentation. Link/world/follow-camera presentation is
now observed after focused repairs, with full fidelity still open. Submission
count, configured pacing and this bounded run provide no original-speed,
sustained gameplay or thermal acceptance. Establish correct world pixels/live
execution before performance measurement; Route A's historical oracle remains
frozen. Details: `ROUTE_B_WORLD_INTEGRATION_2026-09-09.md`.

## 2026-09-01 - Current Host Does Not Rehabilitate LLVM v27

- A strict diagnostic ABI-4-prefix adapter passes one retrace and 700
  retraces; title-ready remains 333 and the copied card is unchanged.
- The current host reduces the historical sampled wall time from about 31
  seconds to 25.83 seconds, with 25.00 seconds user CPU.
- The candidate stops after 105,544,680 blocks at `0x80245664` and lacks the
  current generated cycle-observation suffix. It is not current-route exact.
- The result offers no plausible 1.5x end-to-end gain, so current LLVM
  regeneration is rejected and the temporary ABI adapter is removed.
- Evidence:
  `local-research/evidence/llvm-v27-current-host-prefix-v1-20260901/`.

## 2026-09-01 - Exact LLVM v27 Slowdown Decomposed

- The hash-matched v27 artifact reproduces its historical 700 route under the
  source-compatible host in 31 sampled wall seconds.
- Of 22,745 samples, old monolithic host `main` owns 40.7%, the old DSP branch
  23.5%, old REL alias handling 9.8%, and dispatcher plus hottest generated
  budget loop 9.8%.
- The old aggregate comparison did not isolate LLVM execution. One strict
  current-host prefix-ABI tier is authorized before a current regeneration.

## 2026-09-01 - Inline Edge Filter Closes the Chaining Route

- Live PI state, a sorted 52-address index, and the dynamic REL base eliminate
  ordinary-edge host callback entry.
- The focused 388 deliveries are exact. The 700 tier matches the prior pure
  callback's milestones, totals, first 1,024 deliveries, 24,070,695 turns, and
  late aggregate hash.
- User CPU is 23.18 seconds versus 23.02 for the every-edge callback. The
  expected callback-overhead gain does not exist at end-to-end scale.
- The activation is removed. The route review selects one exact-LLVM v27
  slowdown decomposition profile; translated-C edge tuning is closed.

## 2026-09-01 - Pure Eligibility Callback Is Still Too Expensive

- A pure semantic/observation predicate plus scheduler-owned interrupt
  eligibility makes the first 388 deliveries exact and recovers the complete
  700 route.
- Turns fall 61.2%, from 62,038,491 to 24,070,695, but user CPU falls only
  13.4%, from 26.57 to 23.02 seconds.
- The aggregate delivery hash diverges only after the first 1,024 exact
  records. The candidate remains rejected under the unchanged oracle.
- Both callback forms are closed. The next performance tier must reject
  ordinary edges inline and avoid entering host code unless a candidate is due.


## 2026-09-01 - Per-Edge Service Recovers Route, Not Enough CPU

- Absolute-cycle host service at every chained edge reduces 700-route turns
  from 62,038,491 to 20,096,479 (67.6%).
- Every gameplay/subsystem summary and the first 1,024 delivery records match,
  but the aggregate delivery hash diverges later. The candidate is rejected.
- User CPU improves only from 26.57 to 23.88 seconds (10.1%); the edge callback
  and device publication consume most of the raw turn-removal gain.
- This closes per-edge host service as the finish-line optimization. A viable
  successor must keep ordinary edges local and service only actual deadlines.


## 2026-09-01 - Selective Edge Yield: Fast but Rejected

- The known semantic intercepts plus fixed route-observation PCs reduced the
  700 tier from 62,038,491 to 22,321,565 host turns (64.0%). Runtime was 12.89
  seconds wall / 12.30 seconds user.
- The result is inadmissible for performance promotion. Title-ready, scene
  draws, and gameplay/collision milestones were absent; external/DSP delivery
  counts fell to 7,050/6,338 from 11,477/10,281.
- The first external delivery shifted 117 cycles and changed PC, so the
  remaining owner is edge-quantized CPU runtime observation/delivery, not the
  cost of the callback or an incomplete performance address list.
- The >=40% throughput opportunity remains demonstrated, but inaccessible
  until edge topology cannot move an architectural delivery.


## 2026-09-01 - Full generated-edge loop proves speed and rejects semantics

- The post-O2 return census records 62,096,587 exits: 56.6% cross-chunk,
  31.2% budget, 11.9% same-chunk, 0.22% outside code, 0.09% exception, and
  0.265% directly eligible chunk entries.
- A composite loop spanning calls, returns, indirect branches, and same-chunk
  exits reduced the 700-retrace route from 62,038,491 to 22,048,210 turns
  (64.5%) and from 26.61 to 10.60 user seconds.
- The candidate is rejected: it produced 722/1,050 route records, 718/1,046
  invariants, no title-ready milestone, and zero DSP deliveries. The control
  delivered 10,281 DSP interrupts and completed 71,538 DSP DMAs.
- The speed result validates generated-edge dispatch as the major owner. The
  correctness result proves the full host turn contains an interception
  contract not represented by budget/exception state. The next mechanism must
  expose that host-owned decision to chained generated edges.
- Evidence:
  `local-research/evidence/post-o2-return-census-v1-20260901/` and
  `local-research/evidence/composite-dispatch-loop-v1-20260901/`.

## 2026-09-01 - Publication-only end-turn sync promotion

- Interrupt source publication is now separate from dispatch rebudgeting.
  End-turn sync publishes only; observations and dispatch preparation retain
  exact budgets.
- All 215 tests pass. The 700 and Outset candidates match all 1,050 route
  records, all 1,046 cycle invariants, turn totals, normal stops, and canonical
  cards.
- Outset takes 343.88 seconds wall / 346.16 user versus 356.06 / 352.29, a
  3.4% wall and 1.7% user reduction.
- Decision: promote the cleaner exact contract, but close further deadline and
  cap variants. Generated edge/dispatch architecture remains the largest owner.
- Evidence:
  `local-research/evidence/interrupt-publication-budget-v1-20260901/`.

## 2026-09-01 - Exact hybrid O2 Outset owner profile

- The visible 14,500-retrace run stops normally after 983,291,014 turns in
  415.62 seconds wall / 418.15 user / 6.73 system.
- Its 1,050-record digest is exactly `3a70982a...`, matching the accepted
  post-lifecycle route, and the copied card remains canonical.
- Generated dispatch accounts for about 4,285/7,652 active-main samples
  (56.0%), down from 99.1% before O2 but still the largest aggregate owner.
- Explicit host leaves include `host_cycle_deadline_distance` at 501/7,814
  main-thread samples (6.4%) and donor DSP `Step()` at 334 (4.3%).
- The profile led to a static contract correction: end-turn interrupt-source
  publication still triggers an unusable rebudget through
  `bluewake_interrupt_sources_refresh`. The next bounded A/B separates source
  publication from dispatch budgeting; it is not another cap or deadline-source
  sweep.
- Evidence:
  `local-research/evidence/legacy-hybrid-o2-profile-v1-20260901/`.

## 2026-09-01 - Cycle-domain lifecycle deduplication improves Outset route

- All 215 registered tests pass, including callback-count assertions proving
  deadline scans occur at dispatch preparation but not turn begin/end.
- The candidate matches the promoted 1,050-record Outset digest exactly, with
  canonical card, 820,034,526 turns, and final PC `0x80307EF4`.
- The two qualified controls average 634.46 seconds wall / 560.60 seconds user
  CPU. Candidate is 540.14 / 512.38 seconds: 14.9% lower wall and 8.6% lower
  user CPU.
- The candidate is promoted. Exact pre-dispatch budgeting and observation-time
  rebudgeting remain unchanged.
- Evidence:
  `local-research/evidence/cycle-lifecycle-dedup-v1-20260901/`.

## 2026-09-01 - Current Outset profile selects cycle-domain lifecycle overhead

- A visible 14,100 run matches the promoted route digest exactly. Its sample
  attached too near exit and was empty, so a profile-only 14,500 retry changed
  only the stop bound and captured ten seconds beginning at retrace 13,987.
- Of 5,244 active-main samples, `host_cycle_deadline_distance` owns 468 (8.9%),
  DSP `Step()` 186 (3.5%), and GX draw-plan construction 67 (1.3%); generated
  guest code remains the largest aggregate owner.
- Every translated dispatch is explicitly prepared, but the ordinary turn
  lifecycle also performs two full deadline calculations whose results cannot
  be consumed by translated code: at turn begin and after final flush.
- The next A/B removes only those unused calculations while retaining exact
  pre-dispatch budgeting and the promoted Outset digest.
- Evidence:
  `local-research/evidence/outset-native-profile-v1-20260901/`.

## 2026-09-01 - Deterministic Outset performance route qualified

- Two serialized cap-256 runs enter live Outset at retrace 13,910 and stop
  normally at 14,100 after exactly 820,034,526 host turns at `0x80307EF4`.
- Their predeclared 1,050-record route digests are identical:
  `2b7a1fa575b62a2eece2f1ed860542953a33689b4ca922b1a62cc644deab4e2b`.
  The independent invariant checker also matches all 1,046 records, and both
  copied cards remain canonical.
- Each run records 168 Outset background draws, 179 DVD forwards/0 failures,
  4,638 ARAM transfers/0 rejected, and 825,494 DSP DMAs.
- Wall/user times are 647.04/555.70 and 621.88/565.50 seconds. Timing is
  excluded from the correctness digest and remains measurement data.
- This is now the standard gameplay-scale performance A/B tier. A fresh native
  sample of its live Outset window must rank the next CPU owner.
- Evidence:
  `local-research/evidence/outset-performance-route-v1-20260901/`.

## 2026-09-01 - Corrected contract reaches Outset; Omasao route is unreachable

- The cap-256 22,200-retrace tier stops normally after 3,773,282,695 host
  turns and 2,573.24 seconds wall / 2,336.90 seconds user CPU.
- Title-ready, file-select, opening, live play, ladder exit, DVD, ARAM, DSP,
  and copied-card outcomes remain coherent under the promoted cycle contract.
- After the ladder exit at retrace 20,674, the unchanged route's first-target
  distance grows from 367.722 to 4,484.323 units. It never reaches waypoint 1,
  event confirmation, or Omasao, so this run is not an Omasao throughput
  measurement and cannot qualify DSP or GX gameplay gains.
- Performance selection now requires the review F4 tier: two identical runs
  of a short bounded Outset-scene replay with a predeclared digest.
- Evidence:
  `local-research/evidence/mtmsr-cycle-contract-outset-v1-20260831/`.

## 2026-08-31 - Cycle contract passes the 700-retrace title rebaseline

- The authentic cap-256 route stops normally at 700 retraces with title-ready
  at 333, file-select at 535, 71/0 DVD forwards, 4,310/0 ARAM transfers,
  71,538 DSP DMAs, and a canonical copied card.
- The accepted post-`mtmsr` semantic baseline is 5,670,000,001 cycles and
  delivery hash `4736B7EA6A1D0B24`.
- Host turns are 62,038,491 versus 70,941,829 in the preceding exact-idle
  candidate/control tier, a 12.5% reduction. This remains title-route evidence;
  Outset qualification is required before a gameplay-scale claim.
- Evidence: `local-research/evidence/mtmsr-cycle-contract-700-v1-20260831/`.

## 2026-08-31 - MTMSR delivery boundary qualifies three-way short-route invariance

- Authentic three-retrace cap-256 turns: 7,685,219 -> 1,122,566 (`-85.4%`).
- Aggregate work is retained: 382 external deliveries, 367 ARAM transfers,
  and 33 DSP DMAs; the copied card remains canonical.
- The 774-cycle change from the old turn-quantized route is the required
  one-time semantic rebaseline. Fresh cap-256, cap-1,024, and dynamic runs are
  identical across 404 invariant records, with delivery hash
  `1ECDECBC0DEABA5A`; host turns are 1,122,566 / 1,089,965 / 1,090,565.
- `scripts/cycle_invariance.py` compares architectural records while excluding
  cap selection, wall time, and host-turn count. The short correctness tier is
  accepted; 700-retrace and Outset-scale qualification remain before a broad
  performance claim.
- Evidence: `local-research/evidence/mtmsr-delivery-boundary-v1-20260831/`.

## 2026-08-31 - Pending-disabled interrupt clamp owns 84.8% of short-route turns

- An observation-only return census exactly reproduces the accepted route and
  card. Budget exits are 6,697,265/7,685,446 dispatches (87.1%); eligible
  chunk-entry transitions are 577 (0.0075%).
- Ten one-cycle `__flush_cache` edges contribute 6,514,929 dispatches (84.8%).
  A pending external interrupt clamps the deadline to one even while guest
  interrupts are disabled.
- A deliverability-gated candidate reduces blocks from 7,685,219 to 1,048,569
  (86.4%), but fails exactness at ordinal 1, 768 cycles early. First DSP
  delivery is 772 cycles early, final PC and delivery hash differ, and AI
  remainder changes 105,346 -> 105,344. The candidate is removed; no cap-1,024
  run follows.
- Exact census/candidate log SHA-256:
  `3a0432f830a3053f56490a4edf2b9e1d0814f2c3e8f3947ee53f9a580f564c52`,
  `937329b238d2b331137a422d162bdb30cb1a90e159e592e47dc4fbff63c79247`.

## 2026-08-31 - Direct cross-chunk C calls fail the bounded falsification tier

- The isolated probe published section-local chunk ranges and emitted 56,211
  direct-call sites in the complete 748-chunk corpus. All 19 translator tests
  and ABI/address coverage passed.
- `-O1` compilation became pathological at the DOL boundary: eight units ran
  for more than 27 minutes each. Preventing inlining did not produce one
  representative object in 419 seconds. A diagnostic `-O0` composite linked
  in 343.15 seconds and is not a shipping-performance artifact.
- The only authorized cap-256 run reached three retraces and 24,300,000 guest
  cycles, but host turns fell only from 7,685,219 to 6,826,919 (11.2%). It
  also produced 15 rather than 382 external deliveries, zero rather than 33
  DSP DMAs, 17 rather than 367 ARAM transfers, and no disc mount. The card
  retained canonical SHA-256 `6b43aabd94f00ae3...`.
- The candidate misses both the 40% turn-reduction threshold and the exact
  digest, so cap 1,024 was not run. Source changes were removed. Log SHA-256:
  `057912443ca894a9409ce95412d92fdc280dbd5773454667dd32f9b111089e91`.

## 2026-08-31 - DSP exact-idle candidate is inactive on the reachable route

- The exact DSP idle/HALT candidate passed its focused adapter test and a
  serialized 700-retrace pair with byte-identical 1,041-entry delivery
  histories. HALT early-exit skipped 1,285,188 cycles; exact idle skip was zero.
- A 22,500-retrace visible run and a corrected 22,200-retrace headless run both
  stopped normally with canonical cards, but neither reached Omasao. The first
  used the stale 29-point path; the second restored the accepted 33-point
  detour. Both failed before waypoint 1 after the ladder, with distance growing
  from 367.722 to 4,484.323 units and retail recovery procedures taking over.
- Because neither run mounted Omasao or reached its third overlap-phase-6
  milestone, the zero idle count cannot adjudicate the review's Omasao sample
  inference. No speedup claim, control run, route variant, DSP signature
  variant, or HALT-only promotion is admissible. The source candidate is
  removed.
- Visible 29-point run: 9,264.81 seconds wall, 8,000.23 seconds user,
  3,925,598,308 blocks. Headless corrected-route run: 2,171.51 seconds wall,
  2,043.45 seconds user, 3,830,631,050 blocks. Log SHA-256:
  `f17c37d33cd6a1c6383cdf5444096e8d5d9d70d9d60292327f40bb537f24e8cc`,
  `7a074a07b01ce81066d1114992645863712ffe9dfd9857f29101ecec8b26bc5f`.

## 2026-08-31 - Timebase-write observation is route-insensitive

- TBL/TBU writes now flush an executed translated prefix before resetting the
  guest clock, and the focused regression passes, but the serialized cap pair
  exactly reproduces the prior route result.
- Caps 256/1,024 still end after 7,671,056/7,656,876 turns at 24,300,000
  cycles, with unchanged final-PC, AI, second-VI, delivery-hash, and card
  outcomes. No performance claim or long run follows.
- Run-log SHA-256:
  `86287891bffcc9ade598d9ac29223713c1172cf866ebc2e7d4187b53b6b90c15`,
  `074692cabaea73df560fc8d9c426d1b84168855c17a505acfed8d9dccac96eb0`.

## 2026-08-31 - Host interrupt-source rebudget is route-insensitive

- Atomic DI/DSP/SI source refresh and rebudget passes its focused regression
  and all 27 built host tests, but the serialized three-retrace cap pair
  exactly reproduces the prior result.
- Cap 256/1,024 end after 7,671,056/7,656,876 turns with identical 24,300,000
  cycles. Final PC, AI remainder, second-VI saved PC, and delivery hash still
  differ exactly as before; cards remain canonical.
- The correction remains in the host, but it provides no measured route or
  performance change and does not authorize a long run. Run-log SHA-256:
  `a36425d680d4c18ce21451a862f62d25b9db0b634c6bffefb356bf4b604785c8`,
  `65ec150b8ae235095a9584e3ee17d6e450a5361bafde8b7c9fd9d5310a93fe70`.

## 2026-08-31 - Poll rounding correction is route-insensitive

- Full deterministic regeneration differs from the previous 753-file
  composite in exactly the one hot poll translation unit. Incremental rebuild
  and relink take 99.12 seconds; dylib SHA-256 is
  `01856a88ded9c215ea9095c7ec88ff58b1ccb1f5a43d7e4868f665da66b931c0`.
- Complete ABI/address coverage passes. The serialized cap pair exactly
  repeats 7,671,056/7,656,876 turns and 24,300,000 cycles in both runs.
- Final PC, AI remainder, second-VI saved PC, and delivery hash still differ
  exactly as before. The corrected rounding edge is not exercised as the
  owning split on this route.
- The stable-poll shape is closed and no 700-retrace performance result is
  admissible. Run-log SHA-256:
  `5fb37539cfffe8757534991b05d24ad46159475709404b7e2c678f5d1f2ae65c`,
  `b955e86696e015e90225b0e33fa31671ef56442172ac5879daace23dc3bff854`.

## 2026-08-31 - Stable-poll repair removes clock drift but not route drift

- The fresh 748-unit dylib builds in 6,512.29 seconds, passes complete
  ABI/address coverage, and has SHA-256
  `16bc7ff28ae351ca2a226bb7467f5a1de5d4c8c65e37c18bf0e2c2a60ae86261`.
- The serialized cap-256/1,024 pair now ends at the same 24,300,000 cycles and
  2,025,000 timebase ticks. This removes the prior two-cycle final-clock drift
  and proves the corrected hot poll executes on-route.
- Route invariance still fails: turns are 7,671,056/7,656,876, final PC is
  `0x80307EF4`/`0x80307EF8`, AI remainder is 28,257/26,721, and delivery hashes
  are `0765DDD0C6CE1481`/`7BFDF601109E49E8`.
- The first 16 DSP tuples and external/DSP counts remain exact. The first
  recorded architectural difference is second-VI saved `srr0`,
  `0x80305E80`/`0x802B1924`. Cards remain byte-identical.
- The candidate is rejected for blocker closure. No 700-retrace performance
  result is admissible. Run-log SHA-256:
  `a18026a7990ea6f248629b0b634ce717784d9afd520920c8f342aa5ca1db69f9`,
  `7da5487d5c3f7cc5ec8456dde0c16bf2dc6cf06597844c41d66e06ed4e67927b`.

## 2026-08-31 - Stable-poll acceleration is capped by authentic deadlines

- Static cap-independence fixtures clear direct backedges, local-return
  dispatch, and mid-block entry, but a focused stable-poll fixture reproduces
  a five-cycle deadline crossing to 12 cycles.
- The cause is a host-budget ceiling fast-forward after only one loop
  iteration had been proven deadline-safe. The production corpus has exactly
  one affected loop: hot `SelectThread` at `0x80307EF4`.
- Deadline-limited acceleration now consumes only complete loop iterations;
  precise redispatch consumes the remaining instruction cycles and stops
  exactly at five. Host-limited acceleration retains its prior ceiling
  behavior.
- DolRecomp passes 19/19 and composite generation passes 18/18. Patch SHA-256:
  `88020a3757edd30bf1ba3c7d0cdab48466c012148256463499dcdd7cbe2bc7a0`.
- This is source qualification, not a route or performance result. No speedup
  claim is admissible until a fresh composite passes the serialized short cap
  pair exactly.

## 2026-08-31 - Generalized semantic exits are route-insensitive

- The fresh 748-unit balanced-exit composite linked in 9,772.25 seconds and
  passed coverage for 417 ranges, 415 RELs, and 8,079 sections. Dylib SHA-256:
  `a9f2f7537837d10f25ad0cb289c281a77c0042d43bdb5e2db2358566e85d34eb`.
- Its serialized cap-256/1,024 three-retrace pair exactly reproduces the
  FP-only candidate: 7,671,048/7,656,872 turns, 24,300,000/24,300,002 cycles,
  AI remainder 28,257/26,723, and divergent final delivery hashes despite 16
  identical initial DSP delivery tuples.
- The additional balanced semantic exits are therefore not active performance
  or correctness owners on this route. No 700-retrace measurement is
  admissible, and the exit-refund experiment shape is closed.
- Run-log SHA-256:
  `f65435e504cef7c94459c41ef36f1633b25f4ca3c072add92eb0cd6ef29c7eb7`,
  `634a8c85e435bc7736d2e598ca6fba8780983877b3bf7aae1827f60c8ad499db`.

## 2026-08-31 - Lazy-FPU refund is correct but does not qualify the route

- The fully regenerated ABI-v6 composite links 748 units and passes coverage
  for 417 ranges, 748 chunks, 415 REL modules, and 8,079 sections. Its dylib
  SHA-256 is
  `948a9c3daab353be6fac219225f22c0970c0c0f339a71c4a23216da5eff8ad0f`.
- The serialized three-retrace cap-256/1,024 pair stops at the same final PC
  with identical external/DSP counts and first 16 DSP delivery tuples, but
  final cycles are 24,300,000/24,300,002, AI remainder is 28,257/26,723, and
  final delivery hashes differ. Host turns are 7,671,048/7,656,872.
- Both card copies retain canonical SHA-256. The focused exceptional-exit
  refund is retained as correct translator behavior, but the route result
  rejects it as blocker closure. No long-run performance claim is admissible.
- The successor is a static whole-block exit/resume conservation audit. This
  avoids iterating the same early-exit repair shape without evidence of a new
  owning violation.
- Run-log SHA-256:
  `3ca6f18590f3da45409c149bbcd52e41565fe330d22a9b4998905a99845e4f96`,
  `17ea767a75dbe71233ea18f7b5c55d4c8a62d7590257886e3f7557072182fb5d`.

## 2026-08-31 - Post-write deadline clamp narrows but does not close cap drift

- `host_mmio_write` synchronized and rebudgeted before applying a register
  mutation, then returned without publishing a deadline created by the write.
  The promoted runtime now rebudgets after every handled MMIO write.
- All 26 built host/runtime tests pass. The separate three-retrace cap-256 and
  cap-1,024 runs both stop at `0x80307EF4` after 24,300,001 guest cycles.
- Invariance still fails: AI remainder is 28,225 versus 26,689 and delivery
  hash is `14D31C137A95B1EB` versus `991498E309172245`. The 1,536-unit gap is
  exactly twice the 768-cycle cap difference. Host turns are
  7,671,051/7,656,894; cards retain the canonical SHA-256.
- This is accepted as a missing cycle-domain clamp, not as performance or
  blocker closure. Static audit identifies unrefunded prepaid suffixes on
  generated exceptional exits as the next concrete mechanism. No long run is
  admissible until a focused translator fixture proves that handoff.
- Run-log SHA-256:
  `25804a0458875ba8221b34e9909b34dff69510101e903dd12ea7d681cf55cc17`,
  `7d7cd05db3a01a27b8c406785d65ed0d1e191bcde7d839a8265c132c81dda633`.

## 2026-08-31 - Instruction charging closes clock drift but not delivery drift

- A fresh 748-unit ABI-v5 composite linked in 6,865.75 seconds wall. Its
  441 MiB dylib SHA-256 is
  `f4c6af1a45e692bc767a42c3b3017d32320b12e2bbb90226ac918b3b40002c21`.
- Separate three-retrace cap-256 and cap-1,024 runs both end at exactly
  24,300,000 guest cycles and 2,025,000 timebase ticks. This removes the prior
  one-cycle final-clock drift.
- The candidate is nevertheless rejected: AI remainder is 7,152 versus 5,614,
  final PC is `0x80307EF8` versus `0x80307EF4`, and first-VI saved PC differs
  even though the deadline cycle matches. Host turns are 7,685,436 versus
  7,671,536, so this form also provides no useful short-route turn reduction.
- The mechanism loses exact deadline distance whenever it is farther away than
  the host cap. A prepaid block can therefore cross from `distance > cap` to
  past the deadline before precision becomes active. Performance measurement
  stops until that contract is represented independently and cap invariance
  passes; no 700-retrace result is admissible.

## 2026-08-31 - Cycle-domain cap invariance fails at generated check sites

- Authentic cap-256 re-baseline: normal 700-retrace stop, 62,305,847 host
  turns, 5,670,000,000 guest cycles, AI remainder 14,381, canonical card.
- Single cap-1,024 comparison: normal stop, 48,066,103 turns (22.9% fewer),
  but 5,670,000,001 cycles and AI remainder 12,846. FPU/collision, milestones,
  scene, DSP, DVD, ARAM, GX, and card outcomes otherwise match.
- Removing the host loop's synthetic one-cycle advancement for zero-cycle HLE
  turns is architecturally correct but did not close a three-retrace
  falsification: cap 256/1,024 ended at 24,300,001/24,300,002 cycles and AI
  remainder 70,594/69,059. The earliest logged behavioral difference is DSP
  interrupt position before retrace 2.
- Decision: the current basic-block charge can cross an exact device deadline
  before the next generated check site. Stop cap/guard iteration. Add an
  instruction-cycle deadline path, prove it in translator fixtures, and only
  then rerun the three-retrace falsification. Cross-chunk work remains gated.
- Full-run log SHA-256: `0ce866eea2a470e7e756475566c03e6b313b13d028d39afa86d2e48d44f96d7c`
  and `5fa3f8feb9e5e4abb01d973e18ef9e2032f6b6ca0b10a2711ab0089a8001f74b`.
  Three-retrace log SHA-256:
  `4efb5ddb404ab1cb9c8bc633dd7081583442ce6b4b6cd9ae26388d7b1995fb91`
  and `b96f3c2859d5ad2195bae24eb70afa02f1cee7c9c037c306dde38a87bd35486e`.

## 2026-08-30 - Cycle-domain implementation reaches route gate

- The default-on runtime now advances TB/DEC from one absolute cycle count and
  synchronizes VI, DSP, and audio with per-device cursors at MMIO and host-turn
  boundaries. No cycle can be consumed twice by a device.
- Generated C records the unexecuted basic-block suffix before TB/SPR and
  potentially external memory instructions. Observation flushes consume only
  the executed prefix and retain the suffix charge, making the observed clock
  a function of the reading instruction rather than the host return point.
- Every basic-block boundary checks the current distance-valued budget before
  the next block executes. `mtdec` and nested callback dispatches rebudget
  immediately. Cap selection is a diagnostic/performance dial over an always-on
  contract, defaulting to 256.
- Verification: 28/28 DolRecomp CTests and 212/212 registered host/runtime
  tests pass. The regenerated canonical tagged composite passes 18/18
  generator tests and ABI/address coverage for 417 ranges, 748 chunks, 415 REL
  modules, and 8,079 REL sections.
- The full arm64 `-O1` composite linked successfully in 4,066.19 seconds wall
  (20,246.87 user, 515.38 system). Its SHA-256 is
  `bf07dc2f4e5405c713f363b386b99e1f97607c5b25eedb67646e281763f3e771`.
  No fps or route claim is made before the authentic cap-256 re-baseline.
- Next measurement is exact cap-256 versus cap-1,024 route invariance.
  Cross-chunk performance work remains gated on that correctness result.

## 2026-08-30 - Budget drift exposes a cycle-domain contract blocker

- The full 748-unit composite now consumes the ABI-reserved
  `CPUState.cycle_budget`; zero retains the accepted 256-cycle default. The
  rebuilt artifact passes all 417 code ranges and 415 REL ranges, and the
  default host reproduces the exact accepted 61,469,941-turn digest.
- First policy: use 1,024 cycles only outside the nearest DSP, VI, AI,
  decrementer, or pending-interrupt deadline, fall back to 256 near a deadline,
  and tighten to 256 after MMIO. Host turns fell to 47,495,658, but lazy-FPU
  restores moved from 1,873 to 1,875 and final guest cycles moved from
  5,670,000,072 to 5,670,000,104. It is rejected.
- The generated corpus contains one charge of 1,718 cycles, proving that a
  1,024-cycle deadline guard could itself overshoot a deadline. One bounded
  correction added a 2,048-cycle overshoot margin. It restored the exact FPU
  and collision summaries and reduced turns to 50,032,142, but still ended at
  5,670,000,086 cycles and 472,500,007 timebase ticks, versus
  5,670,000,072/472,500,006. It is also rejected.
- The reverse restored control is exact and card-identical. Candidate user CPU
  is 18.76 seconds versus 18.78 for that control; retired instructions improve
  4.6%, but no meaningful matched CPU gain justifies weakening the clock gate.
- Reorientation: `INDEPENDENT_REVIEW_2026-08-30.md` correctly identifies the
  exact baseline as turn-boundary quantized. TB/DEC/time-MMIO observations and
  interrupt delivery move whenever host-turn positions move, so the small drift
  is expected and cannot adjudicate budget correctness. A pass would establish
  only route insensitivity, not equivalence.
- Decision: close guard-window and fixed/dynamic cadence iteration. Keep only
  behavior-neutral generated ABI support with the 256-cycle default. Promote a
  cycle-domain observation/delivery contract, re-baseline once at cap 256, and
  require identical correctness digests at caps 256, 1,024, and dynamic. The
  block/turn counter is explicitly performance data, not an identity invariant.
- Independently verified follow-on evidence: `emit_set_chunk_table` has no
  translator caller and zero generated chunk files contain
  `dolrecomp_call_enter`, so cross-chunk chaining is gated behind the clock
  contract. The Omasao opcode mix validates an exact DSP idle fast-forward and
  halt early-exit as a separate host-only candidate.
- Composite, first policy, guarded policy, restored control, and card SHA-256:
  `8e42ef1b9a292dea3300d3311251a37d04f23d073fc391e1bef4b43e23eee5a2`,
  `eb659d5863351846b9d0b7796e5ad0b98d85e0414087e35326ce64004394b87e`,
  `e794ce6fd2c65071944a531f9ba27221121598d93a94808a27a4e0ef14b65ee6`,
  `fa1629bc16c2a99b7cbbeee71a0df9ee40749ab7b243b06ac0174512e678a53e`,
  `6b43aabd94f00ae3de01b42567b5d11026cf6a9374e4d023565b31bf3c7e1987`.

## 2026-08-30 - Current shipping profile reranks scene CPU ownership

- Route: trace-off signed app with no frame-pacing trace, exact corrected input,
  two ten-second macOS `sample` captures attached to BlueWake itself, copied
  card, and the 22,500-retrace bound.
- Correctness: the profiled run repeats all 3,670,902,066 blocks, full scene
  milestones, CPU/device summaries, and the canonical card SHA. Sampling
  overhead makes its wall time ineligible for performance comparison but does
  not change ownership or correctness admissibility.
- Outset: 8,075 main-thread samples. Selected translated dispatch owns 4,131
  (51.2%). The nested retail GX FIFO/frontend/sink route owns 965 (12.0% of
  main-thread time); exact-eight DSP service owns 962 (11.9%). The remaining
  time is host-turn/device bookkeeping and guest execution outside the named
  GX subtree.
- Omasao: 8,126 main-thread samples. Translated dispatch owns 4,015 (49.4%),
  including 986 GX samples (12.1%); DSP service owns 2,482 (30.5%). The scene
  difference explains why a DSP optimization helped Omasao more than Outset.
- GPU and render workers are predominantly waiting. Texture hashing is no
  longer a sampled owner after the MEM1 generation contract.
- Decision: prioritize worst-scene Outset. The fixed 1,024-cycle probe already
  proved host-turn upside but failed exact delivery. Implement one dynamic
  execution budget capped by the nearest authentic device/interrupt deadline,
  first qualifying the 700-retrace digest. Do not retry a fixed constant,
  another DSP helper, or GX allocation reuse.
- Run, Outset sample, Omasao sample, and card SHA-256:
  `0afcb25a586ef2f7b1c6c087920c3fdec4e4a01a5f676fda08dc54bdcf02175f`,
  `1c5a007a9aeae13d2150828acba7f26b4da7db11fc56e6f896f8360888d8d681`,
  `cdba4fa758ee0d4b0365aa88c5c267a5fc515d95bd2c242c50410dbf81df7699`,
  `6b43aabd94f00ae3de01b42567b5d11026cf6a9374e4d023565b31bf3c7e1987`.

## 2026-08-30 - Shipping configuration improves the exact visible route

- Route: signed trace-off Aurora app, frame-pacing telemetry, donor DSP,
  corrected 33-point collision route, copied accepted card, and one BlueWake
  process through the 22,500-retrace Omasao bound.
- Correctness: normal stop after the exact 3,670,902,066 blocks. Title,
  opening/play, route, event 75, and Omasao phase-6 milestones match. Collision
  and FPU summaries are coherent; DSP is 1,965,597 DMAs, ARAM 5,443/0, GX
  32,476,218/0, and the card remains byte-identical.
- Whole-route result versus the prior exact-eight build: wall time falls from
  1,210.27 to 939.30 seconds (22.4%), user CPU from 1,222.26 to 950.24 seconds
  (22.3%), retired instructions from 18,073,040,996,189 to 13,910,599,626,546
  (23.0%), and host cycles from 3,967,193,897,426 to 2,993,036,229,743
  (24.6%). This is a cumulative comparison after MEM1 generation caching and
  trace compilation policy; the matched short A/B below isolates trace cost.
- Visible Outset: 61 sixty-frame interval samples report 5.91 fps mean,
  6.0 median, 2.5 minimum, and 7.9 maximum, with 6,914 median draws. The 10th
  percentile is 5.3 fps, but two late samples fall to 2.5-2.6 fps while FIFO
  work roughly doubles.
- Visible Omasao: six samples report 13.47 fps mean, 13.55 median, and a
  12.1-15.0 range with 4,466 median draws. The light opening remains near 29
  fps, proving strong scene/state dependence in the same binary.
- Conclusion: recent work is visibly material but does not meet the product
  frame-time gate. A fresh native sample on this exact shipping build must
  rerank current CPU ownership before the next optimization.
- Run/card SHA-256:
  `8e6514a8b08be6b02f005012ba249b7725fe26e561519864b81a97c7f1938d58`,
  `6b43aabd94f00ae3de01b42567b5d11026cf6a9374e4d023565b31bf3c7e1987`.

## 2026-08-30 - Shipping build compiles dormant developer traces out

- Hypothesis: although trace environment values were cached at startup, 28
  trace/watch conditions remained interleaved through the exact 256-cycle host
  loop. A normal build can make them compile-time false while a separate
  trace-enabled build retains diagnostic behavior.
- Matched signed 700-retrace reverse control: both binaries stop normally at
  `0x80307ef4` after 61,469,941 blocks. Cycles/timebase are
  5,670,000,072/472,500,006; title readiness is retrace 333; title and file
  selection milestones match.
- CPU result: trace-off user CPU is 19.30 seconds versus 23.94 (19.4% lower),
  retired instructions are 276,709,110,670 versus 340,347,197,025 (18.7%
  lower), and host cycles are 65,078,742,397 versus 79,926,932,162 (18.6%
  lower). Wall time is 20.91 versus 27.31 seconds, but CPU counters are the
  primary comparison.
- Correctness: lazy-FPU/collision summaries, 71,287 DSP DMAs, 71 DVD forwards,
  4,310 ARAM transfers, 153,170 GX submissions, texture-cache totals, and the
  copied card all match. Card SHA-256 remains
  `6b43aabd94f00ae3de01b42567b5d11026cf6a9374e4d023565b31bf3c7e1987`.
- Scope: this accepts shipping diagnostic policy, not the product performance
  gate. Representative Outset/Omasao frame pacing must be remeasured before
  choosing the next architecture change.
- Candidate/control log SHA-256:
  `63babf992ce5bfce8d8d67cbc6d22484dfa3f8d480357e42b52a1a62729910de`,
  `6ca47897a17a835250e0f5e52e15f7344cc4762c7b43b353285303cfbe86c379`.

## 2026-08-30 - Fixed larger dispatch quantum rejected; deadline budget next

- A fresh signed title-route profile after dirty-generation promotion contains
  no texture-hash hotspot. CARD dispatch and FP availability reappear as small
  symbols, but both exact call-boundary shapes were already rejected.
- A deliberately non-promotable DOL-only probe compiled main-executable chunks
  with a 1,024-cycle loop quantum and reused 256-cycle REL objects. This tier
  can test title dispatch cost but cannot qualify gameplay.
- Matched 700-retrace control/probe: host turns fall 61,469,941 to 47,000,487,
  user CPU 24.23 to 22.16 seconds (8.5%), retired instructions 340.75B to
  306.43B (10.1%), and host cycles 81.13B to 75.35B (7.1%).
- Title and file-select retraces match. DSP is 71,287 DMAs, ARAM 4,310/0, GX
  153,170/0, DVD lifecycle and texture-cache totals match, and both copied
  cards retain SHA-256 `6b43aabd94f00ae3de01b42567b5d11026cf6a9374e4d023565b31bf3c7e1987`.
- Exact delivery does not match: restored lazy-FPU switches are 1,871 versus
  1,873, collision provenance records four deferred-FP returns versus zero,
  and the final cycle count is 5,670,000,402 versus 5,670,000,072. The fixed
  1,024 candidate is rejected.
- Next hypothesis: use the already ABI-reserved `CPUState.cycle_budget` as a
  dynamic per-dispatch limit, bounded by the next authentic device/interrupt
  deadline. Do not parameter-sweep another fixed quantum.
- Probe/control SHA-256:
  `15aa27be94867cb4907dadb81218bb49c6b05f3377aff7e7a81a72bd3a52c817`,
  `a72b5ba1ab87f7ee54a3fc8eb135bfd550e2319661d8879ca8b80ad7aff8fbfb`.

## 2026-08-30 - Authentic dirty generations eliminate repeated texture hashes

- A default-on 4 KiB MEM1 generation table observes retail `dcbst`/`dcbf`
  publication and explicit DVD, ARAM, and guest bulk writes. GX skips a texel
  or TLUT hash only when the complete declared span retains its generation;
  unresolved provenance preserves the original full hash.
- Exact signed 700-retrace results: 280,039 GX submissions, 3,628 uploads,
  373,650 hits, and the canonical card are unchanged. Texture hashes fall from
  376,872 to 3,628; palette hashes fall from 133,994 to 383; 373,244 lookups
  reuse a generation-proven hash and zero use an unproven shortcut.
- Candidate user CPU is 28.22 and 27.66 seconds (27.94 average). Three nearby
  accepted controls are 28.33, 28.06, and 28.65 seconds (28.35 average), for
  about a 1.4% improvement. Retired instructions fall from 406.52 billion in
  the reverse control to 403.89-404.03 billion in the candidates.
- The result promotes the coherence subsystem but rejects the premise that
  texture hashing accounts for most end-to-end frame cost. A fresh profile,
  not another cache variant, owns the next loop.
- Candidate logs SHA-256:
  `4f19b4ce22317dff8c207a06488adf9df079d07f8b77690201c5618d4272f5b3`,
  `1522d6af99286501323e6be503148a430027127cc8d6cff34a08638acbd0325b`.

## 2026-08-30 - GX allocation probes rejected; texel hashing owns next loop

- Two signed 700-retrace allocation probes preserved exact guest/device/card
  results but did not improve user CPU beyond run noise. Streaming payload
  reuse averaged 28.61 seconds candidate versus 28.20 control; reusable
  DrawPlan vertex/index storage took 28.34 seconds. Both were removed.
- A signed Aurora route then reached retrace 7,633 / 974,033,447 blocks and
  stopped normally at its block bound. Simple opening shots presented near
  24-26 fps; state-heavy title frames remained roughly 11-12 fps, confirming
  strong scene dependence. The copied card was byte-identical.
- Its valid 10-second sample ranks full texel hashing inside
  `submit_draw_plan` above GX allocation and plan construction. The exact 700
  control performs 376,872 texture hashes and 133,994 bounded palette hashes
  while uploading only 3,628 textures.
- A source-version candidate was rejected authentically: the prehashed
  `data_version` exists only on the HLE texture-load route, and the retail FIFO
  control reported zero versioned lookups. No candidate code remains.
- Next hypothesis: a promoted retail memory dirty-generation/invalidation
  subsystem can reuse a previous content hash only when every owning write
  path proves the texture and TLUT spans unchanged.
- Opening sample, route, and card SHA-256:
  `44fa63de9861984488c386705ad587eb96eead4a7676c7d0c6d8be12208b1535`,
  `d7a77d23377a2c4c283f3887bc13da184883e828c03d6731b02ae7c1dd1276cc`,
  `6b43aabd94f00ae3de01b42567b5d11026cf6a9374e4d023565b31bf3c7e1987`.

## 2026-08-30 - Exact-eight DSP adapter primitive

- Full-scene qualification: the candidate repeats the exact corrected
  22,500-retrace route through Omasao phase 6 and stops after the same
  3,670,902,066 blocks. Wall time falls from 1,245 to 1,210.27 seconds (2.8%),
  retired instructions from 18,378,113,533,241 to 18,073,040,996,189 (1.7%),
  and host cycles from 4,018,634,395,156 to 3,967,193,897,426 (1.3%). All CPU,
  DSP, DVD, ARAM, GX, route, final-PC, and card outcomes match.
- Measurement limit: `DOL_FRAME_PACING_LOG` was omitted from the candidate
  launch, so this run cannot revise the published Outset/Omasao FPS range. It
  does qualify whole-route cost and correctness. The next measured owner is
  GX packet/draw construction; repeating this 20-minute route only to emit the
  missing secondary counter would not change that decision.
- Hypothesis: the accepted batch-eight route still crossed generic
  `DSPCore::RunCycles` and `Interpreter::RunCycles` dispatch for every eight
  instructions. The linked ARM64 binary proved ThinLTO had not folded those
  layers away.
- Change: the adapter directly executes exactly eight donor `Step()` calls,
  retaining the per-instruction halt check and the existing eight-cycle
  external-interrupt boundary. Partial tails retain the generic donor path.
  No DSP opcode helper or cadence is changed.
- Warm matched A/B: the signed 700-retrace control takes 25.21 seconds wall /
  24.94 seconds user; the candidate takes 23.90 / 23.55, improving 5.2% wall
  and 5.6% user CPU. Retired instructions fall from 349,555,257,734 to
  341,138,660,110 (2.4%).
- Correctness: both runs stop normally after 61,469,941 blocks at the same PC,
  with identical clock/title milestones, FPU and collision summaries, 71,287
  DSP DMAs, 71 DVD forwards, 4,310 ARAM transfers, 153,170 accepted GX
  submissions, and byte-identical copied cards.
- Verification: 211/211 native CTests, 23/23 focused ABI contracts, strict
  signing, and ARM64 disassembly pass. Scene-level Omasao frame telemetry is
  still required before changing the published FPS estimate.
- Control, candidate, app, and card SHA-256:
  `e5aad9d763f677439c2ea99b417f2a2d853c9888f7fc7a67cca900ea68e17b8d`,
  `95ab1ccb3c764ed33fda4456162e6d9f0f6019a785d0a2bc397d3ebed55c488e`,
  `120d257aaa2780a6449f526e78d99e79ccd195aca55f1b6744229c25f9609812`,
  `6b43aabd94f00ae3de01b42567b5d11026cf6a9374e4d023565b31bf3c7e1987`.
- Full-route candidate SHA-256:
  `0acb7e7d15b1baa1998345ad05cbccb687b4e6d4754984d717095c22d108b27d`.

## 2026-08-30 - Visible Outset and Omasao profile ranks CPU owners

- Route: strictly signed Aurora app, donor DSP, corrected 33-point route,
  copied accepted card, low-rate frame-pacing telemetry, and native `sample`
  captures in Outset and after authentic Omasao overlap.
- Visible result: Outset presents at about 3-5 fps with 5,800-7,023 draws per
  sampled frame. Omasao improves to about 10.6-11.9 fps with roughly 4,450
  draws, still far below the product target.
- Ownership: Omasao assigns about 37% of active main-thread samples to donor
  DSP interpretation, about 16% to GX FIFO/frontend/draw construction, and the
  balance chiefly to translated dispatch/runtime work. Outset independently
  assigns about 27% of its guest-dispatch branch to GX. GPU/render workers are
  mostly waiting, and the process remains approximately one-core bound.
- Correctness: normal stop at retrace 22,500 / 3,670,902,066 blocks; coherent
  collision/FPU state; ARAM 5,443/0; DSP 1,965,597; GX 32,476,218/0; unchanged
  card. Wall time was 1,245 seconds.
- Measurement correction: an earlier run's stack capture targeted
  `/usr/bin/time`, not BlueWake, and is rejected. Its frame-pacing and
  correctness telemetry agree but do not support stack ownership.
- Hypothesis: optimize the existing exact eight-cycle DSP route first without
  changing delivery cadence, then measure GX batching/allocation separately.
- Run, Outset sample, Omasao sample, and rejected wrapper sample SHA-256:
  `a500b666d1372548690ba347ab08439ff201fb4ba14cefa15e571498314129a1`,
  `e9cb7bfc9683bd2e3c6de250ae1122aa26956438dcc1f8014db38a75fd0a492c`,
  `d703940ba8947a207f85ddc8175ac3780a69051be5bd92015e74df5e26ce7b38`,
  `cbba40b88716845141faaf907cf2c4e64548a2617e3419d7159211957a02337e`.

## 2026-08-30 - 64-bit GX sequence passes the former release-scale boundary

- Route: strictly signed Aurora app, donor DSP, corrected 33-point Omasao
  route, copied accepted card, 132,000 retraces, and minute thermal sampling.
- Result: normal guest stop after 13,600,032,229 blocks and 6,776.75 seconds
  wall. GX completed 277,680,555 submissions with zero rejection/failure and
  no non-monotonic sequence, crossing the former failure near retrace 131,072.
- Correctness: 1,042,557 coherent `GroundCross` returns; FPU 0/0/3/413,775;
  DVD 191/0; ARAM 8,597/0; DSP 10,822,460; byte-identical card.
- Resource state: 748,027,904-byte maximum RSS, 699,926,040-byte peak
  footprint, and 113 minute samples with no thermal or performance warning.
- Scope: this resolves GX endurance blocker `BW-P4-0061`. Its roughly 14-20
  retraces-per-second aggregate rate confirms a product performance problem
  but does not assign ownership; `BW-P4-0062` begins with a visible-scene
  profile rather than an inferred optimization.
- Run, thermal, app, and card SHA-256:
  `887fb6509ec1f429eaf5d4e66bb92837079575c3b4292adf5598227159e1dabe`,
  `b0b49adac7d0bd2655f1746bbda9f019aad1dd7ad0bba1b3e5f518b4b1794ecc`,
  `15c275187af4ccbb488f8ebc1207a15b06886931c6e0c5ceafba7077f6f46f57`,
  `6b43aabd94f00ae3de01b42567b5d11026cf6a9374e4d023565b31bf3c7e1987`.

## 2026-08-30 - Release-scale tier reaches bound but GX sequence wraps

- Route: strictly signed Aurora app, donor DSP, corrected 33-point Omasao
  route, copied accepted card, 216,000 retraces, and minute thermal sampling.
- Result: normal guest stop after 21,224,622,447 blocks and 10,956.67 seconds
  wall. CPU/collision/device behavior remained coherent, but the result is not
  a graphics pass: one `non-monotonic packet sequence` failure occurred near
  retrace 131,072 and GX shutdown reported 291,015,921 submissions / one
  failure.
- Ownership: the 32-bit drain packet counter wrapped after billions of
  state/resource packets. The game continued after gxcore presentation stopped,
  so later guest stability cannot certify later visible frames.
- Healthy evidence: 1,672,482 coherent `GroundCross` returns; FPU
  0/0/3/658,256; DVD 191/0; ARAM 11,048/0; DSP 17,613,506; byte-identical card;
  183 thermal samples with no warning. Maximum RSS was 701,300,736 bytes and
  peak footprint was 712,181,296 bytes.
- Failed run, thermal, app, and card SHA-256:
  `c6fc749fb60b8427b3bfb44da6b26dcd8fe62cf650fc02e28b0adbc801c42828`,
  `58c6ef10cfb1c1e97e1407987d41e9f5e8bab60209ae0dad91cc850eeb88c0dc`,
  `b418780fb8f4146e974c5a7ffab369fbf7033eef8b42eb77d3307bf5893981aa`,
  `6b43aabd94f00ae3de01b42567b5d11026cf6a9374e4d023565b31bf3c7e1987`.

## 2026-08-30 - Retail-FPU Omasao correctness tier passes

- Route: strictly signed Aurora app, donor DSP, copied accepted card, and the
  corrected 33-point collision-derived Outset route; one BlueWake process, no
  Simulator, bounded at retrace 36,500.
- Result: Omasao overlap phase 6 at retrace 21,895 and normal stop at retrace
  36,500 / 4,936,416,785 blocks. The run crosses all captured `BW-P4-0060`
  panic and invalid-consume block points without an incoherent tuple.
- Correctness: 326,382 coherent `GroundCross` returns, FPU outcomes
  0/0/3/136,198, canonical SDA2/lava state, DVD 191/0, ARAM 5,837/0, DSP
  3,098,333, GX 63,748,236/0, and a byte-identical copied card.
- Runtime: `1,851.84s` wall, `1,901.54s` user, `33.46s` system;
  705,216,512-byte maximum RSS. This run did not include separate thermal
  sampling and is a correctness closure tier, not the 216,000-retrace release
  workload.
- Run, app, and card SHA-256:
  `d6296f5185a1117da6ad4609c4f22959156d3acce34b41451ae07bd37d2429ac`,
  `b418780fb8f4146e974c5a7ffab369fbf7033eef8b42eb77d3307bf5893981aa`,
  `6b43aabd94f00ae3de01b42567b5d11026cf6a9374e4d023565b31bf3c7e1987`.

## 2026-08-30 - Exact-context door-event bounded re-observation

- Route: strictly signed Aurora app, donor DSP, copied accepted card, exact
  high-clearance Outset route, bounded at retrace 36,500; one BlueWake process
  and no Simulator.
- Result: normal stop after 7,450,379,074 blocks. The route completes 29/29
  waypoints and starts door event 75 at retrace 21,669, but never requests or
  mounts Omasao. The former 4.897B-block `f1`/collision panic is absent.
- Correctness: 400,766 coherent `GroundCross` returns; 11,339 canonical
  r2/source checks; DVD 180/0; ARAM 5,332/0; DSP 3,834,583; GX
  75,030,514/0; copied card byte-identical.
- Runtime: `2,784.89s` wall, `2,790.92s` user, `48.50s` system;
  735,739,904-byte maximum RSS. All 47 minute thermal samples report no
  thermal or performance warning level.
- Scope: authentic re-observation supersedes the old downstream symptom and
  assigns an earlier event-completion blocker. It is not an Omasao or release
  stability pass and is not a performance comparison because scene residence
  changed.
- Run, thermal, app, and card SHA-256:
  `ccbd36a3ae82ba73a107c938e5842b4462f0b273d9f6ebf6a47d7518d975805b`,
  `bccebde7a1f333081e258fd8c468d48a90b0c4606aef50641f4f2b7760067bbc`,
  `cb2da435c57e8c8c3fba51fcd68d6e9dcbf20b7eea0ec4b29472887eb6a2108f`,
  `6b43aabd94f00ae3de01b42567b5d11026cf6a9374e4d023565b31bf3c7e1987`.

## 2026-08-30 - Exact FPU context and cold-entry focused acceptance

- Route: strictly signed Aurora app, donor DSP enabled, copied accepted card,
  bounded at retrace 14,100; no Simulator and one BlueWake process.
- Result: opening complete at retrace 13,852, play scene at 13,912, and normal
  stop at retrace 14,100 / 897,045,485 blocks after crossing the former
  retrace-13,961 `f1` mismatch. `GroundCross` returned coherently 9,068 times.
- Subsystems: 140 lava checks with canonical r2 and source word `0xCE6E6B28`;
  DVD 179/0, ARAM 4,638/0, DSP 825,518 DMAs, GX 1,353,814/0; copied card
  byte-identical.
- Runtime: `379.43s` wall and 742,473,728-byte maximum RSS. This is focused
  correctness evidence, not a sustained-performance or release-stability
  result.
- Verification: 211/211 runtime tests, 28/28 freshly rebuilt translator tests,
  strict app signing. The first translator invocation used a stale fixture
  object older than its source oracle; rebuilding the target made all 28 tests
  pass, so no regression claim attaches to that stale failure.
- App, run, and card SHA-256:
  `cb2da435c57e8c8c3fba51fcd68d6e9dcbf20b7eea0ec4b29472887eb6a2108f`,
  `777440e6b85d40383484ffa60a2797ffc8767ca9832a1e4f33d04d8b67fb96cc`,
  `6b43aabd94f00ae3de01b42567b5d11026cf6a9374e4d023565b31bf3c7e1987`.

## 2026-08-30 - Persistent SDA2 hypothesis rejected at exact failure

- Route: exact signed 36,500-retrace Omasao replay with a copied accepted
  card and optimized default-on SDA2 invariant.
- Result: invalid consume reproduced at retrace 35,734 / 4,896,772,902
  blocks, but live r2 remained `0x803FFD00` after 4,899,708,497 checks and at
  the terminal guard. Persistent SDA2/context drift is rejected.
- Subsystems: DVD 199/0, ARAM 5,897/0, DSP 3,031,470, GX 62,750,453/0; card
  byte-identical and thermal samples clear.
- Runtime: `1,831.11s` wall and 731,676,672-byte maximum RSS.
- Replacement focused control: 700 retraces, 57 immediate pre-load source
  checks, canonical `r2=0x803FFD00`, source word `0xCE6E6B28`, and normal stop
  at 61,421,183 blocks.
- Long run, thermal, and focused control SHA-256:
  `e3df9202054baaeb430493ab1a318d28110fce7a1317da487e8ced0da0dba80d`,
  `badcc474657def7d6e382ff03e57bc98e24117e4584a11bf4baa87ecc5a051c4`,
  `7c655556e16c1af9e313c247271d5c2641252f8f07f93ad71b58aae6afe8d2f7`.

## 2026-08-30 - SDA2 invariant passes signed 700-retrace control

- Static signature: intended `-1e9` is at SDA2 address `0x803FA0F4`; the
  captured `16384.0` is exactly `0x30` later, algebraically identifying
  `r2=0x803FFD30` instead of retail's `0x803FFD00`.
- Control: normal stop at 61,421,183 blocks after 61,483,512 default-on ABI
  checks, canonical title readiness at retrace 333, and both legal collision
  tuple classes. Final `r2` remained `0x803FFD00`.
- Diagnostic cost correction: a first long attempt was stopped at 295M blocks
  after noticing a guest-context read on every successful check. The promoted
  invariant now performs context reads only on failure; no correctness or
  stability claim is attached to the stopped run.
- Evidence SHA-256:
  `36805738aeec91baea35ddb2b1c3eec34d3a089c7e14e7c7dd267adb25a1e92b`.

## 2026-08-30 - Caller tuple captured before retail panic

- Stop: promoted `GetAttributeCode` entry guard stopped at retrace 35,734 /
  4,896,772,902 blocks, before the deterministic retail panic.
- Tuple: returned ground `-1e9`, polygon/background `65535/256`, stored ground
  `16384.0`, Acch ground `0.0`, player Y `0.0`, CR `0x40000088`.
- Result: FP branch behavior matches its operands; ownership moves to the
  player/Acch collision-state inputs that make a coherent no-ground sentinel
  consumable. DVD/ARAM/DSP/GX remained clean and the card was unchanged.
- Runtime: `1,916.88s` wall and 700,989,440-byte maximum RSS; thermal samples
  were clear. This is diagnostic evidence, not a stability pass.
- Run and thermal SHA-256:
  `b3019353ede4439cdab1a7a5403bac776b16b40691490eed47e87afe2eacc89e`,
  `a51463d0da827906c722bec6a2acb697a5172b605584dc08a6b64145c8a7547c`.

## 2026-08-30 - Exhaustive producer tier reproduces caller-side panic

- Result: signed route reproduced retail `d_bg_s.cpp:1044` at retrace 35,855 /
  4,916,701,683 blocks, then reached the 36,500 cap while halted. The run is
  failed evidence, not a normal stability stop.
- Coverage: guaranteed restore-dispatch monitor examined 319,973
  `GroundCross` returns and found no incoherent tuple. This clears producer
  return state and moves ownership to `checkLavaFace`'s translated FP compare/
  branch path before `GetAttributeCode`.
- Host: `1,942.95s` wall, 699,088,896-byte maximum RSS, clear minute thermal
  samples, and byte-identical copied card.
- Failed run, thermal, and callee-guard app SHA-256:
  `fdc8a4abb82d9103f55c86c15b1b4ef7982c23410653eb080321ac9079c1042f`,
  `29cbd80bffdc13bba0d5c08ff1406ee27964f76311ed4fc45bfb53706431f751`,
  `342f7b2b23135c4286c3b3d98058692bccf6473fdc2e67e8b48fb48774690a22`.

## 2026-08-30 - Release target reproduces panic and rejects sampled monitor

- Route: signed 216,000-retrace target with exact Omasao input, shared tuple
  monitor, copied accepted card, and minute thermal sampling.
- Result: the route reached Omasao phase 6 at retrace 21,923, then retail
  `dBgS::GetPolyId1` panicked at retrace 35,855 / 4,916,701,683 blocks. The
  process was stopped in `PPCHalt`; this is not stability evidence.
- Host observations: all 33 thermal samples were clear and the card remained
  byte-identical. The run disproves resolution of `BW-P4-0060`.
- Diagnostic correction: `0x802469F8` was an incidental internal-label sample,
  not an exhaustive host boundary. The replacement `pc=0x80328F84,
  lr=0x80246A04` restore dispatch counted 3,494 `GroundCross` returns in a
  signed 700-retrace control, covered both legal tuple classes, and preserved
  canonical title timing and card contents.
- Failed release, thermal, corrected app, and focused evidence SHA-256:
  `0d54bed67afb3d9348e74f255d19103c43e05a6953a3f14a3acbb1d18e4121ad`,
  `f434d8dbe129b04c166f0ac627e2afbc28e696fb423e6a804a18b8093a43b6de`,
  `394b15622906e6401bbfbc4e5585059e4fc90b275f115ef59c46dbe3f6de5521`,
  `09123e623118f498223e16ff2f1c07fb489448d9036604ce6bf5852a5901b259`.

## 2026-08-30 - Shared GroundCross invariant passes signed Omasao control

- Route: canonical signed Aurora app, authentic donor DSP, copied accepted
  card, exact high-clearance Outset-to-Omasao route, then neutral residence
  through retrace 35,200 with the default-on shared tuple invariant.
- Result: normal stop at 4,847,502,047 blocks after Omasao phase 6 at retrace
  21,923 and after the former 4.817B panic point. No incoherent tuple or panic
  appeared. DVD was 199/0, ARAM 5,885/0, DSP 2,988,202, and GX 61,544,650/0.
  The copied card remained byte-identical.
- Runtime: `1,878.95s` wall, `1,906.40s` user, `31.02s` system,
  695,681,024-byte maximum RSS, and 696,256,000-byte peak footprint.
- Interpretation: this validates the promoted invariant over the bounded
  route and closes the 35,200-retrace shape. The earlier authentic panic
  remains unresolved release-scale nondeterminism; this is not yet a release
  soak pass.
- App executable and evidence SHA-256:
  `0a5de6effa5945fbf46ec8b00518731926bc78ffab279d53c3a946c545a52eac`,
  `e37141e72673d8368c11067edbcfc3bbeb4c2513a67617f266a1cef149b4917b`.

## 2026-08-30 - Shared GroundCross invariant passes signed 700 retraces

- Boundary: default-on tuple check at `cBgS::GroundCross` `0x802469F8`, while
  `r31` still names the check object and the saved guest LR identifies the
  caller.
- Result: normal stop at retrace 700 / 61,421,183 blocks, canonical title at
  retrace 333, DVD 71/0, ARAM 4,310/0, DSP 71,287, and GX 152,441/0. The card
  remained byte-identical.
- Coverage: first valid tuple was `height=400, poly=3188, bg=0`; first sentinel
  tuple was `height=-1e9, poly=65535, bg=256`. No incoherent tuple appeared.
- Runtime: `26.76s` wall, `25.16s` user, `0.86s` system, and 685,522,944-byte
  maximum RSS. This validates the invariant only; it is not a performance
  promotion or long-session stability result.
- Executable and evidence SHA-256:
  `0a5de6effa5945fbf46ec8b00518731926bc78ffab279d53c3a946c545a52eac`,
  `8b1eb8617a4db647601fec989ad362cd777c6f46b469a4280cc83ae12abdb352`.

## 2026-08-30 - Instrumented Omasao collision-provenance control passes

- Route: canonical signed Aurora app, authentic donor DSP, copied accepted
  card, exact high-clearance Outset-to-Omasao input route, then neutral
  residence through retrace 35,200 with the default-on collision-provenance
  monitor.
- Result: normal stop at 4,847,502,047 blocks after clearing both the 4.396B
  trace-divergence boundary and former 4.817B panic point. DVD was 199/0,
  ARAM 5,885/0, DSP 2,988,202, and GX 61,544,650 submitted with zero rejects
  or failures. The copied card remained byte-identical.
- Provenance: the only recorded producer tuple was the legitimate no-ground
  sentinel `height=-1e9, poly=65535, bg=256` at retrace 333. There were zero
  invalid consumers and no `OSPanic`. Sentinel metadata is valid; the active
  owner is rare incoherence between a consumable height and that metadata.
- Runtime: `2,062.66s` wall, `2,098.06s` user, `30.94s` system, and
  696,598,528-byte maximum RSS. This is correctness evidence, not a speed or
  release-soak pass.
- App executable SHA-256:
  `ccd108bb61574684b1f61c39f19ca8c989dc45216120500bbbc1e10971d95299`.
  Evidence SHA-256:
  `376cef162b452475464a9ccae7c8269bfb090d5513b32554fd6909e50ba9df0b`.

## 2026-08-30 - Release-scale Omasao attempt rejected on guest panic

- Route: canonical signed Aurora app, authentic donor DSP, copied accepted
  card, complete high-clearance Outset-to-Omasao input route, then neutral
  residence toward 216,000 retraces.
- Failure: after Omasao admission at retrace 21,923 and a healthy retrace-32,768
  marker, retail `dBgS::GetPolyId1` asserted its background-index range from
  Link's `checkLavaFace`, then entered `PPCHalt`. The process was stopped after
  2,339.34 seconds wall; its 695,910,400-byte maximum RSS and later decline do
  not constitute stability because the guest was halted.
- Host observations: 39 one-minute samples reported no macOS thermal or
  performance warning. The copied card remained SHA-256
  `6b43aabd94f00ae3de01b42567b5d11026cf6a9374e4d023565b31bf3c7e1987`.
- Counterexamples: normal-priority and nice +5 signed replays both stop
  normally at retrace 35,200 / 4,847,502,047 blocks with identical guest
  summaries, DVD 199/0, ARAM 5,885/0, DSP 2,988,202, GX 61,544,650 submitted
  with zero rejects/failures, and unchanged cards. Wall/max-RSS are
  1,879.76s/696,074,240 bytes and 1,836.26s/625,426,432 bytes respectively.
- Interpretation: priority is not the owner, and the failed run cannot support
  a speed, memory, thermal, or soak pass. Duration-only repetition is closed;
  collision provenance must become deterministic before this tier resumes.
- Evidence SHA-256: failed run
  `94a049f4d422299b84c47406fca3b7a7440f271a28941a65c81ed4ea044e76f0`;
  thermal samples
  `78d6ed0215fde6d4b37dc434eee31bf51ab5442cdaad96d14d5f7505b1d8bfca`;
  normal control
  `1953a3a8b553fd8dd337ba7315da7ae73a5a06f0b8b784715c16d9c881cdb0e9`;
  nice +5 control
  `c4f585f47ac272cada4dfbefc58503d23206c9a1fa08226468a3e89ed48e73fe`.

## 2026-08-29 - Rejected full-composite O2 build

- Hypothesis: the original P3 `-O1` policy might leave useful optimization in
  the translated execution subsystem, which owns about 30% of the latest clean
  visible profile.
- Candidate: compile all 748 canonical generated C units at `-O2` while
  retaining `-ffp-contract=off`; no runtime behavior was changed.
- Result: rejected on build cost before runtime. The isolated build was stopped
  after `10,408.68s` wall, `52,800.43s` aggregate user CPU, `934.49s` system,
  and 2,639,020,032-byte maximum RSS, with three compiler jobs unfinished.
  One REL unit had run for 2h53m and DOL units for as long as 45m.
- Scope: no dylib was linked, so there is no correctness or speed result. The
  temporary optimization-level option was removed; canonical `-O1` source and
  app artifacts are unchanged.
- Decision: close whole-composite optimization-level escalation. Revisit only
  after a generated-code shape or partitioning change supplies a bounded
  compile-cost hypothesis; do not proceed to `-O3` or whole-composite LTO.

## 2026-08-29 - Exact LLVM v27 completes 700 retraces but regresses the C route

- Build: cold exact AArch64 translation in `1703.78s`, producing 6,571 native
  DOL objects, 6,571 bitcode companions, 6,571 cache keys, and zero unknown
  instructions. The 7,113-chunk/417-range/415-REL composite passes ABI coverage
  and strict signing.
- Correctness: whole-game IR admits `fcmpu` `0x802AFBF0` and `ps_merge00`
  `0x802EE704` as normal structural entries while memory/PSQ first use retains
  cold retry suffixes. The fresh-card run reaches retrace 700 with no fallback
  or exception, title-ready at 333, DVD 70/0, ARAM 4,295/0, and 36,429 DSP DMAs;
  the first nonzero DMA remains at retrace 354.
- Runtime: `44.20s` wall, `42.51s` user, and `0.33s` system for 105,530,781
  translated dispatches. Maximum RSS is 184,139,776 bytes.
- Comparison: the current C route's accepted 700-retrace control is about
  `27.8s` and 82.3 million dispatches. V27's structural admission closes the
  correctness blocker but removes the native route's performance case.
- Bound calibration: an initial 100-million-block run stopped normally at
  retrace 675. It is retained as a control, not counted as the acceptance run.
- Promotion: rejected. Do not replace the canonical signed-app dylib with v27.
- Dylib SHA-256:
  `782edf96b1614e95536cd0e5d8dde06f27f8b2a6fb0b5cebd4d1c59c80c71a30`.
  Accepted evidence SHA-256:
  `52b3a62ece5f21ffb65ee4200960ae9890fc1394e92ff7f2db8e5b0e1cc925a2`.
  Underbound control SHA-256:
  `6cc7c510cef866348b0903e93d107506dc91a30f1ad8b6c3cf423f253fc1c9a5`.

## 2026-08-29 - Exact LLVM v26 clears paired-register retry, exposes subsystem owner

- Build: 6,571 exact AArch64 DOL objects plus bitcode, passing ABI coverage
  and strict signing; dylib SHA-256
  `1a11a56654013763cbb5bc7507067da3a7fe37fa8493423555061163f778c82e`.
- Correctness: native re-entry at `ps_merge00` `0x802EE704` now succeeds and
  title readiness remains canonical at retrace 333.
- Runtime result: at retrace 397, lazy-FP resume reaches register-only `fcmpu`
  `0x802AFBF0`, which v26 did not admit as a native entry. The process stops
  after 37,747,871 blocks with 4,295 accepted/zero-rejected ARAM transfers and
  16,781 donor DSP DMAs.
- Timing: `16.33s` wall and `11.96s` user through the new boundary. This is
  correctness evidence, not a promotion benchmark.
- Decision: the repeated experiment shape invokes anti-stall. v27 promotes
  register-only FP first use to structural native entries rather than adding
  another opcode-specific cold suffix.
- Evidence SHA-256:
  `278a5b79946feeff6a9fb641399d443e0da4f34c48b6c497dcb6781cea2a6dcc`.

## 2026-08-29 - Exact LLVM v25 reaches title before native re-entry fault

- Build: 6,571 exact AArch64 DOL objects plus bitcode, 7,113 composite chunks,
  417 ranges, 415 REL modules, passing ABI coverage and strict signing.
- Correctness repair: generated tagged addresses now retain bit 30, and the
  host external-memory gateway resolves canonical RAM/aliases before MMIO.
  The focused gateway test and all 22 runnable host/runtime tests pass.
- Runtime result: a fresh-card run resolves profile 5, advances scene phase to
  4, and reaches title-ready at canonical retrace 333. At retrace 382 it exits
  on unsupported fallback for `ps_merge00` at `0x802EE704`; v26 has focused
  retry coverage but has not yet been regenerated at full-game scale.
- Timing to the new boundary: `12.31s` wall, `11.09s` user, `0.27s` system for
  382 retraces, with maximum RSS 183,025,664 bytes. This is correctness and
  potential evidence, not a promotion benchmark because the run terminates
  before the 700-retrace comparison bound.
- Artifact SHA-256:
  `7d16660817e51eb9f4af50715a4b3fe1a79915da27341241431d9d9582e72123`.
  Evidence SHA-256:
  `bbc3c3a5dc0959eefb16504c3380c3b363523c8ce2d94d1dd5fb8367f450dc3d`.

## 2026-08-29 - Signed Omasao stability extension

- Route: clean canonical ThinLTO signed Aurora app, donor DSP, copied card,
  complete high-clearance Outset-to-Omasao route, then neutral indoor
  residence through retrace 27,000.
- Correctness: all accepted admissions repeat exactly through Omasao overlap
  phase 6 at retrace 21,923. The process stops normally at 4,086,119,878
  blocks with 42,915,077 GX submissions and zero rejects, failures, skips, or
  vertex-decode failures; DVD 199/0; ARAM 5,647/0; and 2,330,330 donor DSP
  DMAs. The copied card is byte-identical before and after.
- Duration: `1395.65s` wall, `1410.30s` user, and `24.90s` system for 450
  seconds of guest time, or about 3.10 times slower than real time. The
  post-transition extension is 5,077 retraces, about 84.6 guest seconds.
- Memory: maximum RSS is 700,891,136 bytes and peak physical footprint is
  695,911,936 bytes. Coarse live samples remain around 431-460 MB after the
  transition, and the maximum remains below the prior doubled-Outset run's
  742,506,496 bytes; this bounded run shows no progressive growth.
- Scope: advances signed indoor stability only. It is not a 60-minute thermal
  workload, three-hour soak, unrestricted-play qualification, or proof of
  original speed.
- Artifact SHA-256: app executable
  `1f2906fbb5d4dba791f029cd8dcb7343bd3333f21dbb28c71ba9d9ec55bc5bab`;
  canonical composite
  `3f0abcdda614c2ec9c82e63f7fc3dff99a3d14de27dc6a4df9cd01b08449e665`.
- Evidence SHA-256:
  `1ad87f970a0fe99bbeb57e618b9ddc9d3eb92a0a5c7699c9ef2d71e6c8b8be1a`.

## 2026-08-29 - Exact LLVM v24 reaches the full bound

- Build: cache-disabled exact AArch64 GZLE01 translation, 6,571 native Mach-O
  objects and bitcode companions, zero unknown instructions, `1,826s` by
  output timestamps. The hybrid composite contains 7,113 chunks, 417 ranges,
  415 REL modules, and 8,079 REL sections; ABI coverage passes.
- Correctness: native retry entries clear direct and indexed scalar FP plus
  paired-single first use. The authentic fresh-card run mounts `RELS.arc`,
  initializes cDyl, links module 1, starts donor DSP work, and reaches the
  normal 700-retrace bound with no fallback or exception.
- Runtime: `20.70s` wall, `16.33s` user, `0.11s` system for 11.67 seconds of
  guest time, approximately 1.77 times slower than realtime. Maximum RSS is
  100,679,680 bytes.
- Digest: 35,843,394 blocks, 2,340 accepted/zero-rejected ARAM transfers,
  28,566 donor DSP DMAs, final PC `0x80307EF4`.
- Promotion status: rejected for now. Process 5 remains in scene creation and
  title readiness is zero, so speed and bounded stability are not yet an
  authentic boot-equivalence result.
- Artifact SHA-256:
  `a9bad2ab044ff190edd9572d566685096f2f006ede2875eaafafe00b56fed08e`.
  Evidence SHA-256:
  `58d940ebf2cfc555b330d96fa0a23f47e8acdbaf09e07d584b2480d5040bebba`.

## 2026-08-29 - Exact LLVM DOL candidate, correctness not yet equivalent

- Build: cache-disabled exact AArch64 translation of the real GZLE01 DOL
  emitted 6,571 arm64 Mach-O objects in `1,487.29s`. The hybrid composite has
  7,113 chunks, 417 code ranges, 415 REL modules, and 8,079 REL sections; ABI
  coverage passes.
- Correctness repair: scalar lazy-FP retry address `0x802AE070` is now a
  native switch entry. A focused retry fixture and the full 28-test
  DolRecomp suite pass, and the authentic host no longer falls into the
  interpreter at that address.
- Runtime: the bounded 700-retrace run takes `25.49s` wall, `18.57s` user,
  and `0.26s` system, versus roughly 28 seconds for the current canonical C
  DOL baseline and over four times guest duration on the long visible route.
- Rejection from promotion: the fast run is not an exact digest. It stops
  after 18,603,151 blocks at retail `PPCHalt`, with no title, DVD forwards, or
  DSP DMAs, because `cDyl_InitAsync`/`RELS.arc` initialization was skipped.
  Performance numbers establish potential only; they do not qualify the
  route for the app.
- Dylib SHA-256:
  `6bdc100d8478693843303b50f3da062d66a1996ced240a53eed7eb65c5e8c192`.
  Evidence SHA-256:
  `83afe924bfb613be8e53ae4ec42d3d52bf129a4555411962f02ff2292da759ee`.

## 2026-08-29 - Observation-only AI-DMA WAV capture

- Change: `BLUEWAKE_CAPTURE_AUDIO_WAV` records the authentic guest AI-DMA
  stereo stream immediately before the unchanged platform push. It opens
  lazily at the guest-selected rate and reports frames, nonzero samples, peak,
  and stream hash when the RIFF sizes are finalized.
- Exactness: serialized headless no-capture/capture runs both stop normally at
  49,031,333 blocks with identical clock, title/DSP milestones, device counts,
  scene counts, and final PC.
- Cost: the 500-retrace control takes `30.67s` wall/`29.13s` user; capture
  takes `32.00s`/`30.27s` while writing 1,062,636 bytes. Capture remains an
  explicit observation facility and is absent from production I/O by default.
- Artifact: Aurora and headless WAV files are byte-identical at SHA-256
  `d7a61ce4761ff04358ef50242ef82f38a9ed5959fafc6a50eb649f3c03f3a1de`.
  The stream is 8.3015 seconds of 32 kHz stereo PCM, with 143,942 nonzero
  samples, peak 1,542, mean -45.7 dBFS, and peak -26.5 dBFS.
- Evidence SHA-256 control/candidate/Aurora logs:
  `19ca204e9a654501ee5e31563da9fe7af6356a57440d0650fbe08c26cf18ed30`,
  `1c2acea2494a7455826057e0f97ad5360a003af054adcf80d2138cb05429191b`,
  and `1d32eecc9bb14ef8336671b7f4d509469c8f32a83133b2e90bdaf426bb8bce61`.

## 2026-08-29 - Doubled signed Outset gameplay interval

- Route: canonical signed Aurora app, current composite and donor DSP, complete
  unskipped opening and authored event 38, then 3,600 retraces of player-ready
  forward-stick input.
- Result: normal stop after 4,054,012,720 translated blocks and 23,525
  retraces. Movement and camera response both pass; DVD, ARAM, DSP, and GX
  retain zero-failure/reject route summaries.
- Duration: `1598.85s` wall, `1618.80s` user, and `32.94s` system for about
  `392.08s` of guest time, approximately 4.08 times slower than real time on
  this Outset route.
- Memory: maximum RSS is 742,506,496 bytes and peak physical footprint is
  694,912,512 bytes. Observed resident memory falls after transient opening
  and world-load assets; this bounded run shows no growth trend.
- Scope: this doubles the prior gameplay interval, but is not a 60-minute
  thermal/energy soak or an unrestricted representative-play benchmark.
- Evidence SHA-256:
  `fc35e6a1a0baa0291ea6c0530c4a45975f35de0e0c6b6fc8de4fb1382ef8c65d`.

## 2026-08-29 - Default-off legacy scheduler observation

- Cause: a clean signed Aurora profile still emitted 415 scheduler records
  with no trace environment variables. Early VI/message latches kept several
  historical observation blocks active, including a per-dispatch walk of
  guest video-queue and run-queue state.
- Change: those observation-only blocks now require the existing
  `BLUEWAKE_TRACE_RUNQUEUE` owner. Authentic context restoration, PE-finish
  delivery, the message readiness latch, and independently requested audio
  tracing retain their former paths.
- Correctness: every 700-retrace run preserves exact clock state, 82,291,602
  blocks, 36,443 DSP DMAs, title and first-nonzero milestones, DVD/ARAM
  counts, final PC, and normal stop. The trace-on 120-retrace route still emits
  4,416 scheduler records and stops normally.
- Matched timing: first control is `31.29s` wall/`29.75s` user; the immediately
  following candidate is `28.31s`/`27.01s`; reverse control is
  `31.75s`/`30.15s`. The final promoted build repeats in
  `27.81s`/`26.81s`. Retired instructions fall from approximately 391.08
  billion in both controls to 381.72 billion in the matched candidate and
  381.39 billion in the promoted build.
- Noise retained: an earlier candidate run took `33.06s`/`31.96s` under much
  heavier host scheduling. It is not discarded and does not alter the exact
  guest digest.
- Verification: 19/19 runnable tests, full signed-app build, strict signing,
  trace-on behavior, repository audit, and diff checks pass.
- Evidence SHA-256 first control/candidate/reverse control/promoted:
  `22f99d0303528778b9f0f0b06954db603bf624c5e73a52cad3f0e3c1c82bcd7d`,
  `db0d01f600a10a80358e44085a841655c083e86d737734ba6d6e500347f238aa`,
  `c5c65a5b03db5eb10ab6e6b6e301ba2d9b97e2b35cdf948f23b4aba74a61adaf`,
  and `c223c4c1c901030654f2a8b4788bddb40706bc0ea49c1a6ba88d207304daf83a`.
- Profile evidence SHA-256 control/candidate:
  `e4fd49f2de64dd4be1cfd8b7ba07b7a63c4cab969cf725d461f8dfc87791865e`
  and `9f1aa7a391de3a97ed88f1fc7f34f4b9fee3f921cfd47ca3638dea776c0994c2`.

## 2026-08-29 - Rejected CARD dispatch address envelope

- Cause: the main loop called `bluewake_card_runtime_dispatch` for every PC;
  ordinary addresses paid a large function prologue and compare tree before
  returning false. The owner measured 29 normalized top-of-stack samples.
- Candidate: a call-site predicate admitted only the exact minimum/maximum
  span containing all synchronous and asynchronous CARD entries. Queued
  asynchronous callback service remained unconditional and unchanged.
- Correctness: both authentic 700-retrace orders preserve the exact clock,
  block, DSP, title/audio, DVD/ARAM, final-PC, and normal-stop digest.
- Timing: first order is `27.92s` wall/`26.38s` user control versus
  `28.16s`/`26.77s` candidate. Reverse order is `27.33s`/`26.59s` candidate
  versus `27.48s`/`26.63s` control.
- Decision: reject as below end-to-end resolution; no source change remains.
  Do not continue with another tiny dispatch-boundary optimization from this
  sample.
- Evidence SHA-256 first-order control/candidate:
  `4d09edacb7f2a52777822a55e594025c542300e8ead9ae7d4b0fa393051ad1dd`
  and `ac21a6493c84e191019a4e4e852b9e8b8ae00308524458805880e1923c3db210`;
  reverse candidate/control:
  `1376b1d86126ca9cdbc2b8abe6ea862e5d9938ef601ee6ae63cff87ca531be8c`
  and `947c97340b85294facb7a244b1fca25756dff03c646b72fbe6bfec925563e601`.

## 2026-08-29 - Rejected composite FP-availability inline

- Hypothesis: 42 normalized samples remained in `ppc_fp_available` because
  composite-generated chunks called the runtime helper for every FP operation,
  even after MSR[FP] was set.
- Candidate: restore DolRecomp's exact inline MSR[FP] success check; retain the
  original helper whenever the bit is clear so lazy-FP exception semantics are
  unchanged. This required a full 748-unit rebuild.
- Correctness: every authentic 700-retrace run preserves the genuine
  FP-unavailable boot fault, clock state, 82,291,602 blocks, 36,443 DSP DMAs,
  title/audio milestones, DVD/ARAM counts, final PC, and normal stop.
- Timing: results do not repeat. A reverse pair gives `32.75s` wall/`31.21s`
  user candidate versus `32.81s`/`31.61s` control. A later reverse pair gives
  `26.95s`/`26.16s` candidate versus `28.32s`/`27.36s` control, but its
  immediate forward repeat gives `28.32s`/`27.36s` control versus
  `28.57s`/`27.72s` candidate.
- Decision: reject as neutral/noisy; no source change remains. Do not repeat
  this isolated call-boundary shape without materially stronger ownership.
- Evidence SHA-256 final control/candidate:
  `cd331cf997fe29f2702c5ee093e59d4feb2e399bbd5be72695aa40831974f3a848`
  and `60bddbf115db28a6bddc65a787ff09e23eb561519d6bc14188500c8054b89b30`.

## 2026-08-29 - Exact translated chunk dispatch cache

- Cause: the generated dispatcher cached only its most recently selected
  chunk, while normal execution interleaves enough of the 748 chunks to miss
  that cache and repeat a binary search.
- Change: a 64-slot direct-mapped cache uses a mixed 16-KiB chunk key. Hits
  still require exact start/end containment and instruction alignment;
  collisions and misses execute the unchanged binary search and then fill the
  selected slot.
- Matched result: candidate CPU time improves in all three authentic
  700-retrace pairs. Pair 1 is `28.09s` control versus `31.74s` candidate wall,
  with user time improving from `27.24s` to `26.82s`; the candidate wall time
  contains an external stall. Reverse order gives `27.81s` candidate versus
  `29.34s` control. Pair 3 gives `27.69s` candidate versus `29.43s` control,
  with user time improving from `28.27s` to `26.74s`.
- Correctness: all six runs preserve exact clock state, 82,291,602 blocks,
  36,443 DSP DMAs, title/audio milestones, DVD/ARAM counts, final PC, and
  normal stop.
- Visible profile: Aurora reaches title and genuine donor output with 53,582
  zero-failure GX submissions; `selected_dispatch` falls from 99/787 to
  25/1,094 normalized main-thread top-of-stack samples.
- Promotion check: the canonical dylib has SHA-256
  `3f0abcdda614c2ec9c82e63f7fc3dff99a3d14de27dc6a4df9cd01b08449e665`
  and repeats the exact digest in `27.87s` wall and `27.02s` user. Its evidence
  SHA-256 is
  `f8c4f5c004196b0ff3cdd7d85056e8d90678a5cc183142883f7de4f94fe1c740`.
- Evidence SHA-256 pair-3 control/candidate:
  `44241025ec85feb53cf2a06623f9e58b0361f97d4e274d6cfac30c19da7698e0`
  and `74b9c608e0bb7f4fc4e14a19f2c0a602fdd25578894b3661c46e0f8dfff66cfc`;
  visible sample:
  `9409076102abcf65bba6639d06d046591ae40a72f07f7a8bd6fa01dbfdffa7bc`.

## 2026-08-29 - Raw REL PC alias envelope

- Cause: 17 live raw executable aliases were scanned twice per translated
  dispatch even when the PC was ordinary DOL code or tagged linked code.
- Change: alias add, compaction, and boot reset maintain a conservative raw-PC
  minimum/maximum. Outside PCs reject immediately; inside PCs retain the exact
  first-match scan.
- Matched result: the authentic 700-retrace route falls from `28.86s` wall and
  `27.79s` user to `27.15s` and `26.15s`, reductions of 5.9% each. Reverse
  order confirms `27.22s` candidate versus `28.41s` control.
- Correctness: both orders preserve exact clock state, 82,291,602 blocks,
  36,443 DSP DMAs, title/audio milestones, DVD/ARAM counts, final PC, and
  normal stop.
- Visible profile: Aurora submits 53,582 packets with zero rejects/failures;
  `host_alias_rel_pc` falls from 183/1,453 to 17/787 normalized main-thread
  top-of-stack samples.
- Evidence SHA-256 control/candidate:
  `b27d77c796875d63c03f4b81e82f997c060b96caab799fef53fab3fcfa608500`
  and `df0771280a49a2727516ab16594003e5fe531156e0003b3383cc13454f2da855`;
  reverse-order candidate/control:
  `5676d7297cc54f476fa7f47f49ffd32059b8b04deb1637294d7397f0541d1d79`
  and `c02380810cc0acfe3d2c6615e16834784456291f7ad22ec0da81cb924303b391`.

## 2026-08-28 - Exact fast-forward for stable translated MEM1 polls

- Hypothesis: a generated polling loop cannot observe a change to unaliased
  physical MEM1 until control returns to the host dispatch boundary, where
  device and guest-thread events are delivered.
- Change: DolRecomp recognizes the generic `lwz`/`cmplwi 0`/back-branch shape
  and consumes the remaining loop budget with exact cycle subtraction when
  GXRuntime proves the read is cached, physical, unaliased MEM1. All other
  address classes keep ordinary per-iteration execution.
- Matched result: the authentic 700-retrace route falls from `32.02s` wall and
  `31.16s` user to `29.50s` and `28.92s`, reductions of 7.9% and 7.2%.
  Reverse order confirms `29.65s` candidate versus `32.51s` control.
- Correctness: both orders preserve exact clock state, 82,291,602 blocks,
  36,443 DSP DMAs, title retrace 333, first nonzero DMA retrace 354/hash
  `0x403BBFA8`, DVD/ARAM counts, final PC, and normal stop.
- Visible profile: Aurora reaches title and genuine donor output, submits
  53,582 packets with zero rejects/failures, and records only six top-of-stack
  samples in `loop_80307EF4`.
- Promotion check: the fully rebuilt canonical dylib is byte-identical to the
  isolated candidate at SHA-256
  `24e6947fb5d3621ce0198ef585d2491113595c35a15677075cc1d47bbf151e83`
  and repeats the exact digest in `29.64s` wall and `28.59s` user. Promoted-run
  evidence SHA-256:
  `5206aa824f872d1142ef1d306f1dabd67f929aed6a3dcc112f3ec445dd6e8766`.
- Evidence SHA-256 control/candidate:
  `4d8d70c550556cffe902391672d1744e970734f179210807d6f94ac91fddcf40`
  and `a86b4f2dee0d587638d03fb4f7cb28cf80073101fe5b1b8fd6215235807b34f8`;
  reverse-order candidate/control:
  `89b907bbd05f48e4e80f55e554056cb17b4338933b7603e1d9f8efd8a05a6cae`
  and `c2e9cc0b08a85ee94fffb74f1de83082ddab7e273e002c5c094bff604bcdbe70`.

## 2026-08-28 - Target-scoped ThinLTO for the donor DSP route

- Hypothesis: after three isolated DSP call-boundary experiments triggered the
  anti-stall rule, optimizing the donor/adapter/host subsystem as one link unit
  could remove cross-module overhead without another semantic source change.
- Change: the explicit DSP-enabled macOS build now requires its donor CMake
  tree to have IPO enabled and applies ThinLTO to the adapter, its tests, and
  `bluewake_host`. GXRuntime is deliberately not built with LTO.
- Matched result: against the current composite, the authentic 700-retrace
  control takes `32.30s` wall and `31.48s` user; the target-scoped candidate
  takes `30.96s` wall and `30.28s` user, reductions of 4.1% and 3.8%.
- Correctness: clock state, title retrace 333, first nonzero DSP DMA retrace
  354/hash `0x403BBFA8`, 82,291,602 blocks, 36,443 DMAs, DVD/ARAM counts, final
  PC, and normal stop match exactly.
- Reproduction: `scripts/build_macos_dsp_host.sh` configures and builds the
  pinned donor and host. CMake fails if the donor cache does not record IPO.
- Verification: 19/19 runnable CTests, CPU ABI, ABI/address coverage,
  zero-unknown translation audit, 16/16 composite tests, repository audit,
  full app build, and strict signing pass.
- Provenance correction: an earlier `37.07s`/`36.11s` pair used the shared
  composite built before the MEM1 fast path. Its internally matched result is
  superseded by this current-artifact pair.
- Visible profile: a real Aurora/Metal run reaches title, genuine donor output,
  and a 53,582-submission zero-failure stop. DSP scheduling owns 2,913/8,121
  main-thread samples (35.9%), translated dispatch owns 2,481 (30.6%), and
  guest-alias resolution has one sample.
- Evidence SHA-256 control/candidate:
  `c5f4c19cdf58ca05d6e1c49ee205a307175ea38447199b8303bbb1186acf89c7`
  and `ed6056b3c321d2e23f1f23c19e790fc038f0d5d8147f9dc54747a536f0bb0d0e`;
  visible sample SHA-256:
  `90304c072397918caf3b4dbb67a91977f8303d302dc291fa3b15d4d34d152da6`.

## 2026-08-28 - Rejected DSP register-helper inline

- Hypothesis: `ConditionalExtendAccum()` owned 198 top-of-stack samples after
  instruction-fetch promotion, so exposing its unchanged register checks and
  sign extension at each opcode call site might remove useful call overhead.
- Matched result: the 700-retrace candidate takes `37.38s` wall and `36.67s`
  user versus `37.25s` wall and `36.01s` user for the freshly rebuilt control.
- Correctness: clock state, 82,291,602 blocks, 36,443 DMAs, title/nonzero-DMA
  milestones, 70 zero-failure DVD forwards, 4,295 zero-reject ARAM transfers,
  final PC, and normal stop match exactly.
- Decision: reject; no source change remains. This is the third experiment of
  the one-DSP-call-boundary shape after decode and fetch work, so anti-stall
  closes further isolated helper-inline trials from this profile.
- Evidence SHA-256 control/candidate:
  `794ddefdedbbd93ec01c9d1e6b5e16a152c10fdfe68ae7209b85c0f882d5c4f6`
  and `ca64edc5fef1f7299049de3437f3950507b22b2c0981655698ae4b00edb11819`.

## 2026-08-28 - Inline common DSP instruction fetch

- Cause: after predecoded dispatch, `SDSP::FetchInstruction()` remained an
  out-of-line call per opcode and owned 552 top-of-stack samples. Its callee
  already contained the complete IRAM/IROM switch and PC increment.
- Change: RecompCore `9eb90469b7` moves that unchanged common path into inline
  `SDSP` methods. Invalid instruction-memory addresses still call the original
  logging behavior through a cold fallback.
- Signed-executable headless observation: the 420-retrace route falls from
  `16.73s` to `15.62s` and `FetchInstruction` disappears. Total short-sample
  DSP ownership shifts
  from 38.2% to 39.7%, so sampling alone is considered noisy rather than proof.
- Matched headless A/B: 700 retraces take `32.93s` control and `32.22s`
  candidate, a 2.2% wall reduction (`31.60s` to `31.25s` user).
- Correctness: both headless logs have exact clock state, 82,291,602 blocks,
  36,443 DSP DMAs, title retrace 333, first nonzero DMA retrace 354/hash
  `0x403BBFA8`, 70 zero-failure DVD forwards, 4,295 zero-reject ARAM transfers,
  final PC, and normal stop.
- Verification: 19/19 runnable CTests, exhaustive decode test, CPU ABI,
  ABI/address coverage, zero-unknown translation audit, full app build,
  repository audit, and strict signing pass.
- Evidence SHA-256 control/candidate:
  `01cad623b35a5afb1ef9c94c82f86267c2644eb1f6cd35044968f682ec31741e`
  and `a898c74687c575edd627a64eaddc73863690c52ee1a14ab213dbbde144b02214`.
- Next owner: return to Step 0. Do not repeat fetch, decode-table,
  exception-guard, DSP-batch, alias, or texture-hash experiments.

## 2026-08-28 - Automated opening and save-reload extension

- Route: strictly signed macOS app, canonical tagged composite, donor DSP,
  private `GZLE01` disc, Aurora/gxcore, title-ready PAD automation, default
  card, 2,400 retraces.
- Progression: file select at 677, name input complete at 855, opening request
  at 857, opening asset at 917, and opening actor states through 9.
- Result: 217,551 donor DMAs; 73 DVD forwards with zero failures; 4,310 ARAM
  transfers with zero rejects; normal stop after 156,787,088 blocks.
- Performance: `78.93s` wall, `77.30s` user, and `0.91s` system for `40s`
  guest time, approximately 1.97 times slower than real time on this route.
- Persistence: the retail route changes the card from SHA-256 `26f5f889...`
  to `b0163d86...`. A separate signed 700-retrace process opens the existing
  98,304-byte file, reads both mirrors and the picture region, reaches file
  select at 537, and leaves the new card byte-identical. The reload takes
  `28.53s` wall for 11.67 seconds guest time.
- Scope: cross-process save mutation/reload and bounded opening progression
  pass; complete opening and gameplay stability remain open.
- Extension evidence SHA-256:
  `dffe842130de37d0a046e53cbacfbc247c251fe2626c0ae4cfdf9797b911d737`;
  reload evidence SHA-256:
  `d8bfe74a404163dc423e1ec96cc0388a554eb1b3544bb89e9d94af85d5913e88`.

## 2026-08-28 - Signed 60-second guest-time title tier

- Route: strictly signed macOS app, canonical tagged composite, donor DSP,
  private `GZLE01` disc, Aurora/gxcore, default card open, 3,600 retraces.
- Result: title-ready at retrace 333; first nonzero DSP DMA at 354/hash
  `0x403BBFA8`; 263,726 donor DMAs; 70 DVD forwards with zero failures; 4,295
  ARAM transfers with zero rejects; normal stop after 594,583,242 blocks.
- Persistence: card SHA-256 remains byte-identical before and after at
  `26f5f889bde29958c4fdaa6ef0fc63346ff3624dc6f7e95a11cf6cbc9227bc34`.
- Performance: `219.67s` wall, `213.95s` user, and `3.36s` system for `60s`
  guest time, approximately 3.66 times slower than real time.
- Scope: no title-button automation was active, so this is a sustained title
  tier, not file-select, opening, save-reload, or gameplay stability.
- Evidence SHA-256:
  `9ac550170c489484b5b9702c8ae5204ed6a8dc0ab0691ae4c4eee1f71d7147e3`.

## 2026-08-28 - Predecoded DSP interpreter dispatch

- Route: strictly signed macOS app executable in headless mode, canonical
  tagged composite, donor DSP IROM and COEF, private `GZLE01` disc, 420
  retraces. This is not an Aurora presentation measurement.
- Cause: without LTO, `ExecuteInstruction` made out-of-line calls to
  `GetOpTemplate`, `GetOp`, and, for extended instructions, `GetExtOp`, then
  loaded the extended flag twice.
- Change: RecompCore `16308ba1c4` constructs one immutable 65,536-entry table
  from those exact legacy results. Each entry stores the main member function,
  extension member function, and extended bit. Execution order remains
  extension, main, then writeback.
- Correctness: the donor-adapter test compares every combined entry with all
  three legacy accessors for all 65,536 instruction words. Two signed candidate
  runs reproduce the same current clock/final-block digest, title retrace 333,
  first nonzero DMA retrace 354/hash `0x403BBFA8`, all 18,008 DMAs, 70
  zero-failure DVD forwards, 4,295 ARAM transfers, and a normal stop.
- Result: legacy lookup functions total 375 samples in the accepted precursor;
  the combined accessor has 134 in the candidate. DSP scheduling falls from
  3,412/8,084 to 3,202/8,384 samples. A timed candidate takes `16.73s` versus
  the preceding coarse `19s` control, roughly 12% lower but not a real-time
  result.
- Verification: 19/19 runnable CTests, exhaustive decode equivalence, CPU ABI,
  ABI/address coverage, zero-unknown translation audit, full host build,
  repository audit, and strict signing pass.
- Evidence SHA-256:
  `bf9765518a0a8568d2a6729fecf18e9907ec73923e97783774df4266a908214c`;
  sample SHA-256:
  `d2a6c36625adac76f96cb60a1d46029299d5a7db77b55e9cc99220b38fdb71f1`.
- Next owner: reorient from Step 0. Do not repeat opcode-table, exception-guard,
  DSP-batch, alias, or texture-hash experiments.

## 2026-08-28 - Deliverable-only DSP exception checks

- Route: strictly signed macOS app, canonical tagged composite, donor DSP IROM
  and COEF, private `GZLE01` disc, Aurora/gxcore, 420 retraces.
- Cause: the interpreter called `CheckExceptions()` before every instruction.
  The donor often retains a pending normal exception while `SR_INT_ENABLE` is
  clear, so the handler repeatedly scanned and declined the same masked bit.
- Change: call the unchanged handler at the same instruction boundary only
  when normal interrupts are enabled or the unmaskable external-interrupt bit
  is pending. This mirrors the handler's own delivery predicate; it does not
  defer a deliverable exception or change interpreter batching.
- Result: `CheckExceptions()` leaves the sampled hot path. Exact DSP scheduling
  falls from 4,042/8,118 (49.8%) to 3,412/8,084 (42.2%) samples; coarse wall
  time falls from 20 to 19 seconds. The matched first nonzero DMA, all 18,008
  DMAs, clock state, and final block count are unchanged.
- Correctness: title retrace 333; first nonzero donor DMA retrace 354/hash
  `0x403BBFA8`; 70 DVD forwards/zero failures; 53,558 GX submissions/zero
  rejects or failures; 35,820,091-block normal stop.
- Verification: signed app build, strict signing, 18/18 runnable CTests, CPU
  ABI contract, ABI/address coverage, and repository audit pass.
- RecompCore: `1deab5723b`; evidence SHA-256:
  `12c25cfb068077833b93ed17f8234cb8d51a0e4c8e21475cb9393696b35e2875`;
  sample SHA-256:
  `9b91d1d07af63c41712b480c12b8f4335cb3c8ba26cf8c6dd8007d5c54a2b3b2`.
- Rejected precursor: checking only `exceptions != 0` was performance-neutral
  because masked bits remain pending. Evidence SHA-256:
  `d3f6bdca8934894344b0adcef217c61383238413d0216a7a4144433d415e860c`.

## 2026-08-28 - Unshadowed MEM1 alias fast path

- Route: strictly signed macOS app, canonical tagged composite, donor DSP IROM
  and COEF, private `GZLE01` disc, Aurora/gxcore, 420 retraces.
- Cause: `get_ram_ptr()` called `ppc_guest_alias_resolve()` before every
  ordinary MEM1 access even when all 1,900 installed aliases were in the
  verifier-enforced tagged REL aperture. The resolver owned 731 samples from
  the retail idle loop in the preceding profile.
- Change: the alias registry maintains whether any alias overlaps physical
  MEM1. Untagged MEM1 accesses bypass alias resolution only while no alias can
  shadow that range; registering a low alias immediately restores the original
  alias-first precedence. Existing low/tagged/shared/remove alias tests pass.
- Result: coarse wall time falls from 23 to 20 seconds for the matched route.
  Guest-alias resolution falls from 731 to 5 samples; translated dispatch falls
  from 3,033/8,079 (37.5%) to approximately 2,184/8,141 (26.8%). Exact donor
  DSP scheduling is now the clear owner at 4,032/8,141 (49.5%).
- Correctness: exact title retrace 333; first nonzero donor DMA retrace 354/hash
  `0x403BBFA8`; 18,008 DMAs; 70 DVD forwards/zero failures; 53,558 GX
  submissions/zero rejects or failures; exact clock state and 35,820,091-block
  normal stop.
- Verification: full tagged composite and signed app build, strict signing,
  18/18 runnable CTests, six serial native Metal fixtures, CPU ABI contract,
  ABI/address coverage, and repository audit pass.
- RecompCore: `7506b313be`; evidence SHA-256:
  `b2ccdd53df7bc2610bba346fca3e8b1adb8c2a84833abe4efea8c9e37d34e18e`;
  sample SHA-256:
  `bf38f904b7e0b1658c106e0eb17e39260d7228020b55ed1a9a86a38afa72b301`.
- Next owner: exact donor DSP interpretation. Do not increase the accepted
  eight-cycle batch or repeat the alias/420-retrace experiment shape.

## 2026-08-28 - Post-palette-fix owner profile

- Route: strictly signed macOS app, canonical tagged composite, donor DSP IROM
  and COEF, private `GZLE01` disc, Aurora/gxcore, 420 retraces.
- Host: Apple M2 arm64 macOS; no Simulator; one native process.
- Result: title-ready at retrace 333; first nonzero donor DMA at retrace 354
  with hash `0x403BBFA8`; 18,008 DMAs; 53,558 GX submissions; zero DVD/GX
  failures; normal stop after 35,820,091 translated blocks. The coarse launch
  wrapper measured 23 seconds.
- Texture result: 76,191 hashed lookups, 21,392 bounded palette hashes, and no
  seeded long-hash hotspot. Only one sample reached the generic long-hash
  routine. The accepted palette-span fix has therefore moved texture hashing
  out of the dominant profile.
- Current CPU owners: 3,510/8,079 main-thread samples (43.4%) below
  `host_dsp_advance_schedule`, principally Dolphin's exact DSP interpreter;
  3,033/8,079 (37.5%) below `selected_dispatch`. Within the translated side,
  2,233 samples are in the retail OS idle function and 731 resolve guest
  aliases from that loop.
- Decision: protect the accepted eight-cycle DSP cadence. Investigate a
  compatibility-preserving ordinary-MEM1 alias rejection or an exact donor
  interpreter reduction; do not increase DSP batching, reopen texture
  generations, or repeat the 500-retrace acceptance shape.
- Evidence SHA-256:
  `23b5f7c185e0df6897a7cacc77a022d2b032603256fa0b13e23033b80bd9843f`;
  sample SHA-256:
  `767c3a18e8a94f743ad3589f5ae830f5c0d633c15381208ad7aedcffb7761be7`.
- Rejected measurement: the preceding 400-retrace sampling wrapper attached
  to `/usr/bin/time`, not BlueWake. Its sample SHA-256
  `cde62795bfcd9a271be7f733726f40cca659cb01cb2f23a152aaa62f99edf468`
  is invalid and must not guide optimization.

## 2026-08-28 - Authentic Aurora CI palette hash span

- Route: strictly signed macOS app, canonical tagged composite, donor DSP IROM
  and COEF, private `GZLE01` disc, Aurora/gxcore, 500 retraces.
- Host: Apple M2 arm64 macOS; no Simulator; one native process.
- Baseline: promoted DSP batch-eight control spans approximately `55s` by
  evidence-file timestamps. The earlier `47.68s` result was produced by the
  rejected batch-64 candidate and is not a valid batch-eight baseline.
- Profile: `3,130/8,010` main-thread samples in
  `XXH3_hashLong_64b_withSeed` below `submit_draw_plan` texture resolution.
- Cause: TLUT resolution exposes bytes available through the end of the mapped
  guest range; the cache hashed all available bytes instead of the declared
  palette span (`tlut_entries * 2`).
- Result: `28.35s` wall, `27.11s` user, `1.10s` system for 500 retraces
  (8.33 seconds guest time), approximately 48% below the true batch-eight
  control and about 3.4 times slower than guest time.
- Correctness: exact title retrace 333; first nonzero donor DMA retrace 354/hash
  `0x403BBFA8`; 22,340 DMAs; 70 DVD forwards/zero failures; nonzero host PCM;
  106,434 GX submissions/zero rejects or failures; normal stop after 49,036,749
  translated blocks.
- Verification: 18/18 runnable CTests, six serial native Metal fixtures, full
  app build, and strict signing pass.
- Evidence SHA-256:
  `ea0edc4027b002f2058d75e0a21a34e976ec490786efb54a1f49a58c83942b57`.
- Next measurement: one fresh post-fix sample to select the next owner; do not
  rerun the same 500-retrace acceptance experiment.

## 2026-08-28 - Donor DSP interpreter batching

- Route: canonical tagged donor route, 1,200 retraces headless and 500 retraces
  in the signed Aurora app.
- Baseline: `139.92s` headless for 20 seconds guest time; 4,218/7,776 samples
  below `host_dsp_advance_schedule` with one-cycle interpreter calls.
- Rejected candidate: batch 64 reaches `103.93s` but changes DMA count from
  80,513 to 77,353 through Dolphin idle skipping.
- Promoted result: batch 8 preserves all 80,513 DMAs, clock/final PC, title and
  first-nonzero-DMA retraces, and 192,341,591 blocks; headless time is `123.96s`
  (11.4% improvement).
- Correct signed batch-eight control: approximately `55s`, SHA-256
  `b88047c3f821710483bf8c83b776e16ba49a1198a6161dda2891403fb6622c0c`.
- Headless evidence SHA-256:
  `d69631ec1506a3e87b563587b4e4d5b6c4e5306dc09b6e3f4502dec3262aadc5`.

## 2026-08-25 - P4 authentic A96 dispatch smoke

- Route: authentic macOS A96 pulse at retrace 96; private `GZLE01` disc;
  generated DOL and 415 REL composite.
- Host: Apple Silicon arm64 macOS; Release `-O3`; no Simulator; one host
  process.
- Bound: 2,000,000 translated blocks; normal stop at guest `pc=0x80307EF4`.
- Result: `34.40s` wall, `31.34s` user, `0.10s` system.
- Change under test: generator-owned one-entry last-chunk cache before the
  748-entry composite binary search in `dolrecomp_find_original`.
- Correctness: same normal idle boundary as the existing authentic baseline;
  no new exception, unmapped PC, or route workaround.
- Profile context: `/tmp/bluewake-startup-sample-20260825.txt` places the
  dominant samples in generated dispatch/guest functions; alias lookup is
  secondary. A pre-change wall-time control was not captured, so this entry is
  an absolute measurement, not a claimed percentage speedup.
- Remaining measurements: room-child completion, sustained 30 FPS, AOT versus
  fallback ratio, GPU/audio/memory/thermal/energy behavior.

All entries must be dated, name the deterministic representative route, device/platform/build, FPS/speed/frame distribution, AOT/fallback ratio, CPU subsystems, GPU, memory, I/O, thermals, energy, dominant measured cost, before/after comparison, and correctness digest preservation.
## 2026-08-31: Precision is limited to authentic deadline turns

The cap-1,024 route reduced host turns by 22.9% but crossed an authentic
deadline at a different basic-block boundary, producing one cycle of final
clock drift and a different AI work remainder. Removing fabricated zero-cycle
host time did not remove the earliest DSP interrupt-position divergence. That
result closes cap and guard tuning: the remaining experiment changes delivery
granularity, not the size of another window.

The candidate now keeps the inexpensive prepaid basic-block path whenever the
nearest deadline is farther than the configured cap. Only a turn actually
bounded by DSP, VI, AI, or decrementer distance uses per-instruction charging
and resumable checks. This is intended to preserve most of the measured turn
reduction without allowing a translated block to execute beyond the same
architectural instruction under different caps. It is not yet a performance
win: full regeneration and the three-retrace cap-invariance falsification are
required before timing claims or a 700-retrace run are admissible.

## 2026-09-01 - Partitioned generated-C O2 falsification

- Owner profile: a post-lifecycle 14,500-retrace visible Outset run matches the
  prior 1,050-record digest and places 5,814/5,864 active-main samples (99.1%)
  below `selected_dispatch`.
- Build: 1,024-instruction C partitions produce 2,125 composite chunks. `-O2`
  finishes in 3,943.29 seconds, versus 6,068.98 seconds for the promoted
  legacy-size `-O1` rebuild. The former whole-unit compiler stall is a source
  size/pathology problem, not proof that `-O2` is unusable.
- Speed: partitioned `-O2` completes the canonical 700 bound in 28.23 seconds
  wall / 23.53 user, versus 35.67 / 35.16 for the promoted control. Partitioned
  `-O1` takes 48.43 / 35.99, isolating a 34.6% user-CPU gain from `-O2` at the
  same generated shape.
- Correctness: both partitioned optimization levels produce digest
  `53fbb668...`, one extra final cycle, and delivery hash `1D2D463423047692`,
  rather than accepted digest `f4ca438d...` and hash `4736B7EA6A1D0B24`.
  First divergence is external-delivery ordinal 244 at the same cycle but a
  different saved PC. Totals, milestones, and cards remain coherent.
- Rejected variant: sequential-fallthrough coalescing removes only 0.5% of
  turns, keeps the divergent digest, and regresses wall time to 70.13 seconds.
- Decision: retain partition and optimization build controls; reject shortened
  partitions as a runtime shape. Next test uses legacy logical boundaries with
  `-O2` and bounded per-source `-O1` fallback. No long Outset candidate is
  admissible until the exact 700 digest passes.

## 2026-09-01 - Exact legacy-boundary hybrid O2 promotion

- Build: source-provenance-correct cycle-contract C, 748 legacy chunks, `-O2`
  except three measured pathological sources at `-O1`. Build completes in
  15,378.82 seconds wall / 89,095.12 user with 3.42 GB maximum RSS. Signing and
  full 417-range/415-REL ABI coverage pass.
- Correctness: the 700 route matches all 1,050 digest records and all 1,046
  cycle invariants, including 62,038,491 turns and the canonical card. The
  Outset run exactly matches digest `2b7a1fa5...`, 820,034,526 turns, and all
  subsystem/card outcomes.
- 700 speed: 31.91 wall / 26.87 user versus 35.67 / 35.16.
- Outset speed: 356.06 wall / 352.29 user versus the deterministic
  post-lifecycle control's 540.14 / 512.38. Improvement is 34.1% wall and
  31.2% user (`1.52x` wall), short of the declared `1.7x` target.
- Decision: promote the exact hybrid policy; keep playability open. One native
  profile of this new baseline selects the next owner. Optimization-level,
  fallback-list, and runtime-partition sweeps are closed.
- Invalid precursor: a stale `generated/full` build lacked the promoted
  `mtmsr`/cycle-precision contract. Its timing and divergent digest are excluded.

## Native startup audio sizing — 2026-09-09

A strict original initializer measurement consumes 2,172,424 bytes. The console
0x166800 allocation fails in native bank construction. The private scheduler
fixture uses 0x240000, then original `adjustSize` retains 2,168,352 bytes in its
final replay. Zelda allocation follows original remaining-system-minus-64-KiB
partitioning; total MEM1 remains 24 MiB. These are diagnostic memory measurements,
not an accepted boot budget, speed result or audio-device measurement. See
[the stopping-point handoff](SESSION_HANDOFF_2026-09-09.md).
