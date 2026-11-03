# Where the game thread's time goes (October 11, 2026)

Speed sprint step S1 ([GOAL_LOOP.md](../GOAL_LOOP.md)). Measured on this Mac (M3 Max, ARM64), with the same
preparations the Windows, Linux and Android builds use, to see what the CPU-bound platforms (Steam Deck, phones,
two- and four-core PCs) spend their time on.

## Build and benchmark

- Source: `main` plus #257, built from Chris's own disc with `scripts/builder/build.sh --platform macos
  --combined-optimizations --lean-blocks` (443,155 lean block copies in 813 chunks, trained locally, tiered).
  Training took 35 minutes and the module compiled in 25 (the plain iPad module took 61 the same day).
- Benchmark: headless, unpaced (`BLUEWAKE_RENDERER=headless`, `BLUEWAKE_WALL_PACE=0`, Smooth Motion off), 2,400
  retraces from save states made at retrace 2,390 from copied cards, with `HOME` and every data path pointed at a
  scratch folder.
- Repeatability: three runs at each place gave identical guest checkpoints and block counts, and times within half a
  percent: the sea by Outset 23.4 s (103 retraces a second), Dragon Roost Cavern 15.0 s (160 a second). Later runs
  on a warmer machine took 25.4 to 25.6 s for the same code, so comparisons alternate builds in one session.
- Profile: macOS `sample` at 1 ms for 20 s at Outset. For line attribution the same module was rebuilt with
  `-gline-tables-only` (identical `__text`, 390,188,784 bytes; identical checkpoints and speed), and each sample's
  address was mapped to its source line with `atos -i` and sorted by what the line does.

## Results

Whole game thread, by function:

| Share | What |
| --- | --- |
| 67% | Translated game code (bookkeeping included) |
| 8.6% | The host's per-call checks (`host_direct_can_skip`, `host_chassis_requires_full`, `host_can_skip_observation`), after #242 |
| 6.1% | Memory and register helpers (`ppc_guest_alias_resolve` alone about 2%) |
| 5.1% | Natives |
| 2.9% | Dispatch and cycle charging |

Inside the game module, by source line (sampling can move a few percent between neighbouring lines):

| Share | What |
| --- | --- |
| 22% | The game's own work |
| 13.4% | `ctx->pc = …` before memory instructions |
| 13.2% | The memory fast path (`gather_pipe.h`: a range check, a copy, a byte swap); mostly the game's real loads and stores |
| 9.5% | Inline cycle accounting: `cycle_observation_suffix` and the deadline check after each memory instruction |
| 7.5% | The dispatcher between blocks (`dispatch_loop.h`, after `dispatch()` returns) |
| ~7% | Exact floating point: FPSCR flags and the MSR[FP] check on every float instruction |
| ~5% | Direct-call readiness (`bw_host_quiet`, `bw_direct_call_ready`) |
| ~5% | Cycle helpers and the loop budget |
| ~3% | Natives, alias lookups, the rest |

About a quarter of the CPU time is Wind Waker's own work; the rest keeps the translation cycle-exact and safe.

## What to change, in order

1. **Observation only where it can be observed.** In the lean copies, write the program counter and the cycle
   observation, and check the deadline after a store, only when the access leaves RAM (memory-mapped hardware,
   where the host can see them). Up to about 20% of the module's time. Proof: identical checkpoints.
   **Done in two steps, both opt-in:** lean memory (fix 1, about 5.5 to 6%) and lean deadlines (fix 2, about 4 to
   5% on top), below.
2. **The host's per-call checks as one flag** the module reads inline, with the full check as the fallback. About
   9 to 14% of the game thread.
   **Measured October 12: no gain** when the module answers inline (below); the cost is the question, not the call.
3. **Floating point:** check MSR[FP] once per block (safe). Exact FPSCR flags (up to 7%) change behaviour if the
   game reads them, so that one needs Chris's decision.
4. **Fewer dispatches** (7.5%): more direct chaining, longer term.
5. **A last-hit cache for alias lookups** (about 2%): the binary search over about 1,900 ranges runs on every access
   to an actor module's data.

## S2, fix 1: lean memory (October 12)

Elliott Tate's `lean_memory.py` (`--lean-memory` in the Windows and Linux builders) already does fix 1: a memory
instruction in a lean copy passes its program counter and cycle observation to `bw_read*_at`/`bw_write*_at`, which
store them only when the access leaves RAM. It converted nothing while BlueWake used its conservative block copies
(#179); with lean blocks as the default it converts 501,016 accesses in all 813 chunks. #257 adds it to the Mac
builder for this measurement.

Two modules from the same prepared sources, one with `lean_memory.py` and one without, both compiled without a
training profile and with the same 504 tiered chunks (so the only difference is the step), alternated A, B, A, B on
this Mac, headless, 2,400 retraces from states (first run of each pair dropped: both were cold from disk):

| | Lean | Lean + lean memory | |
| --- | --- | --- | --- |
| The sea by Outset | 26.5, 26.7 s | 25.2, 25.2 s | about 5.5% faster |
| Dragon Roost Cavern | 16.0, 15.9 s | 15.0, 15.0 s | about 6% faster |
| Code size | 340.1 MB | 348.6 MB | +2.5% |

**It plays the same.** Both execute the same number of blocks; in the cavern every checkpoint is identical. At
Outset one checkpoint of eight (retrace 3,600) has a different CPU hash. A local diagnostic build (not committed)
printed the fields: same program counter, the rest of the CPU state and all memory identical, and only
`cycle_observation_suffix` differs (1 against 2), the bookkeeping value lean memory leaves stale on the RAM path,
which nothing reads there. Every checkpoint after it matches again.

**On Apple builds it didn't link with training** (fixed October 12, below). The instrumented module for local
training was too large for Apple's linker: `B/BL out of range (max is +/-128MB)` after "branch islands: maximum
cluster count reached". The module without instrumentation links. Windows and Linux (x86) aren't affected; Android
uses a different linker.

**The fix: train at `-O1` with lean memory.** Training compiles its instrumented module at `-O0`, the quickest
compile; the counters are the same at any level. With lean memory that module has 2.4 GB of code, out of reach of
the linker's branch islands. At `-O1` it has 822 MB and links with no warning. The Mac builder now asks for `-O1`
when `--lean-memory` is on (`train_local_pgo.py --instrumented-opt-level 1`). Training takes about as long: on this
Mac (16 cores) the instrumented compile grows from about 4 to about 28 minutes, and the training runs shrink from
about 31 to 3.5 minutes, because the instrumented game runs faster (31.5 minutes in all, against 35 for the lean
build at `-O0`). A Mac with fewer cores pays more for the compile, so the other builds keep `-O0`.

A complete build with `--combined-optimizations --lean-blocks --lean-memory --lean-deadlines` took 57 minutes and
compared with the trained lean build (`--lean-blocks` alone), both trained, alternated, three runs each after a
cold first pair:

| | Lean blocks, trained | + lean memory + lean deadlines, trained | |
| --- | --- | --- | --- |
| The sea by Outset | 22.36, 22.35, 22.27 s | 20.93, 20.98, 20.88 s | 6.3% faster |
| Dragon Roost Cavern | 14.12, 14.22, 14.12 s | 13.11, 13.14, 13.08 s | 7.4% faster |
| Module | 433.0 MB | 450.5 MB | +4% |

Same blocks, and the same memory, cycles and aliases at every checkpoint. The trained gain is smaller than the two
steps' untrained gains added up (about 9 to 11%), as the training already speeds up some of what they remove.

## S2, fix 2: lean deadlines (October 12)

After lean memory, every load and store in a lean copy still tests the cycle deadline after it (does the rest of the
block reach the next device deadline?), and so do moves to and from the link, count and XER registers, which the
translator counts with `mfspr` and `mtspr` as possible timer reads. In the prepared source that is 826,396 tests,
760,000 of them after a plain access or one of those moves.

Those tests can't fire. A lean copy runs only when `dolrecomp_block_can_precharge` found the next deadline at or
past the block's end, so the deadline budget is at least the block's cycles and every suffix in the block is at most
that; suffixes only shrink through a block, and a test that passes leaves the budget at least its suffix. Only a call
can move the budget: an access's way out of line (MMIO, the decrementer and the timebase all observe and re-budget
there), or another helper an instruction calls.

`scripts/windows/lean_deadlines.py` (new, after `lean_memory.py`) keeps an access's test only on its way out of line
(`bw_readN_ato`/`bw_writeN_ato` in `gather_pipe.h` set a flag there, and the compiler drops the test on the RAM
path), and drops a link, count or XER move's suffix store and test. Where an instruction since the last test called
anything else, the next test stays as it was. It converts all 813 chunks. Builders: `--lean-deadlines` (needs
`--lean-memory`), off by default, in the Mac, Windows and Linux builders.

Same setup as fix 1 (two modules from the same prepared sources, no training profile, alternated, 2,400 retraces
from states, four runs each):

| | Lean + lean memory | + lean deadlines | |
| --- | --- | --- | --- |
| The sea by Outset | 23.68, 23.96, 23.97, 24.00 s | 22.98, 22.98, 23.06, 23.09 s | 3.7% faster |
| Dragon Roost Cavern | 14.67, 14.73, 14.79, 14.80 s | 13.93, 13.97, 14.07, 14.12 s | 4.9% faster |
| Code size | 384.7 MB | 417.3 MB | +8.5% |

Every run with it was faster than every run without it. (This Mac ran faster today than for fix 1, so compare
within a table, not across.)

**It plays the same.** Same blocks at both states; memory, the cycle count and the aliases identical at every
checkpoint; and with the diagnostic build, the program counter and the rest of the CPU state identical too. Only
`cycle_observation_suffix` differs, at two checkpoints of eight at Outset and all of them in the cavern: the
bookkeeping value, which nothing reads until an observation stores its own.

## S2: the host's per-call checks read inline (October 12, no gain)

A profile of the lean memory module at Outset put the direct-call checks at about 8.5% of the game thread's busy
samples: `host_direct_can_skip` 4.8% and the module's `bw_direct_call_ready` (which the compiler keeps out of line,
in the trained build too) 3.7%. A local build (branch `codex/direct-skip-fast`, not opened as a pull request) let
the module answer the host's question itself: the address checks inline, and the scene's overlap watch read through
pointers to the host's own variables, so it is the host's test made at the same moment. A first version that only
handled the time before the name scene answered nothing in play: after the file select the overlap watch is on for
good.

The full version answered every direct call inline (440,598,355 at Outset, 267,599,471 in the cavern), and a verify
mode that asked the host as well found no disagreement either way; checkpoints identical. But it was no faster:
Outset 23.94 s against 23.90 s, the cavern 14.90 s against 14.96 s (four runs each, within noise). The cost is the
question itself (the overlap watch's reads and the address tests), not the trip into the host. Making it cheaper
means asking less often: for example, a host that reads the overlap phase once per turn, which changes when the host
reacts, so it is Chris's call.

## S2, fix 3: the lazy overlap watch (October 12)

A profile of the trained build with lean memory and lean deadlines put `host_direct_can_skip` at 7% of the game
thread's busy samples, and LiquidAzir's on the Fold 7 at 10.7 M instructions a retrace, with the module's
`bw_direct_call_ready` at 7 M (#93). Answering the same question inside the module saved nothing (above): the cost
is the question itself, mostly the scene overlap watch, which reads the overlap object at `0x803F6160` and its
phase from guest memory at every direct call and block edge.

That watch only records the phase in `g_overlap_last_phase` and `g_overlap_terminal_phase`, which nothing but the
diagnostics and the test routes read (frame captures, player traces, scripted pad pulses, the route probes). The
game never sees it. `BLUEWAKE_LAZY_OVERLAP=1` (off by default) leaves it to the edge service and the per-turn
sample, which already read it, and drops it from the per-call checks. With that, the host's answer depends on the
address alone, so a module that has `bluewake_composite_direct_calls_v3` (new) reads it inline: the jump button and
the diagnostics as one flag the host keeps current, plus the four addresses and ranges the host watches whatever
their features' switches. `BLUEWAKE_DIRECT_SKIP_VERIFY=1` asks the host as well and counts disagreements.

Lean blocks, lean memory and lean deadlines, no training profile, the same host, alternated, three runs each after
the first pair:

| | Off | `BLUEWAKE_LAZY_OVERLAP=1` | |
| --- | --- | --- | --- |
| The sea by Outset | 22.96, 22.85, 22.95 s | 21.98, 22.01, 22.07 s | 3.9% faster |
| Dragon Roost Cavern | 13.89, 13.92, 14.02 s | 13.32, 13.33, 13.23 s | 4.7% faster |

**It plays the same.** The module answered every direct call inline (440,598,355 at Outset, 267,599,471 in the
cavern), and the verify mode found no disagreement with the host either way. Checkpoints are identical, and
identical with the switch off to the lean deadlines module's. Since the watch matters only while scenes change
over, four boots from the title through the file select and a scene handover into Outset, Windfall, Dragon Roost
Cavern and the Tower of the Gods, with a checkpoint every 100 retraces, are identical off and on (the inline answer
is off with the stage select, so those runs check the lazy watch alone).

The first version answered inline only before the name scene and so never in play; a second read the overlap
watch through pointers to the host's variables and agreed on every call but was no faster.

## Measured, not proposed: the floating-point helpers inline (October 12)

The trained module keeps 528 out-of-line copies of `inline_fp.h`'s helpers, so every `fmuls` or `fadds` pays a call.
Forcing them inline (`always_inline`, local branch `codex/fp-always-inline`) gave a module 11.5% larger and slower:
Outset 22.66 s against 22.94 s (1.2%), the cavern 13.86 s against 14.20 s (2.4%), checkpoints identical. The calls
cost less than the code they would add.

## S2, fix 4 (not pinned): the alias resolver's last hit (October 12)

`ppc_guest_alias_resolve` (RecompCore `GXRuntime/src/core/cpu.c`) was 2.2% of the trained build's game thread: a
binary search over about 1,900 aliases on every access to an actor module's data. It now tries the alias it found
last first. While no two aliases overlap (the only case that takes the search), at most one can hold an access, so a
remembered alias that holds it is the one the search would find; any other answer goes to the search as before. The
index is a relaxed atomic, not thread-local (a thread-local in a Mach-O dylib costs a call per access); a stale one
is only a miss.

With `BLUEWAKE_LAZY_OVERLAP=1` on both, the same host and sources, alternated, three runs each after a cold first
pair: Outset 21.89 s against 21.79 s (0.5%), Dragon Roost Cavern 13.20 s against 13.03 s (1.2%), every run faster,
identical checkpoints. It is a RecompCore change, on for everyone once pinned, so it waits for Chris: the commit is
`f4afb6f` on a local branch `codex/alias-last-hit` of the benchmark checkout's RecompCore, on the pinned `ddd031b`.
Later on October 12 it went up as chrissotraidis/RecompCore#34 on `ee99bbc` (with the HD look and the flush fix). There, alternated on a quiet Mac, it made no measurable difference at Outset (24.69 s against 24.73 s, medians) and 0 to 2.5% in Dragon Roost Cavern, with identical checkpoints, so it is not pinned. On a trained module (main at #298) it was no better: Outset 0.5 to 0.7% faster in three of four pairs, Dragon Roost Cavern 1 to 3% slower in three of four, identical checkpoints. RecompCore #34 is closed; its branch stays.

## Everything together (October 12)

The lean blocks build (what 0.7.0 ships to PCs) against lean memory, lean deadlines, the lazy overlap watch
(`BLUEWAKE_LAZY_OVERLAP=1`) and the alias resolver's last hit, all four on. Untrained modules from the same prepared
sources, the same host, alternated, three runs each after the first pair:

| | Lean blocks | All four | Time | Retraces a second |
| --- | --- | --- | --- | --- |
| The sea by Outset | 25.15, 25.04, 25.00 s | 21.87, 21.73, 21.68 s | 13.2% less | 15% more |
| Dragon Roost Cavern | 15.71, 15.56, 15.64 s | 13.13, 12.99, 13.06 s | 16.5% less | 20% more |

Same blocks, and the same memory, cycles and aliases at every checkpoint (the CPU differs only in
`cycle_observation_suffix`, as with each step). On a CPU where the game thread sets the frame rate (the Steam Deck,
the Fold 7, a 2-core laptop), that is the size of the change to expect if it carries over from ARM on a Mac, which
only those machines can show.

What the game thread spends after these (the same Outset sample, all four on): the module's per-call check
`bw_direct_call_ready` 6.1% (two per direct call: the interrupt sources through four host pointers, the cycle budget
and the inline answer); the memory and register helpers 6.5%; the dispatcher 3.3%; the floating-point helpers about
6% together; the host's per-call checks down to 2.6% from 9.7%. The next cut in the per-call check is the host
keeping one "interrupt pending" byte current, which needs a RecompCore change (`interrupts.c` writes the PI
registers), so it waits with the alias change for Chris.
Another RecompCore one: `bw_inline_gpr_memory_ready` (1.5%, `inline_gpr.h`) checks each of up to 18 saved
registers' words with `get_ram_ptr`, so each inline register save or restore asks the alias resolver up to 18
times; one "does any alias overlap this span" query from `cpu.c`'s sorted index would answer for the whole frame.

## October 12: the module's per-call check

`bw_direct_call_ready` (direct_calls.h) runs before each direct call and again on its return; it is inlined at
500,773 call sites. Two experiments, M3 Max, untrained lean-deadlines modules from the same prepared source, the same
app, alternated, 3 rounds of 2,400 retraces from save states:

| Build | Outset (s) | Dragon Roost Cavern (s) | __text |
| --- | --- | --- | --- |
| Inlined (as in `main`) | 28.16, 27.82, 28.08 | 16.75, 16.35, 16.77 | 360.7 MB |
| One out-of-line function (`noinline`, `preserve_most`) | 31.02, 27.77, 28.07 | 16.36, 16.61, 16.40 | 361.8 MB |

Identical checkpoints. No gain on this Mac, and the module's code grows slightly, so the inlined check wasn't the
code bloat it looked like (the compiler shares most of it). Not proposed.

Hidden symbols are off in the Android build (#93), so each global the check reads goes through the GOT. Compiled
with the NDK (r29, API 33) at -O2, the busiest chunk (`chunk_0091_text1_8016D6E0`, 2,872 call sites) has 6,491 GOT
loads with the declarations as they are and 5,055 with only direct_calls.h's globals declared hidden (146,805 against
145,363 instructions). Worth measuring on the Fold 7; it changes nothing where hidden symbols are on.

