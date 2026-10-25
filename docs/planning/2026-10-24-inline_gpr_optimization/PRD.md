# PRD — Inline GPR save/restore optimization

Status: Implemented and measured candidate; upstream integration open
Source: user handoff SHA-256 `185ccc3b6575a46f11fb4d460993513409d33fb0f392d82d83f5d8a761bb94bb`
Scope: translated CPU save/restore overhead; excluded: renderer, GPU work, new native routines, and direct replacement of memory helpers

[Technical plan](TECH_PLAN.md) · [Progress](PROGRESS.md) · [Companion RecompCore PR](https://github.com/chrissotraidis/RecompCore/pull/45) · [Parent performance tracker](https://github.com/chrissotraidis/bluewake/pull/421)

## 1. Context and objective

BlueWake translates GameCube PowerPC instructions into C, which is subsequently compiled into native x86-64 code.

The investigation concerns overhead introduced by BlueWake when reproducing the original game's register save/restore operations.

**Objective:** Determine whether repeated memory validation and address resolution can be reduced without changing guest-visible behavior, contributing toward stable 60 FPS presentation with HD textures on Steam Deck.

The supplied handoff described an optimization hypothesis. The candidate is now implemented and measured; this document preserves the original reasoning and records the resulting behavior and evidence.

## 2. Original GameCube execution model

The GameCube's Gekko CPU has 32 general-purpose registers (`r0`–`r31`).

When a function calls another function, certain registers must retain their values. The original game uses shared `_savegpr_N` and `_restgpr_N` routines to save and restore registers `rN` through `r31`.

```text
          Original GameCube execution
                     │
                     ▼
              Game function A
                     │
                     ▼
              _savegpr_N
          Save registers to stack
                     │
                     ▼
              Game function B
                     │
                     ▼
              _restgpr_N
         Restore registers from stack
                     │
                     ▼
              Resume function A
```

For example, `_savegpr_14` saves 18 registers, occupying 72 contiguous bytes.

A representative PowerPC instruction:

```asm
stw r14, -72(r11)
```

stores the 32-bit value of `r14` at guest address `r11 - 72`.

## 3. BlueWake's translation model

BlueWake executes translated game code rather than original PowerPC instructions.

```text
          Original GameCube executable
               PowerPC instructions
                        │
                        ▼
                   DolRecomp
              PowerPC → C translation
                        │
                        ▼
                 Generated C code
                        │
                        ▼
                     Clang
               C → x86-64 machine code
                        │
                        ▼
                  Steam Deck CPU
```

The translated program represents the original PowerPC CPU state in a C structure:

```c
struct CPUState {
    u32 gpr[32];    // Guest PowerPC registers
    // ...
    u8* ram;        // Guest RAM backing storage
    u32 ram_size;
};
```

A PowerPC store becomes conceptually:

```c
mem_write32(cpu, cpu->gpr[11] - 72, cpu->gpr[14]);
```

Unlike a native memory store, `mem_write32()` must resolve the guest address into host memory and preserve the relevant guest memory semantics.

## 4. Guest memory resolution

GameCube guest addresses are not necessarily valid host-process addresses.

BlueWake uses RecompCore's `get_ram_ptr()` to resolve addresses to their corresponding backing storage.

```text
           Translated PowerPC store
                      │
                      ▼
              Guest address
              e.g. 0x800000B8
                      │
                      ▼
                 get_ram_ptr()
                      │
          ┌───────────┼────────────┐
          │           │            │
          ▼           ▼            ▼
      Ordinary      Aliased      Other memory
        MEM1        memory       MEM2 / device
          │           │            │
          ▼           ▼            ▼
     cpu->ram +    Separate      External or
       offset      storage       special path
```

For ordinary MEM1, address resolution is approximately:

```c
host_pointer = cpu->ram + (guest_address - GC_RAM_BASE);
```

But aliases, mirrors, external memory and MMIO require additional handling.

Consequently, contiguous guest addresses do not universally guarantee contiguous ordinary-RAM backing.

## 5. Existing BlueWake optimization: inline save/restore

Originally, calling `_savegpr_N` or `_restgpr_N` required transitions between translated code regions and their dispatcher.

BlueWake already optimizes this by inserting the register operations directly into the caller.

```text
BEFORE INLINING

  Translated function A
           │
           ▼
     Dispatch boundary
           │
           ▼
     _savegpr_N chunk
           │
           ▼
     Dispatch boundary
           │
           ▼
  Resume function A


AFTER INLINING

  Translated function A
           │
           ▼
     Eligibility checks
           │
      ┌────┴────┐
      │         │
     Pass      Fail
      │         │
      ▼         ▼
  Inline GPR   Original
  operations   call path
      │
      ▼
  Resume function A
```

The existing preparation pass rewrote **5,728 call sites** in the measured Clang 18 build. This measures static coverage, not runtime execution frequency.

Relevant generator: [`scripts/windows/inline_save_restore_gpr.py`](../../../scripts/windows/inline_save_restore_gpr.py).

## 6. Baseline memory eligibility checks

Before executing inline register operations, BlueWake calls [`bw_inline_gpr_memory_ready()`](../../../cmake/composite/inline_gpr.h).

At the tested control snapshot, the function checked every register's stack address individually. For `_savegpr_14`, the sequence was:

```text
             Inline GPR eligibility
                       │
                       ▼
               Validate r14 address
                       │
                       ▼
               Validate r15 address
                       │
                       ▼
               Validate r16 address
                       │
                      ...
                       │
                       ▼
               Validate r31 address
                       │
                       ▼
               All checks passed?
                       │
                ┌──────┴──────┐
                │             │
               Yes            No
                │             │
                ▼             ▼
          Execute inline   Original call
          save/restore       fallback
```

Each iteration verified:

- Four-byte address alignment.
- Bounds within allocated guest RAM.
- Correct ordinary-MEM1 backing through `get_ram_ptr()`.

The helper also rejected inlining when memory journaling was active.

### Why individual validation existed

Consider a 72-byte stack frame with a four-byte alias in the middle:

```text
                  72-byte stack frame

 r14   r15   r16   r17         r23              r31
  │     │     │     │           │                │
  ▼     ▼     ▼     ▼           ▼                ▼
 ┌────┬────┬────┬────┬─── ... ┬────┬── ... ───┬────┐
 │RAM │RAM │RAM │RAM │        │ALIAS│         │RAM │
 └────┴────┴────┴────┴─── ... ┴────┴── ... ───┴────┘
                               ▲
                     Different backing storage
```

Resolving the entire 72-byte range using one `get_ram_ptr()` call is insufficient: a partial alias inside the range may be missed.

The focused regression coverage remains in [`tests/inline_gpr_memory_test.c`](../../../tests/inline_gpr_memory_test.c).

## 7. The identified redundancy

Once eligibility succeeds, the generated inline code still performs individual memory operations:

```c
mem_write32(ctx, frame + offset, value);
```

Each `mem_write32()` calls `get_ram_ptr()` again.

The baseline execution flow was therefore:

```text
                 Inline GPR operation
                          │
                          ▼
               Check host readiness
                          │
                          ▼
            bw_inline_gpr_memory_ready()
                          │
            ┌─────────────┴──────────────┐
            │                            │
            ▼                            ▼
       Validate r14                 Validate r15
       get_ram_ptr()                get_ram_ptr()
            │                            │
            └─────────────┬──────────────┘
                          │
                      ... r31
                          │
                          ▼
                Eligibility passed
                          │
                          ▼
                  mem_write32(r14)
                     get_ram_ptr()
                          │
                          ▼
                  mem_write32(r15)
                     get_ram_ptr()
                          │
                         ...
                          │
                          ▼
                  mem_write32(r31)
                     get_ram_ptr()
                          │
                          ▼
                Resume translated code
```

For an 18-register save, this could involve up to 18 readiness resolutions followed by 18 additional memory resolutions.

Not every resolution necessarily performs a full alias search: ordinary MEM1 already has a fast path. Nevertheless, the repeated validation work was a plausible source of overhead.

## 8. Implemented range-level validation

For ordinary MEM1, the candidate proves that the entire contiguous stack range is safe using one range-level decision.

```text
BASELINE MODEL

  Validate r14
  Validate r15
  Validate r16
       ...
  Validate r31
       │
       ▼
  Execute 18 memory operations


CANDIDATE MODEL

  Compute complete stack range
       │
       ▼
  Bounds and alias-range checks
       │
    ┌──┴───┐
    │      │
   Pass   Fail
    │      │
    │      ▼
    │   Original translated
    │   routine fallback
    │
    ▼
  Access the proven frame bytes directly
```

The conservative fast path verifies:

1. Valid register range and four-byte alignment.
2. The complete stack range lies within MEM1 bounds.
3. Neither the original nor canonical address range overlaps a registered alias.
4. No memory journal is active.

The supporting range query is under review in [RecompCore #45](https://github.com/chrissotraidis/RecompCore/pull/45). BlueWake's implementation is in [`cmake/composite/inline_gpr.h`](../../../cmake/composite/inline_gpr.h).

After eligibility is proven, the optimized path reuses the validated MEM1 backing range and performs the register loads and stores with `read_be32` / `write_be32`. A save clears any reservation covered by the frame once before writing. Cycle accounting, host checks and the original translated fallback remain unchanged. This follow-up came from [`pdale-boop` commit `043495d`](https://github.com/pdale-boop/bluewake/commit/043495d55f0eae2a6cf1d1d4b64852a982bc0a36).

## 9. Performance questions and findings

Three different measurements matter:

| Measurement | Meaning | Current evidence |
|---|---|---|
| Rewritten call sites | How many locations contain the optimization | 5,728 static sites in the measured Clang 18 build |
| Runtime executions | How often those locations execute | Runtime counters were inconclusive and are not used as activation proof |
| End-to-end throughput | Whether the complete candidate advances game work faster | Repeatable positive five-scene Steam Deck result |

The relevant relationship remains approximately:

```text
Total validation cost
       =
Execution frequency × Cost per validation
```

The 5,728 rewritten sites alone do not establish runtime frequency. The accepted evidence instead compares complete artifacts across five loaded scenes, equal retrace windows and matching guest checkpoints. Two rounds reverse run order.

| Steam Deck Game Mode | Round 1 | Round 2 | Mean paired change |
|---|---:|---:|---:|
| Headless | `43.9139 → 44.9400` | `43.6836 → 44.8011` | **+2.447%** |
| Rendered | `39.4510 → 40.4019` | `39.9174 → 40.8888` | **+2.422%** |

All five scenes improved in all four pairs, all scenes loaded, and all 31 checkpoints matched. A player-facing island tour completed without a reported gameplay issue. The complete methodology and rejected invalid attempts are recorded in the [technical plan](TECH_PLAN.md#measurement-methodology-and-findings).

Aside from the GPR optimization, its RecompCore alias-range API, integration wiring and tests, the control and candidate source are identical. There is no unrelated source-code change confounding the comparison.

The two artifacts used separately generated local PGO profiles. The GPR source change can legitimately change what training observes, so this measures the complete GPR candidate under its normal training process. It does not isolate how much of the gain comes directly from the source edit versus compiler decisions mediated by the new profile. A matched-profile qualification remains open.

## 10. Scope and expected outcome

This work targets **translated CPU execution overhead**, not rendering or GPU performance.

The change is small and reversible, with focused correctness tests for ordinary RAM, aliases, mirror aliases, bounds, MMIO and journaling. It does not replace the existing register memory operations or weaken the translated fallback.

The measured candidate improved five-scene Steam Deck throughput by about 2.4% in both headless and rendered runs. It does not by itself establish stable 30 FPS game execution or continuous 60 FPS presentation in every scene.

**Measured snapshots:** BlueWake `f9205a53de307caac4e306e2a744609b1565135b`; RecompCore `0ab73ca9a673553675b8e511882e6f6b485407eb`.

**Review snapshot:** RecompCore `47a5229cbfb3387cfd21c2b9d304b303a54a6767` in [RecompCore #45](https://github.com/chrissotraidis/RecompCore/pull/45); BlueWake dependency pin and integration in [BlueWake #480](https://github.com/chrissotraidis/bluewake/pull/480).

**Remaining gates:** merge the RecompCore API upstream, replace the temporary fork pin, requalify the current-main build under an explicitly matched PGO policy, and pass Linux and Windows CI.
