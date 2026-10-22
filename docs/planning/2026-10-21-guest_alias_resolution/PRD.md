# PRD — Guest alias-resolution evaluation

Status: Draft

Source: BlueWake `main` at `56a9537`; pinned RecompCore
`338a3dbcdd384142f0fe40db44e0cd1d1b0c61e4`.

Scope: measure and, only if justified, evaluate one guarded
`ppc_guest_alias_resolve` fast path. Excluded: changing guest-memory semantics,
parallel execution, or retaining a change without a Deck win.

Parent: [PR #421](https://github.com/chrissotraidis/bluewake/pull/421), runtime
M2. Its M1 GroundCross experiment is [closed](https://github.com/chrissotraidis/bluewake/pull/436): correct for supported paths, but not a Deck optimization.

## Motivation

Steam Deck gameplay is game-thread limited in the outdoor route tracked by
[#215](https://github.com/chrissotraidis/bluewake/issues/215). The current
profile samples `ppc_guest_alias_resolve`; the user-confirmed goal is to find
out whether repeated alias resolution is worth optimizing before changing it.

## Outcomes

- Record resolver-call distribution and Deck profile evidence for the same
  state and settings.
- Reject the candidate if bounds pruning or existing fast paths make resolver
  work insignificant.
- If justified, test one RecompCore guard that preserves alias precedence,
  bounds checks, returned pointer, and journal offset.
- Retain it only when focused tests, matching checkpoints, gameplay, and
  matched Deck A/B runs show a repeatable gain beyond noise.

## Success and constraints

| Metric | Baseline | Target | Validation |
| --- | --- | --- | --- |
| Resolver evidence | Existing AppImage | Nonempty trace with calls, pruning, iterations, hits, alias count, and MEM1-overlap state | `BLUEWAKE_TRACE_ALIAS_COST=1` normal exit |
| Resolver correctness | Existing lookup result | Identical result for all RecompCore resolver tests and any dual-path check | RecompCore tests |
| Player behavior | Matched control | Matching checkpoints and no route regression | Deck A/B and gameplay route |
| Performance | Matched control | Improvement beyond observed noise | Interleaved Deck pairs and profile |

## Clarifications

- The existing direct MEM1 fast path remains the baseline; this plan does not
  assume a cache or a specific optimization before measurement.
- Runtime code belongs in a separate RecompCore `bluewake-next` PR. BlueWake
  may only pin the accepted RecompCore revision afterward.
