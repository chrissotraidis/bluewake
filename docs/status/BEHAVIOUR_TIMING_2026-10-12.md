# Game-behaviour timing (T1): what's left to take out

October 12, 2026. A design note for the prototype in [GOAL_LOOP.md](../GOAL_LOOP.md) (T1), from the prepared source of
an untrained lean-deadlines Mac build. Nothing here is built yet.

## Where a block's time goes now

With lean blocks, lean memory and lean deadlines (on by default in the Windows, Linux and Android builders), every
block has two copies. The fast copy (`bwfast_N`) runs the block's instructions with no per-instruction bookkeeping;
the precise copy (a `ctx->pc` store and `dolrecomp_charge_precise` before each instruction) only runs when the
cycle budget is nearly spent. What every block still pays on the fast path:

- `ctx->pc` stored at the block's start;
- `dolrecomp_block_can_precharge`, the turn's cycle budget compared, `ctx->downcount` decremented;
- the guest's registers in `ctx->gpr[]`; with `BLUEWAKE_FIXED_CPU` the CPU state and guest RAM are separate
  globals, so the compiler already keeps most of them in host registers inside a block (the busiest chunk compiled
  for the Mac: about 6,400 32-bit loads and 9,300 stores at an offset in 233,000 lines of assembly, many of them
  the `ctx->pc` store at offset 640);
- at each direct call, `bw_direct_call_ready` before the call and on the return (inlined at 500,773 sites; moving it
  out of line was no faster on the M3 Max, see [SPEED_PROFILE](SPEED_PROFILE_2026-10-11.md)).

## What "behaviour timing" would change

The HD recomp reproduces what the game does each frame, not how many cycles the console took, which is where its
speed comes from. The equivalent here, behind a translator switch:

1. **Cycles charged per block from a table, checked only at loop back-edges and calls.** Today's per-block budget check
   becomes a check where control can loop. The decrementer, the audio DMA and VI keep their own time from the
   charged cycles, so the game's clock still advances; it advances in larger steps.
2. **Guest registers kept in host registers across blocks,** not only inside one: the compiler already does it
   inside a block (see above), so what's left is the write-back and reload at every block boundary and call. Needs
   the translator to know which instructions can leave the block.
3. **No `ctx->pc` store on the fast path;** the exit sets it.

## How it would be judged

The same as every speed step: headless from save states at Outset, the bird scene and Dragon Roost Cavern, guest
checkpoints compared, then by hand. Unlike the earlier steps the checkpoints *will* differ (the clock moves in larger
steps), so the bar is that the game behaves the same: the 468-place stage select sweep, cutscenes with their sound in
time, the audio stream without starving, a save made and loaded. Build time measured too: fewer copies of each block
is also what a first-start build in minutes needs ([DIRECTION.md](../DIRECTION.md)).

## Where it lives

DolRecomp (the translator, a submodule of RecompCore) and the composite's preparation scripts. A prototype is a
DolRecomp branch plus a `--behaviour-timing` builder option, off by default; Chris decides any default.

## Results

Steps 1 and 2 are built: `scripts/windows/behaviour_timing.py --loop-checks`, `--behaviour-timing` in the Windows,
Linux and Mac builders, off by default.

- **Step 1, every block prepaid.** Each block start that has a prepaid copy (384,783 of them) runs that copy and is
  charged its cycles at once; the program counter is stored only on the way out to the host.
- **Step 2, the budget checked at loops.** The turn's budget is then checked only at a block that a jump goes back to
  by guest address (a loop's start). A call still checks it (`bw_direct_call_ready`), and so do the dispatcher and
  each chunk's return dispatch, so control still comes back within one pass of straight-line code. In the busiest
  chunk, 129 of its 946 block starts keep the check.

Guest registers across blocks (point 2 of the plan above) are not in it.

M3 Max, untrained modules from the same prepared source (lean blocks, memory and deadlines), the same app, alternated,
three rounds of 2,400 retraces from save states (the first run of a new module is slow each time, loading it; medians):

| | Without | Step 1 | Steps 1 and 2 |
| --- | --- | --- | --- |
| Outset (s) | 25.97 | 24.48 (5.7%) | 24.14 (7.0%) |
| Dragon Roost Cavern (s) | 15.18 | 14.07 (7.3%) | 13.80 (9.1%) |
| Blocks run, Outset / cavern | 1,237,187 / 1,250,840 | 1,237,192 / 1,250,836 | 1,237,146 / 1,250,837 |
| Module | 418.1 MB | 391.7 MB | 382.6 MB |
| Module compile (CPU time) | 15,286 s | 11,061 s | 10,084 s |

The game state is not identical, by design: an event can land a block later. What it should keep, on both steps:

- The 468-place stage select sweep from a Dragon Roost save: the same outcome at every place as without it (446
  arrive, the same 11 stop at the retrace limit and the same 11 known crashes), and the card unchanged after all 468.
- The sound: the audio the game produces over each 2,400-retrace run, captured with `BLUEWAKE_CAPTURE_AUDIO_WAV`, is
  identical sample for sample at both places.

## On x86 (pdale-boop, October 12, #215)

Trained Linux builds from one commit (a3ee3ff), the default lean blocks, lean memory and lean deadlines, one also with
`--behaviour-timing`; clang 23.1.1, `-march=x86-64-v3`. Headless from save states, 2,400 retraces, three alternated
rounds, median retraces a second with the pace off:

| | Outset | Bird scene | Tower |
| --- | --- | --- | --- |
| Intel i5-6500 (the same Debian app, only the module swapped) | +7.1% | +9.3% | +7.6% |
| i5-12600KF, its four E-cores | +8.1% | +8.4% | +8.0% |
| i5-12600KF, its P-cores | +9.5% | +10.6% | +10.1% |

Faster in all 27 pairs; 6.6 to 7.8% fewer instructions, the same instructions per cycle, 5 to 7% fewer L1 instruction
cache misses. The module's code 476 MB to 406 MB (15% smaller), its compile 18 min 12 s to 12 min 15 s.

## On an iPad Pro (M2), October 12

Two personal iOS builds from the same commit, both trained, with `--combined-optimizations --lean-blocks --lean-memory
--lean-deadlines`, one also with `--behaviour-timing` (401,618 block starts always prepaid). The same copied test
cards, scripted presses and 7,200 retraces; the game thread's share of each frame (median of 85 samples):

| | Installed 0.7.0 build | Prepared | Prepared, behaviour timing |
| --- | --- | --- | --- |
| The sea by Outset | 81% | 54% | 52% |
| Dragon Roost Cavern | 53% | 43% | 41% |

59.9 retraces a second in every run, no crash, no thermal warning, and the player's own save unchanged throughout. On the
iPad, behaviour timing takes about 4 to 5% of what is left, less than on the Mac.

Not checked: played by hand, a PC, the Deck, a phone, and long runs (cutscenes with their sound in time, a save made and
loaded).

## October 13 and 14: other machines, and the default

Measured by contributors, each with both builds made from the same commit:

| Machine | Who | Result |
| --- | --- | --- |
| Intel i5-6500 and i5-12600KF, Linux | pdale-boop (#215) | 7 to 9% and 8 to 11% faster, all 27 pairs; module 15% smaller |
| Steam Deck (SteamOS 3.8, 4 cores) | jkoehler11 (#215) | 63.6 to 69.4 retraces a second at Outset's sea (9%), three interleaved rounds each; module 518 to 444 MB |
| Galaxy Z Fold 7 (Android) | LiquidAzir (#93) | 4% fewer instructions and 10% fewer cycles a retrace at Outset's pier; module 10.5% smaller |

Played with it: pdale-boop on Linux at 21:9 and 60 FPS, and jkoehler11 on the Deck (a cutscene with music, and sailing).
Neither noticed anything look or sound different. On that evidence the Windows and Linux builders turn it on by default
from October 14; `--no-behaviour-timing` builds without it. The Mac, iPhone and iPad builder keeps it opt-in.
