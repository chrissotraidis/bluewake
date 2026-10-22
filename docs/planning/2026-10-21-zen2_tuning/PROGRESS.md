# PROGRESS — Steam Deck Zen 2 tuning

Status: Planned; M5's headless evidence currently favors Clang 18, but its
rendered stage-completion parser and repeated Game Mode gate remain open. M6
still waits for the M5 decision, the `--mtune` implementation, and upstream
resolution of [#433](https://github.com/chrissotraidis/bluewake/issues/433).

[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md) ·
[Parent M6](https://github.com/chrissotraidis/bluewake/pull/421) ·
[M5 prerequisite](https://github.com/chrissotraidis/bluewake/pull/437)

## Milestones

### M0: Freeze the tuning contract

- [ ] M0-T01: Record the compiler retained by M5 and freeze source, dependencies, PGO input, options and Deck workload.
- [ ] M0-T02: Define the opt-in `--mtune znver2` interface while preserving `-march=x86-64-v3` and the default path.
- [ ] M0-T03: Define provenance, fingerprint and effective-command evidence for tuned and untuned artifacts.

### M1: Implement and build the option (depends: M0)

- [ ] M1-T01: Propagate the tune option to host and module compilation and record it in profile/artifact identity.
- [ ] M1-T02: Add source-contract tests and concise Linux build documentation.
- [ ] M1-T03: Produce separate untuned and Zen 2-tuned AppImages and run Linux, ABI, smoke and checkpoint checks.

### M2: Measure and decide (depends: M1)

- [ ] M2-T01: Establish untuned A/A noise, then run five interleaved Steam Deck A/B pairs.
- [ ] M2-T02: Capture matched Game Mode summaries and explain conflicting results if required.
- [ ] M2-T03: Retain the opt-in only for a correct gain beyond noise; keep the portable default unchanged without broader evidence.

## Evidence ledger

| Task | Commit / artifact | Command / method | Environment / input | Result |
|---|---|---|---|---|
| M0-T01 (preparation) | #446 `48a5872` private Clang 18/22 AppImages | Container build plus AppImage ABI/extraction smoke | Steam Deck-compatible `x86-64-v3` baseline | Build path works before merge; no retained M5 compiler or shared PGO profile yet |
| M0-T01 (M5 status) | [PR #437](https://github.com/chrissotraidis/bluewake/pull/437) | Fixed-state Deck recipe comparison | Five-scene headless grid, one preliminary rendered pair, and directional Game Mode samples | Headless favors Clang 18 by 3.2%; M5 has not retained a compiler, so M6 remains blocked. |
