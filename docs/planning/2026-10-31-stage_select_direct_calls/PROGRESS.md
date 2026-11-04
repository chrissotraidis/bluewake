# PROGRESS — Stage-select direct-call cost

Status: Source and focused tests complete; build and Deck qualification pending
[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md)

## Milestones

### M0: Freeze the host-observation contract

Architecture: [translated dispatch](../../architecture/GameThread.md#translated-dispatch).

- [x] M0-T01: Record the discovery measurement, profile shares, build identity and affected stage-select/warp modes.
- [x] M0-T02: Enumerate every stage-select observed address and prove how the generated and runtime watch paths cover it.
- [ ] M0-T03: Run current `v3` verification with stage selection and warp routes; record unsafe/conservative counts and A/A spread.
- [x] M0-T04: Select and document the versioned fast-path representation and older-module fallback.

### M1: Keep the inline answer safely (depends: M0)

Parallel with M2; owns `main.c` and the direct-call ABI files; integrates at M3.

- [x] M1-T01: Add the versioned module handshake without changing `v2` or `v3` layouts.
- [x] M1-T02: Supply the complete stage-select watch contract and enable the inline answer when supported.
- [x] M1-T03: Preserve conservative fallback for old or incomplete modules and diagnostic modes.

### M2: Lock coverage and measurement guidance (depends: M0)

Parallel with M1; owns tests and documentation; integrates at M3.

- [x] M2-T01: Add component cases for watched addresses, mirrors, malformed contracts and compatibility fallback.
- [x] M2-T02: Add source-contract coverage preventing a new stage-select hook from bypassing the watch contract.
- [ ] M2-T03: Document stage-select/warp state in benchmark evidence, using #522 after it lands.

### M3: Integrate and qualify (depends: M1, M2)

- [ ] M3-T01: Run focused tests, Linux/Windows CI, diff and repository audits.
- [ ] M3-T02: Exercise menu entry/return, warp arrival/music and file selection on the candidate.
- [ ] M3-T03: Run Game Mode A/A and on/off windows plus the rendered warp tour; require matching checkpoints.
- [ ] M3-T04: Record retain/reject findings and update #503 with the corrected benchmark interpretation.

## Evidence ledger

| Task | Commit / artifact | Command / method | Environment / input | Result |
|---|---|---|---|---|
| M0-T01 | [#503 discovery record](https://github.com/chrissotraidis/bluewake/pull/503), control build `f4bda45` | `scripts/gamemode_measure.py`; 30 s `perf record -F 2999` on the game thread | Steam Deck Game Mode, Outset, scale 3, Smooth Motion on; `BLUEWAKE_STAGE_SELECT_TOGGLE` on/off | 23.6 → 26.1 FPS (+10.9%); host skip 4.5%, observer 1.7%; clock unchanged |
| M0-T02 | [`stage_select.h`](../../../runtime/host/src/stage_select.h), [`test_direct_calls.py`](../../../tests/test_direct_calls.py) | Parse the observer and compare its fixed-address symbols with the host-supplied v4 watch set | Source contract | Five observer addresses exactly match five v4 watches |
| M0-T04, M1 | [`direct_skip.h`](../../../cmake/composite/direct_skip.h), [`direct_calls.c`](../../../cmake/composite/direct_calls.c), [`main.c`](../../../runtime/host/src/main.c) | Source inspection and focused implementation | `v4` extends `v3`; stage-select host with an old module uses `v2` | Existing ABI retained; invalid contracts decline safely |
| M2-T01 | [`direct_calls_test.c`](../../../tests/direct_calls_test.c) | Compile with `cc`; run `/tmp/bluewake-direct-calls-test` | Synthetic direct calls, normal and mirrored watches, v4→v3 fallback | PASS |
| M2-T02 | [`test_direct_calls.py`](../../../tests/test_direct_calls.py) | `python3 tests/test_direct_calls.py` | Seven source/transform contract tests | PASS (7/7) |

