# What we're doing

The dated plan. Ask "what are we doing on October 10?" and the answer is that day's section below. Owner: Chris.
Updated October 10, 2026 (evening).

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

## Where things stand on October 10

- **Decided by Chris:** performance changes count when the game plays the same; lean block copies on by default;
  Linux as an AppImage download; Android offered as "build it yourself, experimental". The next release is 0.7.0.
- **Merged today:** lean blocks on by default (#220), the controller zoom (#221, cforain), the AppImage CI for
  SteamOS (#217, cforain), and from October 9's review: the crash fix (#211), the Linux compile fix (#218), FPS
  position (#106) and five more contributor pull requests.
- **In CI, to merge for 0.7.0:** player 1 follows the controller you press, rumble on player 1 only (#222);
  the Pictobox stick (#223); Smooth Motion on 8+ threads and the renderer's name (#224, RecompCore #21); Elliott's
  draw fusion (#225, RecompCore #22); the AppImage first-run setup (#226, cforain).
- **Found today:** Elliott's `windows-release` has three days of speed work BlueWake doesn't: draw fusion (now in
  #225), faster loads, natives rounds 4 to 7 and a much smaller upload. It roughly accounts for the gap to his
  builds. His builder work has to be ported by hand (no shared history). The table is in
  [PERFORMANCE.md](PERFORMANCE.md#the-plan).

## Next: release 0.7.0

**Goal:** 0.7.0 out from one commit of `main`. Its contents and checks are in [RELEASE_0.7.0.md](status/RELEASE_0.7.0.md).

| # | Step | Who | Done when |
| --- | --- | --- | --- |
| 1 | **Merge** #222, #223, #224 (after RecompCore #21 is fast-forwarded onto `bluewake-next`), #225 (after #22), #226, each with green CI. | Codex | All merged |
| 2 | **Freeze.** `version.json` to 0.7.0 build 6; the commit in the release record. | Codex | CI green, commit recorded |
| 3 | **Windows build** on Chris's PC: tell its agent "Pull the latest chrissotraidis/bluewake and follow docs/status/WINDOWS_BUILD_0.7.0.md as a goal loop until its hand-off is done." About 40 minutes of building, then about 20 minutes of Chris playing and looking. | Chris's PC | The zip on the v0.7.0 draft, results in a pull request |
| 4 | **Linux AppImage** from a contributor's own disc: `python3 scripts/linux/build.py DISC`, then `scripts/linux/make_appimage.sh` and `check_appimage_abi.py`. Asked on #56 of pdale-boop, cforain or jkoehler11. | A contributor; Chris receives it privately | The AppImage passes `check_public_assets.sh` |
| 5 | **Mac and Apple.** Ten minutes of Mac play with a controller connected at launch; the app-only IPA, PadMint's `audit`, the release check. | Codex, on this Mac | Every check passes |
| 6 | **Release.** Draft with all assets, `SHA256SUMS` and the notes; Chris publishes; each issue in the release record is told to try it. | Codex prepares, Chris publishes | Release live, issues told |

## After 0.7.0: catch up with Elliott, then the decompilation

One pull request per row, measured on the benchmark (headless, unpaced, from save states) on an x86 PC, each row's
result in PERFORMANCE.md's "Results". Elliott's `windows-release` is cloned at
`~/.codex/work-bluewake-mac-loop/research/Wind-Waker-Recomp`; his reports are in its `docs/status/CURRENT.md`.

| # | Step | Who | Done when |
| --- | --- | --- | --- |
| 1 | **Fusion on Mac and Linux.** Play Forest Haven with `DOL_GX_FUSE=1` on this Mac and on a Linux PC; if both are clean, drop the host's off switch. | Codex on the Mac; pdale-boop or jkoehler11 on Linux | Merged, a "Results" row |
| 2 | **Faster single loads and early return dispatch** (his `7aca42a`: `inline_fp.h`'s `lfs` fast path with its 2^32-pattern test, `return_ranges.py`), and the watch-list fix (`5edeacc`). | Codex; a contributor's build | 4 to 5% on the game thread, plays the same |
| 3 | **Natives round 5** (the GX SDK's FIFO writers, `native_gx_gen.py`) and **round 4** (animation, collision setup, colour), with his comparison tests. First rerun #179's certification on a lean build with jkoehler11's Linux loader (#194): it should certify now. | Codex; jkoehler11 | Certified counts in the build log; 2 to 6% |
| 4 | **Natives round 7** (libm, collision blocks, rotations, geometry, JASystem) and `cache_ops.py`. | Codex | Certified; about 4 points at Forest Haven |
| 5 | **His upload and vertex changes** (RecompCore `6f52a68` to `400728a`), merged by hand with patch 0157. Check dungeon maps (#74), HD packs and lava colours before and after. | Codex | Merged on `bluewake-next`; maps and packs unchanged |
| 6 | **A native from the decompilation's source:** `__ieee754_fmod` from `e_fmod.c` against his replayed one, on the benchmark ([PERFORMANCE.md](PERFORMANCE.md#the-plan)). | Codex | A "Results" row; a yes or no for doing more |
| 7 | **Android:** LiquidAzir rebuilds #93 on `main` (lean blocks, hidden symbols, fusion) and measures Outset on the Fold 7 against the 110 M of Wind Waker Recomp's build. | LiquidAzir | Numbers on #93; merged as experimental when it holds 30 |
| 8 | **Crash sweep and benchmark tour:** #210's stage select once it costs nothing when off; warp to all 468 places on every release candidate; `bench_tour.py` on `BLUEWAKE_WARP`. | pdale-boop, Codex | Merged; a sweep in the next release record |
| 9 | **Flicker** (#136): the reporters' two switch runs name the patch. | Reporters, then Codex | Cause named |
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

