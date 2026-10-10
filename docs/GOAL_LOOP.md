# What we're doing

The dated plan. Ask "what are we doing on October 10?" and the answer is that day's section below. Owner: Chris.
Updated October 10, 2026.

**The aim for the next versions:** BlueWake runs faster and steadier, the bugs players reported are fixed, Linux
becomes a download, and Android gets to the iPhone app's standard. Most of that work is already being done by
contributors, so bringing it in, measured, comes first. The ranked backlog is [PRIORITIES.md](PRIORITIES.md), the
speed strategy and its runbook are [PERFORMANCE.md](PERFORMANCE.md), and the standards everything is held to are in
[DIRECTION.md](DIRECTION.md).

## The loop

Every session, by a person or a bot, runs the same five steps:

1. **Read** today's section, then check `main`, open pull requests and new issue replies.
2. **Do** the first row that isn't done and isn't waiting on someone. Before writing anything new, check whether an
   open pull request already does it: reviewing and merging measured contributor work beats starting over.
3. **Prove** it. Say what ran, on what device, from which commit. A build that compiles is not a game that plays.
4. **Record** it. Mark the row done here, add numbers to [PERFORMANCE.md](PERFORMANCE.md#results), and tell the
   reporters on their issues, in Chris's voice.
5. **Re-plan** when the evidence changes. Unfinished rows move to the next day. Every row has a "done when".

## Speed, in three horizons

The full reasoning is in [PERFORMANCE.md](PERFORMANCE.md#the-plan). In short:

| Horizon | When | What | Gate |
| --- | --- | --- | --- |
| 1. Lean what we have | 0.6.1 and 0.7.0 (days) | Lean block copies, hidden symbols in the game module, Smooth Motion pacing, the renderer fallback notice | Faster on the benchmark, and the game plays the same |
| 2. Behavior, not cycles | 0.8 (weeks) | Lighter timing, natives that remove round trips, cached display lists | The benchmark, and plays the same |
| 3. Follow the decompilation | Months | Drawing at the GX/J3D API level, then matched scenes ported from source | Each piece checked against the recompilation |

## Where things stand on October 10

- **Merged October 9 and 10:** a crash fix for `pc=0x8180FFF0` (#211), the FPS counter's position (#106, saulob),
  rebuilding into the same folder works again (#209), Linux compiles the game twice as fast and runs faster (#218),
  the Linux test loader (#194), Linux build packages (#207), the lean-blocks results (#208) and the Steam Deck
  profiling guide (#216). All by contributors; `main` passes the audit and the builder tests.
- **Measured:** lean blocks are 5 to 8% faster on a fast core, 8 to 12% on slow and four-core CPUs, with 9.9% fewer
  instructions, and 15½ minutes of play went cleanly (pdale-boop, #208). Hidden symbols on Linux cut 7.8% of
  instructions and gave 9 to 10% on slow cores, with identical checkpoints (#218). Together that is about a sixth
  less work on Linux. The phone gap is 36%, so lean blocks alone don't explain it.
- **Waiting for review:** the Linux AppImage (#200), its CI (#217) and controller zoom (#213), whose CI was approved
  October 10; the stage select (#210); Android (#93).

## Saturday, October 10: release 0.6.1

**Goal:** 0.6.1 out, built from one commit of `main`. Its contents and checks are in
[RELEASE_0.6.1.md](status/RELEASE_0.6.1.md). The lean build planned for today isn't needed: pdale-boop measured it.

| # | Step | Who | Done when |
| --- | --- | --- | --- |
| 1 | **Freeze.** Set `version.json` to 0.6.1 build 6 and write the commit in the release record. | Codex | CI green, commit recorded |
| 2 | **Windows 0.6.1.** `python scripts\windows\build.py DISC` from that commit, started from a terminal on the desktop (from SSH or a background shell, Windows compiles on the slow cores). Check a controller connected at launch, portable remaps, the window opening in place and the FPS counter's position ([WINDOWS_TASKS.md](WINDOWS_TASKS.md)). | Chris's PC or pdale-boop | Build made, checks recorded |
| 3 | **Mac check.** The crash fix (#211) is shared code: build the Mac app from the commit and play ten minutes, with a controller connected at launch. | Codex, on this Mac | Recorded in the release record |
| 4 | **Apple.** App-only IPA, PadMint's `audit`, `scripts/release/check_public_assets.sh`. | Codex, on the Mac | Every check passes |
| 5 | **Release.** Draft with `SHA256SUMS` and the notes; Chris publishes; each issue in the release record is told to try it. | Codex prepares, Chris publishes | Release live, issues told |
| 6 | **Dungeon maps.** bessian confirmed on October 9. | | Done: #74 closed |

## Sunday, October 11: decide lean blocks, fix what players feel

| # | Step | Who | Done when |
| --- | --- | --- | --- |
| 1 | **Lean blocks.** Chris accepts "plays the same" and decides whether 0.7.0 turns `--lean-blocks` on for Windows, Linux and Android. The recommendation is yes: the slow CPUs that players complain about gain the most, play was clean, and the longer compile is offset by #202 and #218. | Chris, with Elliott | Written in PERFORMANCE.md "Decisions" |
| 2 | **Hidden symbols on the Mac and iPhone.** #218 is ELF only. Build the Mac module once with `hidden_externs.h` and compare instructions at Outset, headless. Under 3%: record it and stop. | Codex, on this Mac | A row in "Results" |
| 3 | **Renderer fallback notice** (runbook 2a). | Codex | Merged, Linux CI green |
| 4 | **Smooth Motion** keeps its frames on 8 threads or more and comes back in about a second (2b); starts off on 4 threads or fewer (2c). | Codex; checked on Chris's PC | Merged; no game-speed loss on the benchmark |
| 5 | **Controllers:** player 1 goes to the first controller that presses a button while player 1 is idle, so an adapter's empty ports can't take it (#155); the Pictobox stick prompt (#191); rumble only to player 1 (#190). | Codex | Merged, Windows CI green |

## October 12 to 16: 0.7.0

**Goal:** a release that is measurably faster than 0.6.1 on slow CPUs, with Linux as a download and Android
buildable.

| # | Step | Who | Done when |
| --- | --- | --- | --- |
| 1 | **Lean blocks on by default,** if October 11 said yes. | Codex | Merged with Chris's approval |
| 2 | **Android's gap.** LiquidAzir rebuilds #93 on `main` (its module is ELF, so it gets #218's hidden symbols) with `--lean-blocks`, and measures instructions a retrace at Outset against the 110 M of the Wind Waker Recomp build. | LiquidAzir | Numbers on #93 |
| 3 | **Android merged** as "build it yourself, experimental" once it holds 30 FPS at Outset when cool. | Codex reviews; Chris decides | Merged, or a short list on #93 |
| 4 | **Linux download.** #217 (AppImage CI with a glibc 2.39 ceiling, so it runs on SteamOS), then #200 (first-run disc setup), then a contributor builds the AppImage from their own disc; the release check; a Steam Deck run with lean blocks and #218 (#215). | cforain, jkoehler11; Chris approves | `check_public_assets.sh` passes, Deck numbers on #215 |
| 5 | **Crash sweep.** #210's stage select merged once it costs nothing when off; then every release candidate warps to all 468 places and any crash becomes an issue. #211 was found this way. | pdale-boop, Codex | Merged; a sweep recorded in the release record |
| 6 | **The benchmark tour** on #210's `BLUEWAKE_WARP` instead of the card route (runbook phase 1). | Codex | Two runs of one build agree within 3% |
| 7 | **Controller zoom** (#213). The fast right-stick camera uses up and down to tilt the view, so the follow camera lost the C-stick's distance control; #213 puts it on R3 plus the stick, sharing the mouse wheel's zoom. Review with its CI, check it on Windows, merge off by default. | Codex, cforain | Merged, a row in WINDOWS_TASKS.md |
| 8 | **Flicker** (#136): the reporters' two switch runs name the patch. | Reporters, then Codex | Cause named |
| 9 | **Linux video** for social media, shot list on #56. | cforain or jkoehler11; Chris posts | A clip in hand |
| 10 | **0.7.0 build day,** the same steps as October 10. | Chris | Release live |

## Late October and November: 0.8

Horizon 2, one pull request and one "Results" row each, off by default until tested on its platform: measure the
bookkeeping left after lean blocks; try charging cycles per block with interrupts at block boundaries; natives for
the loops that cross chunks most (collision and J3D drawing first, named with the decompilation); cache converted
static display lists. Then the graphics thread on four-core CPUs (runbook phase 5).

## On the first of every month

Add a row to PERFORMANCE.md's decompilation table from [decomp.dev](https://decomp.dev/zeldaret/tww). Profile the
benchmark, list the hot functions the decompilation has matched since last month, and pick the next natives from
them. When the actor modules pass about 90% of code matched, revisit porting scenes from source.

## Decisions waiting for Chris

| Decision | Recommendation | Needed by |
| --- | --- | --- |
| Performance changes count when the game "plays the same", while correctness fixes keep the strict comparison | Accept: lean blocks passed it on Windows (#208), and every horizon depends on it | October 11 |
| `--lean-blocks` on by default in 0.7.0 | Yes, for Windows, Linux and Android | October 11 |
| Linux as a ready-made download in 0.7.0 | Yes, as an AppImage once it passes the release check (the October 4 exception allows it) | 0.7.0 |
| How Android is offered | "Build it yourself, experimental" in the README and on #93, with no download | 0.7.0 |

