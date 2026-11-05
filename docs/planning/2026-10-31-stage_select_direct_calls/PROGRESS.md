# PROGRESS — Stage-select direct-call cost

Status: Deck-qualified through iteration 4: stage select's cost −10.0 % → −3.6 % (same build, on vs off); off unchanged with the same profile. Iteration 5 (`b7079abce`, slot filter) and the stage-select-on profile check running. Open: rendered warp tour, CI on the latest head
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
- [x] M3-T03: Run on/off windows on the Deck with `render_bench` (2x2 and three iterations, below); checkpoints identical in every run. The rendered warp tour is still to run.
- [x] M3-T05: Iterate on the profile: keep what is ahead in every launch, revert the rest (below).
- [ ] M3-T06: Iteration 5 (`b7079abce`): one bit test before the hook compares.
- [ ] M3-T07: Stage-select-on profile check: control source with this PR's profile against this PR, both on.
- [ ] M3-T04: Record retain/reject findings and update #503 with the corrected benchmark interpretation.

## Evidence ledger

| Task | Commit / artifact | Command / method | Environment / input | Result |
|---|---|---|---|---|
| M0-T01 | [#503 discovery record](https://github.com/chrissotraidis/bluewake/pull/503), control build `f4bda45` | `scripts/gamemode_measure.py`; 30 s `perf record -F 2999` on the game thread | Steam Deck Game Mode, Outset, scale 3, Smooth Motion on; `BLUEWAKE_STAGE_SELECT_TOGGLE` on/off | 23.6 → 26.1 FPS (+10.9%); host skip 4.5%, observer 1.7%; clock unchanged |
| M0-T02 | [`stage_select.h`](../../../runtime/host/src/stage_select.h), [`test_direct_calls.py`](../../../tests/test_direct_calls.py) | Parse the observer and compare its fixed-address symbols with the host-supplied v4 watch set | Source contract | Five observer addresses exactly match five v4 watches |
| M0-T04, M1 | [`direct_skip.h`](../../../cmake/composite/direct_skip.h), [`direct_calls.c`](../../../cmake/composite/direct_calls.c), [`main.c`](../../../runtime/host/src/main.c) | Source inspection and focused implementation | `v4` extends `v3`; stage-select host with an old module uses `v2` | Existing ABI retained; invalid contracts decline safely |
| M2-T01 | [`direct_calls_test.c`](../../../tests/direct_calls_test.c) | Compile with `cc`; run `/tmp/bluewake-direct-calls-test` | Synthetic direct calls, normal and mirrored watches, v4→v3 fallback | PASS |
| M2-T02 | [`test_direct_calls.py`](../../../tests/test_direct_calls.py) | `python3 tests/test_direct_calls.py` | Seven source/transform contract tests | PASS (7/7) |
| M3-T03 | `f6bbf27a6` vs control `ee991d312`, stage select on/off | `render_bench` 2x2, 5 states, 3 pairs + warm-up, `--cpu-profile` | Steam Deck | On: −10.0 % → −6.35 % (fix +2.58 %); off: −1.42 %. Profile: stage select's remaining cost is in the module (58.0 % vs 56.5 %), not the host (7.14 % vs 7.12 %) |
| M3-T05 | `cde980cc3`, `8f7442ade` (host only) | Same, 6 launches per build | Steam Deck | +0.65 % but launches overlap: **no-go**, reverted (`b027a2ec3`, `041d50c8c`) |
| M3-T05 | `f9d926907` (hooks as module constants) | Same | Steam Deck | On: **+3.11 %**, every launch ahead; off: −2.69 % (`bw_direct_call_ready` is out of line, so every call paid the v4 loads) |
| M3-T05 | `c48e83f0e` (hooks in `simple`'s bits, no run-time list) | Same | Steam Deck | On: −0.01 % against `f9d926907` (keeps +3.1 %); off: −1.51 % against the control |
| M3-T05 | Control source built with this PR's optimization profile | Same, stage select off | Steam Deck | Control code with this profile: −1.75 % against the control; this PR's code with the same profile: +0.48 % (launches overlap). **The stage-select-off gap is training variance, not this PR** |

## Decisions

| Date | Decision |
| --- | --- |
| 2026-11-05 | Keep `f9d926907` + `c48e83f0e`: the five hooks are module constants carried in `BwHostSkipFast.simple`'s bits; no run-time watch list (a host needing an unknown watch turns the inline path off). Stage select on: −10.0 % → −3.6 % against the control. Off: no change |
| 2026-11-05 | Reverted, kept in history: `cde980cc3` (inline address filter for the per-edge calls, +0.65 % not consistent) and `8f7442ade` (v3 with stage select off, no effect) |
| 2026-11-05 | Compare stage-select-off builds only with the same optimization profile: two separate trainings differ by ≈ 1.75 % on the Deck |

