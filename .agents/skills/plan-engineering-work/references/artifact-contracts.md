# Artifact contracts

Use only relevant sections. Replace sample IDs/content; never leave empty templates in a deliverable.

Default directory: `docs/planning/YYYY-MM-DD-feature_name/`. Keep `PRD.md`, `TECH_PLAN.md`, and `PROGRESS.md` together; use the user's local creation date and a lowercase snake_case feature name.

## PRD.md

```markdown
# PRD — <Feature>

Status: Draft
Source: <revision / artifact identity>
Scope: <included>; excluded: <explicit limits>

## Motivation
<User-confirmed problem, affected users, desired outcome and why now>

## Outcomes
- <Observable result and acceptance>
- <Failure or unavailable-evidence behavior>

## Success and constraints
| Metric | Baseline | Target | Validation |
|---|---|---|---|

## Clarifications
- <Question, user answer and source; keep unanswered questions explicitly open>
```

Keep technical filenames/APIs in TECH_PLAN. Source provenance and architecture links are valid in PRD.

## TECH_PLAN.md

```markdown
# TECH_PLAN — <Feature>

Status: Planning only
Source: <revision>
[PRD](PRD.md) · [Tracker](PROGRESS.md)

## Inspected facts
- <Verified behavior and source link; distinguish measurements from hypotheses>

## Changes
| Status | File / symbol | Practical edit | Milestone | Architecture |
|---|---|---|---|---|
| Modify | <real path/symbol> | <specific change> | M1 | <verified flow> |

## Contracts and examples
<Small before/after snippets; explicit proposed APIs>
<Diagram only when useful>
<Failures, lifecycle, state ownership, compatibility>

## Verification
| Layer | Input / command | Required evidence | Failure / skip |
|---|---|---|---|

## Implementation order
| Milestone | Depends | Can run with | Deliverable / integration gate |
|---|---|---|---|
| M0 | — | — | <shared contract and baseline> |
| M1 | M0 | M2 | <implementation stream, owned files and contract> |
| M2 | M0 | M1 | <independent fixture/platform stream, owned files and contract> |
| M3 | M1, M2 | — | <integrate and verify both streams> |

Critical path: <longest dependency route, estimates if supported>.
Concurrency limits: <shared files/resources; why any serial work is necessary>.

## Decisions / gaps
- <Decision, alternatives only when meaningful, unresolved evidence>

## References
- <Source with revision and role>
```

Keep performance and reliability targets in PRD success criteria and the
verification matrix. Do not promise speedups when building test infrastructure;
use qualitative acceptance when no defensible numerical target exists.

## PROGRESS.md

```markdown
# PROGRESS — <Feature>

Status: Planned
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md)

## Milestones
### M0: <Name>
Architecture: [affected flow](<verified reference>).
- [ ] M0-T01: <Concrete action>
- [ ] M0-T02: <Meaningful verification>
- [ ] M0-T03: <Evidence/report action>

### M1: <Name> (depends: M0)
Parallel with M2; owns <files>; integrates at M3 under <shared contract>.
- [ ] M1-T01: <Action>
- [ ] M1-T02: <Action>
- [ ] M1-T03: <Verification>

### M2: <Independent stream> (depends: M0)
Parallel with M1; owns <different files>; integrates at M3.
- [ ] M2-T01: <Action>
- [ ] M2-T02: <Action>
- [ ] M2-T03: <Verification>

### M3: <Integration> (depends: M1, M2)
- [ ] M3-T01: <Integrate streams>
- [ ] M3-T02: <Run combined acceptance>
- [ ] M3-T03: <Record evidence>

## Evidence ledger
| Task | Commit / artifact | Command / method | Environment / input | Result |
|---|---|---|---|---|
```

Task states: `[ ]` open, `[x]` completed with evidence, `[!]` blocked with reason/dependency. A completed task needs a populated evidence-ledger row with commit/artifact, command/method and result. Inline evidence uses `evidence: artifact=<identity>; method=<command or method>; result=<outcome>`. Optional evidence can be unavailable; required gates remain incomplete. Report tasks must not say benchmarks passed without actual logs.

## Output integrity

| Check | Agent responsibility |
|---|---|
| Source/API exists | Inspect exact code/revision |
| Proposed snippet | Label; do not claim compiled/executed |
| Architecture link | Read target heading and confirm relevance |
| Complete acceptance | Compare all requested outcomes after edits |
| Actual impact | Require recorded method/input/result |
| Motivation / ambiguity | Ask; preserve user answers and unresolved questions |
| Concurrent implementation | Match dependencies, file ownership, shared contracts and integration gates across plan/tracker |
| No architecture docs | Cite implementation and record gap |
| Generated local files | Save; do not imply remote commit |
| Updating existing trio | Preserve identity, task states, IDs and measured evidence |
