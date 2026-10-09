# What we're doing

The dated plan. Ask "what are we doing on October 10?" and the answer is that day's section below. Owner: Chris.
Updated October 9, 2026.

**The aim for the next versions:** BlueWake runs faster and steadier, the bugs players reported are fixed, Linux
becomes a download, and Android gets to the iPhone app's standard. The ranked backlog is
[PRIORITIES.md](PRIORITIES.md), the speed strategy and its runbook are [PERFORMANCE.md](PERFORMANCE.md), and the
standards everything is held to are in [DIRECTION.md](DIRECTION.md).

## The loop

Every session, by a person or a bot, runs the same five steps:

1. **Read** today's section, then check `main`, open pull requests and new issue replies.
2. **Do** the first row that isn't done and isn't waiting on someone.
3. **Prove** it. Say what ran, on what device, from which commit. A build that compiles is not a game that plays.
4. **Record** it. Mark the row done here, add numbers to [PERFORMANCE.md](PERFORMANCE.md#results), and tell the
   reporters on their issues, in Chris's voice.
5. **Re-plan** when the evidence changes. Unfinished rows move to the next day. Every row has a "done when".

## Speed, in three horizons

The full reasoning is in [PERFORMANCE.md](PERFORMANCE.md#the-plan). In short:

| Horizon | When | What | Gate |
| --- | --- | --- | --- |
| 1. Lean what we have | 0.6.1 and 0.7.0 (days) | Lean block copies, Smooth Motion pacing, the renderer fallback notice | Outset at least 10% faster and the game plays the same |
| 2. Behavior, not cycles | 0.8 (weeks) | Lighter timing, natives that remove round trips, cached display lists | The benchmark, and plays the same |
| 3. Follow the decompilation | Months | Drawing at the GX/J3D API level, then matched scenes ported from source | Each piece checked against the recompilation |

## Saturday, October 10: build day

**Goal:** release 0.6.1 and find out whether lean blocks close the speed gap, both built from one commit of `main`.
What 0.6.1 contains and its checks are in [RELEASE_0.6.1.md](status/RELEASE_0.6.1.md).

| # | Step | Who | Done when |
| --- | --- | --- | --- |
| 1 | **Freeze.** Set `version.json` to 0.6.1 build 6. Merge #106 only if saulob has brought it up to date and CI is green. Write the commit in the release record. | Codex | CI green, commit recorded |
| 2 | **Windows 0.6.1.** `python scripts\windows\build.py DISC` from that commit. Check a controller connected at launch, portable remaps and the window opening in place ([WINDOWS_TASKS.md](WINDOWS_TASKS.md)). | Chris's PC or a tester | Build made, three checks recorded |
| 3 | **Lean build.** Same commit and machine, `build.py DISC --lean-blocks --out build\windows-lean`. Always use a fresh `--out` folder (#59). | Same machine | Build made, build time recorded |
| 4 | **Measure.** Both builds from save states: Outset, the bird scene, Tower room 0; headless and rendered, unpaced; Outset also on four E-cores. Then 30 minutes of play on the lean build: Outset, a cutscene with music, Dragon Roost Cavern, sailing, a fight. | Chris, or pdale-boop and jkoehler11 (offered); LiquidAzir on his phone (#93) | Rows in PERFORMANCE.md "Results" |
| 5 | **Apple.** App-only IPA, PadMint's `audit`, `scripts/release/check_public_assets.sh`. | Codex, on the Mac | Every check passes |
| 6 | **Release.** Draft with `SHA256SUMS` and the notes; Chris publishes; each issue in the release record is told to try it. | Codex prepares, Chris publishes | Release live, issues told |
| 7 | **Dungeon maps.** bessian checks #74 on his save. | bessian | Reply on #74; closed if confirmed |

Not in 0.6.1: lean blocks on by default, the Smooth Motion changes, Android.

## Sunday, October 11: decide lean blocks, start 0.7.0

**Goal:** turn Saturday's numbers into a decision, and land the runtime fixes that make the game feel smoother.

| # | Step | Who | Done when |
| --- | --- | --- | --- |
| 1 | **Lean blocks.** If Outset is at least 10% faster and the play was clean, Chris accepts "plays the same" and decides whether 0.7.0 turns `--lean-blocks` on for Windows and Linux. Under 10%: record it and try lean copies only in the hot chunks (runbook phase 4). | Chris, with Elliott | Written in PERFORMANCE.md "Decisions" |
| 2 | **Renderer fallback notice** (runbook 2a): a `dol_aurora_backend()` getter in RecompCore, one log line and one notice on screen. | Codex | Merged, Linux CI green |
| 3 | **Smooth Motion keeps its frames** on 8 threads or more and comes back in about a second (2b). | Codex; checked on Chris's PC | Merged; the benchmark shows no game-speed loss |
| 4 | **Smooth Motion off by default** on 4 threads or fewer (2c). | Codex | Merged, settings tests pass |
| 5 | **Small bugs:** the Pictobox stick prompt (#191), rumble only to player 1 (#190). | Codex | Merged, Windows CI green |

## October 12 to 16: 0.7.0

**Goal:** a release that is measurably faster than 0.6.1, with Linux as a download and Android buildable.

| # | Step | Who | Done when |
| --- | --- | --- | --- |
| 1 | **The benchmark tour,** `scripts/bench_tour.py` (runbook phase 1), so every later change is one command to measure. | Codex | Merged; two runs of one build agree within 3% |
| 2 | **Lean blocks on by default,** if October 11 said yes; then measure `--lean-memory` as a second step. | Codex; builds by testers | Merged with Chris's approval |
| 3 | **Linux download.** cforain's AppImage (#200), narrowed to the disc chooser and the package; the release check; a Steam Deck run. | cforain, Codex; Chris approves | `check_public_assets.sh` passes |
| 4 | **Linux video** for social media: the Linux build running well with the HD pack and mods, shot list on #56. | cforain or jkoehler11; Chris posts | A clip in hand |
| 5 | **Android** (#93) built with lean blocks and measured on the Fold 7; the label decided (below). | LiquidAzir; Chris decides | Decision recorded; the PR merged or given a short list |
| 6 | **Flicker** (#136): the reporters' two switch runs name the patch. | Reporters, then Codex | Cause named |
| 7 | **0.7.0 build day,** the same steps as October 10. | Chris | Release live |

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
| Performance changes count when the game "plays the same", while correctness fixes keep the strict comparison | Accept: it is how Wind Waker Recomp's builds were judged, and every horizon depends on it | October 11 |
| Linux as a ready-made download in 0.7.0 | Yes, once the package passes the release check (the October 4 exception allows it) | 0.7.0 |
| How Android is offered | "Build it yourself, experimental" in the README and on #93, with no download until its speed and menu match the iPhone app | 0.7.0 |

