# PRD — Native GroundCrossGrpRp reimplementation

Status: Draft

Source: upstream `main` at `2ab8118fc09fa08569100201a871616d18a1b771`

Scope: one guarded serial native implementation of guest routine
`cBgW::GroundCrossGrpRp` (`0x802491F4`); excluded: parallel collision
traversal, other hotspots, and compiling the decompilation into BlueWake.

Parent workstream: runtime milestone M1 in [PR
#421](https://github.com/chrissotraidis/bluewake/pull/421). This document
decomposes that milestone into implementation and verification gates; it does
not define a separate candidate.

## Motivation

Steam Deck gameplay remains game-thread bound below the 30 FPS game target in
heavy outdoor scenes ([#215](https://github.com/chrissotraidis/bluewake/issues/215)).
The current profiling plan identifies the translated chunk containing
`GroundCrossGrpRp` as the leading bounded game-thread candidate. This work is
worth retaining only when the native path is equivalent to the translation,
activates in the measured scene, and improves the matched Deck control beyond
run-to-run noise. Reaching 30 FPS is the broader goal, not this candidate's
individual acceptance threshold.

Until it merges, [PR #421's technical
plan](https://github.com/chrissotraidis/bluewake/pull/421/changes#diff-dd62d1af6e31824bdcd61d4b21e687cedc87ae9cfcb19111531762f73e3b7bf9)
is the temporary source for the candidate profile and control procedure.

## Requirements

### FR-01 — Equivalent native execution

Architecture: [native game module](../../architecture/GameModule.md) and
[guest machine state](../../architecture/GuestState.md).

- For supported inputs, the native routine must match the translated path's
  CPU state, writable memory, return value, callbacks, traversal order, and
  cycle/deadline behavior.
- The decompilation may explain the algorithm but is not the execution oracle.

### FR-02 — Safe translated fallback

- Unsupported memory, aliases, callbacks, host boundaries, exceptions, or
  timing state must return to the translated path without any mutation.
- The existing global `BLUEWAKE_NATIVE_ENTRIES=0` opt-out must remain valid.

### FR-03 — Certified and observable activation

- Build-time insertion must be limited to the exact translated fragments
  covered by the differential oracle and rejected when their hashes change.
- Runtime reporting must distinguish accepted and declined calls so a fallback-
  only result cannot pass qualification.

### FR-04 — Layered qualification

Architecture: [test evidence levels](../../architecture/Testing.md#evidence-levels).

- Public tests must cover guards, unchanged decline, compilation, and hook
  contracts without game data.
- A private generated-module oracle must compare native and translated cases
  byte-for-byte.
- A matched Steam Deck A/B must preserve checkpoints and gameplay and show an
  improvement beyond measured same-build noise.

## Success and constraints

| Metric | Baseline | Target | Validation |
| --- | --- | --- | --- |
| Guest equivalence | Translation at `0x802491F4` | No CPU or writable-RAM mismatch in accepted oracle cases | Private differential suite |
| Fallback integrity | Existing translated path | Every decline leaves CPU and RAM unchanged | Public guards and private oracle |
| Activation | Entry is not currently registered | Positive accepted-call count in the target Deck scene | Native report and session evidence |
| Deck game-thread cost | PR #421 supplies provisional evidence and the procedure; current-source A/A is not yet measured | Candidate improves its matched control beyond measured noise | Interleaved A/B and re-profile |

Personal modules, disc data, saves, states, and raw logs remain private. The
change stays opt-in until affected-platform correctness and performance gates
pass.

## Clarifications

- The requested outcome is one serial native routine with translated fallback;
  parallelizing the recursive traversal requires a separate equivalence plan.
- PR #421's runtime M1 is the parent milestone. It is referenced rather than
  copied while the broader plan is unmerged.
- [PR #435](https://github.com/chrissotraidis/bluewake/pull/435) is the temporary
  architecture reference for how translated, native, and decomp routines relate.
