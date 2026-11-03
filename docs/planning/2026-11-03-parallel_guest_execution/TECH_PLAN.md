# TECH_PLAN — Parallel guest execution

Status: Planning only
Source: BlueWake `main` at `ee991d3`; RecompCore `0fe472a`; tww `9cbb155`
[PRD](PRD.md) · [Tracker](PROGRESS.md)

## Inspected facts

- **Fixed CPU and RAM.** Generated chunks carry `extern CPUState bw_guest_cpu; #define ctx (&bw_guest_cpu)` and take `CPUState* ctx_param` unused. ABI v4 requires `bluewake_composite_guest_cpu()`, v5 the MEM1 getter ([`module_cpu_contract.h`](../../../cmake/composite/module_cpu_contract.h)). With `BW_GUEST_MEM1`, `BW_RAM_BYTES` is the linked symbol, not `cpu->ram` ([`gather_pipe.h`](../../../cmake/composite/gather_pipe.h)).
- **Globals on the translated path.** `bw_direct_depth`, `bw_direct_enabled`, `bw_chunk_fns`, host flag pointers in [`direct_calls.h`](../../../cmake/composite/direct_calls.h); gather-pipe drains at hardware accesses and chunk returns ([`dispatch_loop.h`](../../../cmake/composite/dispatch_loop.h)); RecompCore `g_mem_write_journal` (post-store callback), `g_ppc_guest_aliases_overlap_mem1`.
- **Worker pool.** One global pool, one batch at a time, joins before return; caller takes the last share; off unless `BLUEWAKE_NATIVE_WORKERS` is set (Linux and Windows hosts default it to 2); serial on Windows ([`native_work_pool.c`](../../../cmake/composite/native_work_pool.c)).
- **Native coverage.** `native_skin.c` (`calcWeightEnvelopeMtx`, `0x802EE67C`), `native_game_math.c` (`key-s`, `transform-simple`, clipper, cM3d), native J3D transform info; all on by default on Linux and Windows, declining back to translation on unsupported inputs.
- **Model calc** (tww `J3DModel.cpp:690`): `j3dSys.setModel`, skin flags, `frameInit`, visibility, deform, two hooks, `calcAnmMtx` → `recursiveCalc` (static `J3DSys::mCurrent*`, joint callbacks In/Out), `calcWeightEnvelopeMtx`, skin deform, `mCalcCallBack`. Outputs `mpNodeMtx`, `mpWeightEnvMtx`, scale-flag arrays; `viewCalc` derives draw matrices.
- **Guest PC sampler.** `BLUEWAKE_PC_SAMPLE=path`, `_FROM`, `_TO` ([`main.c`](../../../runtime/host/src/main.c)); reads `cpu->pc` every 100 µs; overweights short dispatched functions.
- **Tooling gap.** `bench_tour.py` strips `BLUEWAKE_*` from the child environment, so sampler runs need a wrapper app script.

## Changes

| Status | File / symbol | Practical edit | Milestone | Architecture |
| --- | --- | --- | --- | --- |
| Proposed | `scripts/bench_tour.py` | Pass through an allowlist of diagnostic variables (`BLUEWAKE_PC_SAMPLE*`) | M0 | [Testing](../../architecture/Testing.md) |
| Proposed | `scripts/pc_sample_attribute.py` | Attribute `BLUEWAKE_PC_SAMPLE` output to tww functions and subsystems | M0 | — |
| Proposed | `cmake/composite/native_skin.c`, `native_game_math.c` | Count declines by reason (a reason code per guard) | M1 | [Native routines](../../architecture/NativeRoutines.md) |
| Proposed | same, plus generators and oracle tests | Widen the certified domain for the dominant decline reasons | M2 | [Native routines](../../architecture/NativeRoutines.md) |
| Proposed | host diagnostic (opt-in) | Trace reads of one model's `mpNodeMtx`/draw-matrix range between `mDoExt_modelUpdateDL` and the draw-list flush | M3 | [Guest state](../../architecture/GuestState.md) |
| Proposed | new async job path (separate from the pool) | Submit, join at flush, ordered publication; opt-in flag | M4–M5 | [Game thread](../../architecture/GameThread.md) |

## Contracts

**Eligibility** (decided before submission; anything else runs serially as today): no joint callback on any node, no `mCalcCallBack`, no `mpDeformData`/`mpSkinDeform`, no hooks at `0xc4`/`0xc8`, no visibility manager (or proven pure), key-frame or no animation with Basic/Maya/Softimage calc, not the locked `getModelDataType()==1` path, called from `draw()`.

**Async pose job** (proposed):

```
actor draw(): setBaseTRMtx → capture inputs → submit(calc+viewCalc) → entry()
...remaining actors' draw()...
draw-list flush: join all → publish matrices + scale flags → set j3dSys as the last serial calc would → draw
```

Guest cycles for the serial calc are charged at submit. A job never writes live guest memory; publication happens on the game thread.

## Validation

- Native widening: existing differential oracles extended to each widened case; checkpoint and picture comparison over `bench_tour` stops.
- Async: serial build as oracle; identical checkpoints and pictures; A/A then A/B with `render_bench` on the Deck; report p95/p99 update time, not only means.

## Commands

```sh
# guest-PC profile of the Outset stop (wrapper app sets BLUEWAKE_PC_SAMPLE, _FROM=2700, _TO=4200)
python3 scripts/bench_tour.py CARD wrapper.sh --mode headless
python3 pc_sample_attribute.py tww/config/GZLE01/symbols.txt outset.pcs 45
```
