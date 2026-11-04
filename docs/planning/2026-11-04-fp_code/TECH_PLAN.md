# TECH_PLAN — Floating-point code in the translated game

Status: M1 implemented, opt-in and unit-tested; measurement next.
[Product brief](PRD.md) · [Tracker](PROGRESS.md)

## M1: one FP-available test per stretch of a prepaid copy

### Why

Every translated FP instruction starts with:

```c
if (!ppc_fp_available_inline(ctx, 0x...u)) return;
```

- **What it costs:** a `pc` store before it, a test of MSR[FP], and an exit edge. The exit stops Clang from keeping state in registers across it.
- **What it blocks:** `lean_memory.py` rewrites an access only when its instruction calls nothing else and never leaves, so the test disqualifies every FP load and store. Each one keeps its `pc` store, its cycle-suffix store and its deadline test.

In the prepared source of the #437 build:

| | Count |
|---|---:|
| Prepaid copies | 464,979 |
| Copies with FP tests | 72,463 (15.6%) |
| FP-available tests in them | 368,745 |
| FP loads and stores in them | 240,490 |
| Of those copies, ones that write MSR, an SPR or the FPSCR | 5 |

### Why the test can't go

The Dolphin OS switches FPU state lazily: `OSSetCurrentContext` clears MSR[FP] for every thread except the FPU's owner, and that thread's first FP instruction traps. So the FP-unavailable path really runs, at thread switches, and a failing test must behave exactly as today.

### Design

`scripts/windows/fp_checks.py` runs on the prepaid copies only, after `fast_blocks.py` and `direct_calls.py` and before `lean_memory.py`:
- **The first test of each stretch stays** at its place, with its `pc` store. A failure is unchanged.
- **Later tests in the stretch are removed.**
- **A stretch ends** at any call that could run guest code or change MSR, or at a label. The next FP instruction then keeps its test, as the first.

### Exactness

Within a prepaid copy, a removed test always passes, because MSR can't change between the stretch's first test and it:
- **The only MSR change in the FP path** is a failing `ppc_fp_available`, and the code returns right after.
- **FP operations never take an exception:** `set_fp_exception` only sets FPSCR bits (`cpu_interpreter_float.c`).
- **Memory accesses don't change MSR:** an exception an MMIO handler raises is taken only after the copy returns.
- **A copy is entered only at its start,** holds no label anything can jump to, and leaves for the original block at any refund. The original blocks keep every test, because they can be resumed mid-block after another thread has run and changed MSR.
- **Every other call ends the stretch:** direct calls, `bw_call_translated`, natives, extracted loops and `rfi`. The list of safe calls is explicit; a prefix match would admit `ppc_fallback_instruction`, which raises a program exception.

### Changes

| File | Change |
|---|---|
| [`scripts/windows/fp_checks.py`](../../../scripts/windows/fp_checks.py) | The pass |
| [`scripts/linux/build.py`](../../../scripts/linux/build.py) | `--fp-checks` (off by default, needs `--prepared-blocks`); the prepared-source inputs, the receipt, the training fingerprint, provenance |
| [`tests/test_fp_checks.py`](../../../tests/test_fp_checks.py) | First test kept and later ones dropped; every stretch-ending call; original blocks untouched; repeatable; unsafe names; end to end, a later `lfs` lean after `lean_memory.py` |
| [`.github/workflows/audit.yml`](../../../.github/workflows/audit.yml) | Runs the test |

### Verification

| Level | Check |
|---|---|
| Unit | `tests/test_fp_checks.py` in the audit job |
| Source | On the #437 prepared source: 296,188 of 368,745 tests removed in 811 chunks; the hottest J3D chunk compiles cleanly with the build's own command |
| Exactness | render_bench, five scenes: guest checkpoints identical to the control |
| Performance | Ahead of the control in every interleaved Game Mode launch (at least six); desktop `perf stat` as a first read |

### Risks

- **A call missing from the safe list** only ends a stretch early, which costs speed, not exactness. A call *wrongly* on the list could skip a needed test. The list is deliberately short: the FP operations, memory accesses and pure conversions.
- **Module size** changes little; the removed lines are small.

## Later steps

M2–M5 (see the [PRD](PRD.md#steps)) are planned after M1 is measured. Each gets its own section here.
