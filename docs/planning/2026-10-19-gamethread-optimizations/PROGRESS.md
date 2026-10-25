# PROGRESS — Steam Deck game-thread optimization

Status: M1–M3 and M6 are completed negative results. M4 remains conditional on
a render-thread limit. The [#433](https://github.com/chrissotraidis/bluewake/issues/433)
regression claim was retracted; [#451](https://github.com/chrissotraidis/bluewake/issues/451)
blocks certified native-entry generation for a current control.

[Requirements](PRD.md) · [Technical plan](TECH_PLAN.md) ·
[Game-thread architecture](../../architecture/GameThread.md)

## Completed prerequisite evidence

- [x] Established the Deck environment and verified the profiled AppImage,
  module, state and running Game Mode process.
- [x] Collected repeatable headless, Desktop and Game Mode measurements with
  matching checkpoints and zero-loss CPU profiles.
- [x] Identified the game thread as limiting and mapped its leading translated
  range to `cBgW::GroundCrossGrpRp`.

## Milestones

M0 creates the shared current-source control. Runtime and build-time candidates
then remain separate: compare one change at a time, retain only a passing
candidate, and use that retained result as the next track-specific control.

**Shared control**

### M0: Establish the shared control (depends: candidate ranking)

- [ ] M0-T01: Build the current source with the default Linux recipe. (FR-01)
- [ ] M0-T02: Record inputs, flags, PGO, hashes, size and Deck environment. (FR-01)
- [ ] M0-T03: Repeat the target scene and establish current A/A noise. (FR-02)

**Runtime track**

### M1: Native `GroundCrossGrpRp` (depends: M0 decision) — [rejected and closed in PR #436](https://github.com/chrissotraidis/bluewake/pull/436)

- [x] M1-T01: Map translated memory, calls, cycles and boundaries. (FR-02)
- Not pursued: a full-traversal oracle is unnecessary after the partial hook failed its Deck performance gate.
- [x] M1-T03: Add the opt-in serial entry and activation counters. (FR-03)
- Not pursued: full-path and Windows qualification are unnecessary after the performance no-go.
- [x] M1-T05: Record the A/B decision: reject the partial hook; it is correct for supported paths but not faster than translation on Deck. (FR-04)

### M2: Guest alias resolution (depends: M1 decision) — [rejected in PR #447](https://github.com/chrissotraidis/bluewake/pull/447)

- [x] M2-T01: Capture the fixed-route Deck alias-cost census and CPU sample. (FR-02)
- [x] M2-T02: Compare the observed lookup work with the pinned resolver implementation. (FR-03)
- [x] M2-T03: Reject a new guard: the relevant non-overlapping-alias binary search already exists. (FR-04)

### M3: Native dispatch and numeric paths (depends: M2 decision) — [rejected in PR #448](https://github.com/chrissotraidis/bluewake/pull/448)

- [x] M3-T01: Capture dispatch, native activation, and pool telemetry on the fixed Deck route. (FR-02)
- [x] M3-T02: Reject a serial dispatch change: the sampled path is the existing exact PC-keyed cache. (FR-03)
- [x] M3-T03: Reject worker work: production `array_max=8` does not reach the 256-item grain. (FR-05)
- Not pursued: correctness and A/B gates require a selected code change.

### M4: Secondary graphics work (depends: M3 decision)

- [ ] M4-T01: Reconsider GX only if the render thread becomes limiting. (FR-02)
- [ ] M4-T02: Keep render-thread gains separate from game-thread claims. (FR-04)

**Build-time track**

This track may run alongside M1–M3 after M0. Each step starts from the last
retained build-time result, without incorporating unqualified runtime changes.

### M5: Compiler recipe (partial result in [PR #437](https://github.com/chrissotraidis/bluewake/pull/437); depends: M0)

- [x] M5-T01: Compare Clang 18 and 22 recipes, including their trained PGO profiles. (FR-06)
- [ ] M5-T02: Confirm loaded rendered scenes, then capture qualified rendered and Game Mode observations. (FR-06)
- [ ] M5-T03: Repair rendered stage confirmation, repeat controlled rendered/Game Mode comparisons, then retain or reject a recipe. (FR-06)

Results: [PR #437](https://github.com/chrissotraidis/bluewake/pull/437)
records five warmed interleaved fixed-state headless pairs on source #446
`48a5872`: Clang 18's recipe is 3.2% faster overall with matching checkpoints.
The rendered scenes are not accepted because loading was not confirmed. Repeat
the rendered and Game Mode stages after confirmation; neither is a
default-selection gate yet.

Remaining M5 order: confirm rendered loading; record rendered A/A noise; run
five interleaved rendered pairs; verify the local Game Mode route; record Game
Mode A/A noise and five interleaved pairs; then retain a recipe or record an
inconclusive result. The detailed procedure is in [PR #437](https://github.com/chrissotraidis/bluewake/pull/437).

| Run | What ran | Outcome | Decision use |
|---|---|---|---|
| Build and smoke | Clean Clang 18 and Clang 22 #446 builds; ABI and extraction/help smoke | Both private artifacts passed | Artifact validation only |
| Headless benchmark | Five scenes; 10 interleaved A/B/B/A samples per recipe | Clang 18 +3.2%; 31 checkpoints match | Accepted headless finding |
| Rendered Desktop attempt | One warmed five-scene tour | Completion parser reported scenes not loaded | Excluded from selection |
| Game Mode attempt | One 30-second profile per recipe | Directional profile, not repeated controlled comparison | Excluded from selection |

| Task | Commit / artifact | Command / method | Environment / input | Result |
|---|---|---|---|---|
| M5-T01 | [PR #437 findings](https://github.com/chrissotraidis/bluewake/pull/437); #446 `48a5872`; private Clang 18/22 AppImages | Fixed-state five-scene headless A/B/B/A pairs | Steam Deck on mains; compiler-specific PGO; 10 samples/recipe | All checkpoints match; Clang 18 recipe median +3.2% overall. |
| M5-T02 | — | Not accepted pending scene-load confirmation | — | Open; no rendered or Game Mode result is retained for recipe selection. |

### M6: Zen 2 tuning (decision recorded in [PR #438](https://github.com/chrissotraidis/bluewake/pull/438))

- [x] M6-T01: Measure module-only `-mtune=znver2` while retaining `-march=x86-64-v3`.
- [x] M6-T02: Reject the option and preserve the portable default.

| Task | Commit / artifact | Command / method | Environment / input | Result |
|---|---|---|---|---|
| M6-T01 | [PR #438](https://github.com/chrissotraidis/bluewake/pull/438#issuecomment-6357949110) | Four interleaved headless, unpaced rounds for matched builds from `56a9537` | Steam Deck; five fixed scenes; candidate adds module-only `-mtune=znver2` | Scene medians range from -0.7% to +0.4%; mean +0.0%. |
| M6-T02 | [PR #438 decision record](https://github.com/chrissotraidis/bluewake/pull/438) | Compare medians against observed variation | Same workload | Reject / park this flag; retain `x86-64-v3` and do not add a builder option. |

### M7: Certified native entries (depends: M6 decision, #451)

- [ ] M7-T01: Build `--native-entries` and verify hook certification. (FR-06)
- [ ] M7-T02: Require activation, differential checks and a Deck gain. (FR-06)

### M8: Deck-scene PGO weighting (depends: M7 decision)

- [ ] M8-T01: Build a separately weighted Fortress/Forest Haven profile. (FR-06)
- [ ] M8-T02: Compare target and out-of-tour scenes for overfitting. (FR-06)

**Integration**

### M9: Combine retained candidates (depends: M4, M8)

- [ ] M9-T01: Build only the independently retained runtime/build variants. (FR-04)
- [ ] M9-T02: Run final checkpoints, gameplay, profile and stable-30 gates. (FR-05)

## Evidence boundary

Private AppImages, states, raw logs, and `perf.data` remain on the Deck.
Experiment sources are deliberately separate: M1–M3 use the source-matched
[`3b5c5c4`](https://github.com/chrissotraidis/bluewake/commit/3b5c5c4826ef62bd1f8ff387734ff2ef295898e5)
control; M5 uses [#446 source `48a5872`](https://github.com/chrissotraidis/bluewake/pull/446);
and M6 uses `56a9537` as recorded in [PR #438](https://github.com/chrissotraidis/bluewake/pull/438).
None is a certified current native-entry control while #451 is unresolved.

## M1 decision record

The detailed implementation and evidence are in [PR #436 (closed)](https://github.com/chrissotraidis/bluewake/pull/436).
The guarded hook accepted 123,609 of 440,495 calls (28.1%); 99.46% of its
declines required unimplemented traversal. Its accepted early return measured
226.1 ns/call native versus 179.3 ns/call translated on Deck. Five interleaved
pairs were within noise, so M1 is rejected.

### M2 decision

The [M2 investigation PR #447](https://github.com/chrissotraidis/bluewake/pull/447)
captured two fixed-route Deck alias-cost reports and a 20-second 99 Hz profile.
The resolver accounted for 1.60% sampled self time, but successful lookups used
about 11 comparisons across roughly 1,900 aliases—the existing sorted-index
binary-search path. No distinct equivalence-safe guard was identified, so M2
is rejected without a RecompCore change. M3 is the next runtime investigation.

### M3 decision

[PR #448](https://github.com/chrissotraidis/bluewake/pull/448) captured a
60-second Deck profile with zero lost samples and normal-exit activation
telemetry. `selected_dispatch` maps to the existing generated exact PC-keyed
cache; native routing exposes no redundant guard; and the sole pool client
remained serial (`array_max=8`, `parallel=0`). M3 is rejected without a code
change.
