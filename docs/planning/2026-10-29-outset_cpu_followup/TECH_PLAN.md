# TECH_PLAN — Outset game-thread follow-up

Status: Retained stack: +1.8% Game Mode, +1.6% rendered (Outset +1.9%), checkpoints identical  
Source: BlueWake `a7731853c197cc9763a86d62e731d65ca0b25556`; control build = [#504](https://github.com/chrissotraidis/bluewake/pull/504) + `main` 0.10.0 (local `f4bda45`)  
[PRD](PRD.md) · [Tracker](PROGRESS.md)

## Where the time goes

The [game-thread architecture](../../architecture/GameThread.md#performance-model) runs translated game code and the host's boundary work on one thread, so an idle GPU does not help. Measured in Outset, in M cycles per game frame (≈ 129 M total; 30 FPS allows ≈ 113 M):

| Part | M/frame | What it is |
|---|---:|---|
| Translated game code | ≈ 85 | Collision 22, animation/skinning 14, draw setup 9, actors and the rest; flat, nothing above 2.3 |
| Runtime and host | ≈ 44 | The per-call and helper costs every scene pays (table below) |

The runtime's share, with what the code and prior PRs say can still be removed:

| Cost | M/frame | Removable | Basis |
|---|---:|---|---|
| Native routines (`*_try`) | 10.8 | Low | Their own work; the routing check is 0.33% |
| **Direct-call check** (`bw_direct_call_ready`) | **5.6** | **≈ 3** | **Ceiling test below** |
| GX command stream | 5.1 | Unknown | Host-side parsing; [#495](https://github.com/chrissotraidis/bluewake/pull/495) already took the alias test out |
| Dispatch loop | 4.3 | Unknown | [#448](https://github.com/chrissotraidis/bluewake/pull/448): the dispatch cache is already exact |
| Native actor search | 2.9 | ≈ 1 | A full memory lookup per 4-byte read |
| Memory slow paths, aliases | 2.2 | Unknown | — |
| Paired-single load/store | 2.1 | ≈ 1 | Generic quantization path for the plain-float case |
| FP helpers | 1.2 | Small | — |

## Why the direct-call check first

It runs before every call from one translated chunk into another. It has 32,604 call sites in the 84 hottest chunks, each paying ≈ 35 instructions (≈ 17 loads, 12 branches) in a 252-byte function the compiler keeps out of line.

**What earlier work rules out:**

| Idea | Result | Source |
|---|---|---|
| Move the host's question into the module | No gain: "the cost is the question itself" | [`SPEED_PROFILE_2026-10-11.md`](https://github.com/chrissotraidis/bluewake/blob/main/docs/status/SPEED_PROFILE_2026-10-11.md#s2-the-hosts-per-call-checks-read-inline-october-12-no-gain) |
| Force small helpers inline | 11.5% larger module, 1–2% slower | Same document, FP helpers |
| Read the host's answer inline (address-only) | +3.9% at Outset on a Mac; now the default v3 path | [#274](https://github.com/chrissotraidis/bluewake/pull/274) |
| Hook a game function natively (collision) | Accepted path slower than translation on the Deck | [#436](https://github.com/chrissotraidis/bluewake/pull/436) |

So what remains is to make the question itself smaller, without moving it or inlining it.

**What a smaller question is worth.** A diagnostic-only ceiling module kept only depth, exception, budget and one host flag. It had the same optimization profile as the control:

| Test | Control | Ceiling | Change | Spread |
|---|---:|---:|---:|---:|
| Game Mode, worst-angle Outset state, 3 × 15 s per launch, control → ceiling → control | 26.4 FPS | 27.2 FPS | **+3.1%** | 0.5 |
| render_bench, five states, 3 pairs, scale 3, unpaced | — | — | **+4.5%** (Outset +4.3%, Lava Room +6.8%, Hyrule Castle +2.4%, Butterfly Room +2.7%, Shipwreck +5.5%) | — |

The ceiling clears the 2% bar. It also shows what must not be cut: its guest checkpoints drifted from the control in every scene, because skipping the interrupt test changes when interrupts are taken. **The shippable check keeps every runtime test and removes only work whose answer is already known.**

## Results

Every A/B ran in Game Mode from the worst-angle Outset state, launched through the Steam shortcut, in the order control → candidate → control, with 3 × 15 s windows per launch. All builds shared the control's composite profile (`--reuse`).

| Build | What changed | Game FPS | Change | Spread | Lesson |
|---|---|---:|---:|---:|---|
| Ceiling (not shippable) | Check reduced to depth, exception, budget, one flag | 26.4 → 27.2 | +3.1% | 0.5 | The bound; skipping interrupt tests drifts guest state |
| v1 | Range tests settled at build time | 26.6 → 25.8 | **−2.9%** | 0.5 | The plain check was inlined at ≈ 448 sites per chunk (+11% hot code): layout, not logic |
| v2 | v1, both checks out of line and `hot` | 26.5 → 26.7 | +0.7% | 0.6 | Exact, but too small alone |
| **Stack (retained)** | **v2 + actor-search lookup inlined** | **26.1 → 26.6** | **+1.8%** | 0.4 | Two exact shared-cost cuts add up |

Rendered, five states, 3 pairs (unpaced): the stack is **+1.6% overall** (Outset +1.9%, Lava Room +3.0%, Hyrule Castle +1.8%, Butterfly Room +0.5%, Shipwreck +0.7%), with **guest checkpoints identical to the control in every run and scene**.

Considered and dropped: the paired-single plain-float path already exists; only call overhead remains (≈ 0.2 M cycles per frame), and inlining such helpers was slower in October.

## Approach

Of the ≈ 17 loads per call, two groups are known before the game runs:

1. **The two watched ranges** (particle and wake draws) are fixed guest addresses, and a translated call's target is a literal in the generated code. The compiler can settle "is this target in a range?" per call site.
2. **The enabled / callback / cpu tests** are implied whenever the host's inline-answer pointer is set, if every path that disables direct calls also clears that pointer.

The interrupt sources, depth, exception, budget and the two single watched addresses change at runtime and stay.

```mermaid
flowchart TD
    Call["Translated call<br/>to another chunk"]
    Wrap{"Target a constant<br/>outside both ranges?<br/>(compile time)"}
    Plain["Plain check<br/>depth · exception · budget<br/>interrupts · 2 single addresses"]
    Full["Full check (unchanged)<br/>+ ranges, enabled, host query"]
    Go["Call directly"]
    Loop["Back to the dispatch loop"]

    Call --> Wrap
    Wrap -->|yes, nearly all sites| Plain
    Wrap -->|no| Full
    Plain -->|inline answer off or verify on| Full
    Plain --> Go
    Full --> Go
    Plain -.->|a test fails| Loop
    Full -.->|a test fails| Loop
```

## Changes

| File / symbol | Change | Why |
|---|---|---|
| [`cmake/composite/direct_calls.h`](../../../cmake/composite/direct_calls.h) `bw_direct_call_ready` → `_plain` / `_full` | Always-inline wrapper. A constant target outside both ranges takes the plain check; anything else takes the original check, renamed `_full` | Removes the range comparisons and three implied tests from nearly every call, while every runtime test stays (the ceiling's drift shows they matter) |
| [`cmake/composite/direct_skip.h`](../../../cmake/composite/direct_skip.h) `BW_SKIP_RANGE*` | The two ranges as constants shared by host and module | The compile-time decision needs them in the module |
| [`runtime/host/src/main.c`](../../../runtime/host/src/main.c) | `_Static_assert` that those constants equal `draw_tags.h`'s | A future edit to one list cannot silently desynchronize the other |
| [`cmake/composite/direct_calls.c`](../../../cmake/composite/direct_calls.c) | Every path that disables direct calls also clears `bw_host_skip_fast` | Makes "pointer set" a valid stand-in for the dropped enabled/callback tests |
| [`tests/direct_calls_test.c`](../../../tests/direct_calls_test.c) | v3 cases on the shared ranges; plain vs full compared for six addresses in all 256 host/CPU states | Proves the two checks give the same answer, not just that each compiles |
| [`cmake/composite/direct_calls.h`](../../../cmake/composite/direct_calls.h) `noinline, hot` on both checks | Keep both out of line in the hot section, as the single check was | v1 inlined the plain check everywhere and ran 2.9% slower; the reused profile does not know the new names |
| [`runtime/host/src/main.c`](../../../runtime/host/src/main.c) `bw_search_word` | Always inline (seven call sites, one function) | Every sampled lookup took the plain-RAM path; ≈ 60% of its samples were the call and three saved registers |
| [`linux/CMakeLists.txt`](../../../linux/CMakeLists.txt), [`linux-host.yml`](../../../.github/workflows/linux-host.yml) | Run `bluewake_direct_calls_test` in the Linux host job (it already ran on Windows) | Release build, so the constant addresses take the plain path, as in the game module |

No handshake change. [#524](https://github.com/chrissotraidis/bluewake/pull/524) touches the same files for the stage select; whichever merges second rebases.

## Contract

```c
/* direct_calls.h: the plain check must equal the full one wherever it is used. */
static inline __attribute__((always_inline)) bool bw_direct_call_ready(const CPUState* cpu, u32 address) {
    if (__builtin_constant_p(address) && bw_skip_range_outside(address))
        return bw_direct_call_ready_plain(cpu, address);
    return bw_direct_call_ready_full(cpu, address);
}
```

`BLUEWAKE_DIRECT_SKIP_VERIFY=1` still routes every call through the full check and the host's answer, and counts disagreements.

## Verification

| Layer | Method | Required evidence | Status |
|---|---|---|---|
| Unit | `direct_calls_test`, -O0 and -O2/Release, Linux and Windows CI | Plain = full in every state | Passing locally and in CI |
| Exactness | render_bench, five states | Guest checkpoints identical to the control in every scene | Passed: identical in all 14 runs × 5 scenes |
| Safety | `BLUEWAKE_DIRECT_SKIP_VERIFY=1` run | Zero unsafe answers | Pending |
| Performance | Game Mode A/B/A (Outset), rendered A/B/B/A ≥ 5 pairs | Outset gain above 2% and the A/A spread | Game Mode +1.8% (beyond spread); rendered +1.6% overall, Outset +1.9% (3 pairs) |
| Regression | Five-scene tour, Outset walk | No scene beyond noise; no player-visible issue | Pending |

## Next steps

1. **Safety:** one `BLUEWAKE_DIRECT_SKIP_VERIFY=1` run with zero unsafe answers (exactness already shown by identical checkpoints).
2. **Player check:** an Outset walk on the Deck with the stack.
3. **Follow-up, separate change:** the interrupt test. A host-maintained "interrupt may be pending" word would replace three of its reads. It needs every write site kept in sync (≈ 17 in `main.c`, 5 in RecompCore's `interrupts.c`), a RecompCore companion PR and a handshake bump coordinated with [#524](https://github.com/chrissotraidis/bluewake/pull/524). Estimated ≈ +1%.
4. **Later:** count the dispatch-loop edges and look at the GX command stream.

## Implementation order

| Milestone | Depends | Deliverable |
|---|---|---|
| M0 | — | Control build, fixed Outset state, A/A spread |
| M1 | M0 | Costs attributed to game functions and runtime parts; one candidate selected |
| M2 | M1 | The candidate, its focused test and CI |
| M3 | M2 | Exactness, Game Mode and rendered A/B, five-scene regression, retain or reject |

Candidates stay separate A/B variables unless deliberately stacked (step 4).

## Gaps

- `scripts/render_bench` and `gamemode_measure.py` live in open [#492](https://github.com/chrissotraidis/bluewake/pull/492) and [#522](https://github.com/chrissotraidis/bluewake/pull/522).
- The Outset state and translated module are private hardware inputs and cannot enter CI.
- M0-T03 (five A/A pairs) is still open; the Game Mode control-twice runs give a 0.5 FPS spread meanwhile.

## References

- [#501 live follow-up](https://github.com/chrissotraidis/bluewake/pull/501) · [Steam Deck issue #215](https://github.com/chrissotraidis/bluewake/issues/215)
- [Profiling guide](../../PROFILING.md) · [Performance overview](../../performance/Overview.md)
- [Game thread: translated dispatch](../../architecture/GameThread.md#translated-dispatch)
