# What we're doing

The dated plan. Ask "what are we doing on October 11?" and the answer is that day's section below. Owner: Chris.
Updated October 11, 2026.

**The aim for the next versions:** BlueWake runs faster and steadier, the bugs players reported are fixed, Linux
becomes a download, and Android gets to the iPhone app's standard. Most of the speed is already written, by Elliott
Tate after October 3 and by contributors, so bringing it in, measured, comes first. The ranked backlog is
[PRIORITIES.md](PRIORITIES.md), the speed strategy and its runbook are [PERFORMANCE.md](PERFORMANCE.md), and the
standards everything is held to are in [DIRECTION.md](DIRECTION.md).

## The loop

Every session, by a person or a bot, runs the same five steps:

1. **Read** today's section, then check `main`, open pull requests and new issue replies.
2. **Do** the first row that isn't done and isn't waiting on someone. Before writing anything new, check whether an
   open pull request, or Elliott's `windows-release`, already does it.
3. **Prove** it. Say what ran, on what device, from which commit. A build that compiles is not a game that plays.
4. **Record** it. Mark the row done here, add numbers to [PERFORMANCE.md](PERFORMANCE.md#results), and tell the
   reporters on their issues, in Chris's voice.
5. **Re-plan** when the evidence changes. Unfinished rows move to the next day. Every row has a "done when".

## Speed, in three horizons

The full reasoning is in [PERFORMANCE.md](PERFORMANCE.md#the-plan). In short:

| Horizon | When | What | Gate |
| --- | --- | --- | --- |
| 1. Lean what we have, and catch up with Elliott | 0.7.0 and the next two weeks | Lean block copies (default since today), draw fusion, his faster loads and natives rounds 4, 5 and 7, his upload changes, hidden symbols, Smooth Motion pacing | Faster on the benchmark, and the game plays the same |
| 2. Behavior, not cycles | 0.8 (weeks) | Lighter timing, natives that remove round trips, cached display lists | The benchmark, and plays the same |
| 3. Follow the decompilation | Months | Natives compiled from its source, drawing at the GX/J3D API level, matched scenes ported | Each piece checked against the recompilation |

## Where things stand on October 11

- **0.7.0 is out** (October 10, from `66b33be`): Windows zip, the app-only IPA, source and the PadMint recipe. Linux builds
  from source; the AppImage was not in it and still needs a contributor's own-disc build (#56). Each issue in its
  release record was told to try it. The Mayflash adapter (#155) is confirmed fixed and closed.
- **Merged today:** pdale-boop's #242 (the host's per-block checks do less: 7% fewer instructions, 5 to 8% faster on
  slow x86 CPUs, identical game state) and #210 (the developers' stage select and `BLUEWAKE_WARP`, off by default and
  costing nothing when off). Reviewed: every diagnostic switch is set before #242's summary flags, the module already
  runs the interrupt test in `bw_host_quiet`, and the Mac and iPhone builder uses the same `direct_calls.py` watch list.
- **In CI:** draw fusion on for Linux (#247, from jkoehler11's Steam Deck run on #215) and Swap L and R (#248, #244).
- **Android (#93):** every item on its merge checklist is done; speed is the gate (Outset 67 to 80% on the Fold 7 against
  "holds 30"). LiquidAzir has been asked to merge `main` with #242, turn hidden symbols off for Android and measure.
- **New reports:** a 2-core A6 laptop that can't hold 30 at sea, game thread (#246); D-pad in the item screens (#245);
  the screen recorder losing the Vulkan device on Android (#238). The flicker's two earlier switches are ruled out (#136).
- **A new release gate (Chris):** before announcing a release PadMint builds on a Mac, PadMint's
  `scripts/player-check.sh` passes, on the clean checkout before tagging and again after publishing.

## Next: the next release

Not dated yet. It carries #242, #210, #247 and #248, plus whatever of row 0 below is ready. Before the freeze: the
468-place sweep on the candidate (row 8), Forest Haven played with fusion on Linux, the Linux AppImage from a
contributor's own disc, and `player-check.sh`. Chris decides when to freeze.

## After 0.7.0: catch up with Elliott, then the decompilation

While a release build is running on someone's PC, don't merge code into `main` (docs are fine): the build's check compares `main`'s code with the candidate. One pull request per row, measured on the benchmark (headless, unpaced, from save states) on an x86 PC, each row's
result in PERFORMANCE.md's "Results". Elliott's `windows-release` is cloned at
`~/.codex/work-bluewake-mac-loop/research/Wind-Waker-Recomp`; his reports are in its `docs/status/CURRENT.md`.

| # | Step | Who | Done when |
| --- | --- | --- | --- |
| 0 | **The host's per-call checks** (October 10, the biggest lead). LiquidAzir's profile on the Fold 7 (#93): per retrace, the host and runtime cost 40 M instructions in BlueWake against 7 M in Wind Waker Recomp's build, and `host_direct_can_skip` alone is 23 M. **First part merged October 11: #242** (pdale-boop), 7% fewer instructions on x86, identical game state. Next: the phone and the Deck remeasured on `main`; then the host keeps one "may skip" flag current when its inputs change, which the module reads inline (`cmake/composite/direct_calls.h`), keeping the checks that depend on guest memory. Behind a switch, checked with "plays the same". | Codex; LiquidAzir and jkoehler11 for numbers | #242's share on ARM measured; the inline flag in a pull request with a "Results" row |
| 1 | **Fusion on Mac and Linux.** **Linux in #247** (October 11), from jkoehler11's Deck run on #215: a tenth of the draws, nothing missing, the frame rate unchanged because the game thread sets it. Still to do: Forest Haven played on Linux, and on this Mac. | Codex on the Mac; jkoehler11 or cforain on Linux | Linux merged; the Mac played or left off with a reason |
| 2 | **Merged October 10 after the hand-off (#234, #235).** **Early return dispatch and the watch-list fix** (his `return_ranges.py` from `7aca42a`, and `5edeacc`). The `lfs` half of `7aca42a` is #228. **Ported October 10: #235 and #234**, checked on the prepared lean source (811 of 811 dispatches, repeatable, syntax-clean) and by the tests. Held until the Windows 0.7.0 hand-off, so `main`'s code stays the candidate's while it builds. | Codex; a contributor's build | Merged after the hand-off; 2 to 3% on the game thread, plays the same |
| 3 | **Natives round 5** (the GX SDK's FIFO writers, `native_gx_gen.py`) and **round 4** (animation, collision setup, colour), with his comparison tests. First rerun #179's certification on a lean build with jkoehler11's Linux loader (#194): it should certify now. | Codex; jkoehler11 | Certified counts in the build log; 2 to 6% |
| 4 | **Natives round 7** (libm, collision blocks, rotations, geometry, JASystem) and `cache_ops.py`. | Codex | Certified; about 4 points at Forest Haven |
| 5 | **His upload and vertex changes** (RecompCore `6f52a68` to `400728a`), merged by hand with patch 0157. Check dungeon maps (#74), HD packs and lava colours before and after. | Codex | Merged on `bluewake-next`; maps and packs unchanged |
| 6 | **A native from the decompilation's source:** `__ieee754_fmod` from `e_fmod.c` against his replayed one, on the benchmark ([PERFORMANCE.md](PERFORMANCE.md#the-plan)). | Codex | A "Results" row; a yes or no for doing more |
| 7 | **Android:** every merge-checklist item is done (October 10). LiquidAzir merges `main` (#242), turns hidden symbols off for Android (`NOT ANDROID`, 9 M a retrace on ARM64) and measures the pier from a cool start against the 110 M of Wind Waker Recomp's build. | LiquidAzir | Outset holds 30 when cool: merged as "build it yourself, experimental" |
| 8 | **Crash sweep and benchmark tour:** **#210 merged October 11** (no cost when off). Warp to all 468 places on every release candidate; `bench_tour.py` on `BLUEWAKE_WARP`. | pdale-boop, Codex | A sweep in the next release record |
| 9 | **Flicker** (#136): the two earlier switches are ruled out on 0.7.0 (Muyfa666). Next, `DOL_AURORA_UBERSHADER=0` and a 120 FPS log, then 0.4.0's Smooth Motion against 0.5.0's. | Reporters, then Codex | Cause named |
| 10 | **Linux video** for social media, shot list on #56, once the 0.7.0 AppImage exists. | cforain or jkoehler11; Chris posts | A clip in hand |

## Late October and November: 0.8

Horizon 2, one pull request and one "Results" row each, off by default until tested on its platform: measure the
bookkeeping left after Elliott's work (a fifth of the module's time lands at block starts, by his profile); try
charging cycles per block with interrupts at block boundaries; natives for the loops that cross chunks most; cache
converted static display lists. Then the graphics thread on four-core CPUs (runbook phase 5).

## On the first of every month

Add a row to PERFORMANCE.md's decompilation table from [decomp.dev](https://decomp.dev/zeldaret/tww). Profile the
benchmark, name the hot functions with the decompilation's `symbols.txt`, list the ones matched since last month,
and pick the next natives from them: replayed from the translation where exactness matters, compiled from source
where step 6 showed it pays. When the actor modules pass about 90% of code matched, revisit porting scenes from
source.

