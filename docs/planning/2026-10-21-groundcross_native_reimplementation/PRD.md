# PRD — Native GroundCrossGrpRp reimplementation

Status: Evaluated — rejected as a performance optimization

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
`GroundCrossGrpRp` was evaluated as a bounded native candidate. Such work is
worth retaining only when the native path is equivalent to the translation,
activates in the measured scene, and improves the matched Deck control beyond
run-to-run noise. This candidate did not meet that final condition. Reaching
30 FPS is the broader goal, not an individual candidate's acceptance threshold.

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
| Activation | Entry was not registered | 123,609 native calls out of 440,495 (28.1%) in the Deck capture | Native report and session evidence |
| Deck game-thread cost | Matched control | Faster than control beyond noise | **Not met:** five interleaved pairs were within noise; accepted native path was slower in direct Deck timing |

Personal modules, disc data, saves, states, and raw logs remain private. The
partial implementation is not to be retained as a performance optimization.
Its evidence is retained to rule out this narrow path and guide future
candidate selection.

## Clarifications

- The requested outcome is one serial native routine with translated fallback;
  parallelizing the recursive traversal requires a separate equivalence plan.
- PR #421's runtime M1 is the parent milestone. It is referenced rather than
  copied while the broader plan is unmerged.
- [PR #435](https://github.com/chrissotraidis/bluewake/pull/435) is the temporary
  architecture reference for how translated, native, and decomp routines relate.
