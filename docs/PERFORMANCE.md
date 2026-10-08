# Making BlueWake faster

The plan for frame rate and slowdowns: what players hit, what is ruled out, the levers in order of payoff for
effort, and the fast way to test them. [PRIORITIES.md](PRIORITIES.md) places this work among everything else.
Owner: Chris. Written October 8, 2026; add every result below as it comes in.

## What players hit

Seventeen session logs attached to issues (#137, #159, #155, #61, #97, #65, #80, #107) and the Steam Deck reports,
read on October 8:

| Machine | Where | Game speed when slow | What limits it |
| --- | --- | --- | --- |
| Ryzen 7 2700, 8 cores (#137), Smooth Motion 120 | Outset | 21 to 22 game frames a second | Game thread (27 s), Smooth Motion stepping down (21 s), late frames (14 s), graphics thread (8 s) |
| i7-6500U, 2 cores (#159) | Outset, title | 38 to 65% | Game thread, every slow second |
| i7-4790K, 4 cores (#97) | Title | 79 to 85% | Game thread; the graphics thread 70 to 77% busy |
| Ryzen 7 7840HS, Linux, Vulkan | Outset, bird scene | median 91%, lowest 74% | Game thread |
| Steam Deck (Linux) | Outset | about 22 FPS | Game thread, about 30% short |
| i9-9900K, Ryzen 5800X, Radeon 860M laptop | Title, Outset | 90% and up | Little |
| Mac M4 | Everywhere | 98 to 100% | Nothing |

Almost every slow second is the **game thread**: the translated game code plus the runtime around it. It is too
much work for one core of an older or mobile x86 CPU, and just enough for an Apple M4. Resolution and GPU settings
don't change it. The slow places are Outset and the title flyover, which every player sees first.

## Ruled out or small

- **Denormal floats:** the game sets non-IEEE mode at boot, so flush-to-zero is on all session (jkoehler11, #59).
- **The native entries** (#179): measured on October 2 at about 1% of the game thread together
  ([NATIVE_ENTRIES_2026-10-02.md](status/NATIVE_ENTRIES_2026-10-02.md)). Worth having, not a fix.
- **GPU and resolution:** the logs show the GPU waiting, not working.
- **`-O3` and `-mcpu`:** neutral on the Mac (September).
- **The DSP:** already high-level on every platform.
- **The Linux bird scene at 4%:** an OpenGL ES fallback; with Vulkan its lowest is 74%.

## The levers, in order

### 1. The instruction gap: lean block copies (biggest)

*Evidence:* on the same Galaxy Z Fold 7, LiquidAzir measured BlueWake's game thread at about 158 M instructions
per retrace on Outset, against 110 M for his port built from Wind Waker Recomp's translation (#93). That is 30%,
about what the Steam Deck is short. The most likely difference between the two is the prepaid block copies:
Elliott's builder makes lean copies of almost every block (443,166) and drops the bookkeeping stores of plain loads
and stores, while BlueWake's makes conservative copies that skip nearly every block touching memory. Elliott
measured his set at 26.3 to 23.6 ms of game thread a frame on four cores, and it is in Wind Waker Recomp's
Windows builds.

*Why it isn't on:* on October 2 Elliott's version passed 30,000 function comparisons but differed in BlueWake's
strict boot-route comparison, which demands the exact same guest state at the same cycle: the final PC and 22 of
600 samples differed. Dropping the bookkeeping stores moves when interrupts land by a few cycles, so this is
expected. It is not, on its own, evidence of a gameplay bug.

*The decision this needs (Chris, with Elliott):* for performance changes, accept "plays the same" instead of
"cycle-exact". That means the same milestones on the boot and Outset routes within a few frames, saves that load
and play, cutscene audio, and long sessions without a crash or a soft lock, as Elliott's builds were judged. Keep
the strict comparison for correctness fixes.

*Then:* bring Elliott's block preparation in behind a builder flag (`--lean-blocks`), and run one build each on
Linux and Windows. Measure with the benchmark below, play the routes, and make it the default if both hold.
*Expected:* 10 to 30% less game-thread work. *Effort:* a few days and two builds.

### 2. Code the training skipped is compiled for size (cheapest test)

*Evidence:* the Windows and Linux builders train on the opening and a tour of warps, then compile code the training
never ran as cold: optimized for size, and at `-O1` with tiering. The tour has no cutscenes, combat or bosses, so
that code is the slow kind on x86. The Mac and iPhone build compiles everything at `-O2` and only uses the profile
for the hottest chunks. When the Apple build tried `-O1` everywhere, the iPad dropped from 30 to 26 FPS
([BUILDER.md](BUILDER.md)).

*Test:* build once with `--no-cold` (added October 8: every chunk at `-O2`, profile-guided size optimization
off) and compare the bird scene, a dungeon and Outset with the default build. *Expected:* nothing at Outset, which
is trained; up to about 15% in scenes the training skips. *Effort:* one longer build.

*If it helps:* make it the default, or train on a longer tour that plays cutscenes and fights, or ship a profile
trained by maintainers over a long playthrough. The last also lets players skip the 30-minute training run, which
first-launch builds need anyway ([DIRECTION.md](DIRECTION.md#1-no-game-code-in-any-release)).

### 3. Smooth Motion steps down and stays down (how it feels)

*Evidence:* on #137's eight-core Ryzen at 120 FPS, the game ran at 21 to 22 game frames a second on Outset.
Smooth Motion went from 3 in-between frames to 1, then 0, waiting 3, then 6, then 12 seconds before coming back.
Half of that log's slow seconds kept full game speed: they were the pacing, not the game. The rule was tuned on
four efficiency cores, where in-between frames compete with the game thread. On eight cores they don't. *Change:*
keep in-between frames when the CPU has cores to spare, and come back about a second after a hitch, as Wind
Waker HD Recomp's v0.2.7 does. Runtime only, no game module rebuild. *Expected:* fewer visible drops, not a faster
game.

### 4. Say when the renderer falls back

A missing Vulkan loader on Linux silently meant OpenGL ES at a fraction of the speed. One line on screen and in
the log. Runtime only.

### 5. The graphics thread on four-core CPUs

*Evidence:* the i7-4790K's graphics thread is 70 to 77% busy when the game is slow, and #86's is 92 to 98%; it
competes for the same few cores. *First step:* profile it at the benchmark spot.

### 6. Smooth Motion on two-core CPUs

#159's two cores run the game at 38 to 65% even with Smooth Motion off, so this alone won't fix them. Start it
off on CPUs with four threads or fewer, so it never makes things worse.

The long-term lever is native rendering, replacing the CPU-side conversion of drawing commands, as Wind Waker HD
Recomp does for the Wii U's graphics library. It comes after 1, 2 and 5.

## How to test without the long loop

The slow part is compiling the game module (17 to 45 minutes on a desktop, longer on a laptop). Everything else is
minutes.

- **Measure with a script, not by playing.** `scripts/bench_tour.py` (to write) uses pieces the builders already
  have. It copies the tester's own memory card and places a save on Outset (`scripts/card_set_restart.py`), then
  continues it with the pad script the save acceptance uses, which reaches control in about 833 retraces. Then it
  warps through Windfall, Dragon Roost Cavern, the sea and back (`BLUEWAKE_TEST_WARP`, the training tour's stops)
  with `BLUEWAKE_WALL_PACE=0`, so the game runs as fast as the machine allows. It prints game frames a second at
  each stop, headless and rendered, plus `BLUEWAKE_GUEST_CHECKPOINT_INTERVAL` hashes that show two builds behave
  the same (jkoehler11's check on #178). One unattended run takes a few minutes, needs nothing private shared,
  and two builds on the same machine compare directly.
- **Save states** (`BLUEWAKE_LOAD_STATE`) cover a scene the tour can't reach, like the bird scene. They're made
  from a maintainer's own card and never committed or attached.
- **Runtime and host changes** (items 3, 4 and 6) rebuild the app without recompiling the game module.
- **Build-flag changes** (items 1 and 2) are one unattended build each, on the fastest machine available: pdale-boop's
  i5-12600KF builds in about 17 minutes, jkoehler11's Ryzen 9 5900X on Linux.
- **One change per build,** and its result written below.

## The runbook

Written so an agent can carry it out later in the week without this conversation. Do the phases in order. Each one
ends in a pull request, a row in "Results" below, and an update to [PRIORITIES.md](PRIORITIES.md).

### Before you start

- Read [AGENTS.md](../AGENTS.md), then this file and [PRIORITIES.md](PRIORITIES.md). Check open pull requests and
  new replies on #59, #93, #137, #159 and #179: a contributor may already have answered a question below.
- **Rules that bind every phase.** Branches and pull requests only. Runtime (RecompCore) changes go to
  `chrissotraidis/RecompCore` branch `bluewake-next` by pull request first, then get pinned here and exported as
  `patches/recompcore/NNNN-*.patch` (see #184 for the pattern). A setting goes in both menus
  (`runtime/host/src/settings_menu.cpp` and `windows/src/win_settings.cpp`). Anything that changes gameplay,
  timing or rendering is off by default until it has been tested on the platform it affects. Never commit or
  attach a disc, game files, a card, a save state, a session log or a built module.
- **Hardware.** This Mac (M3 Max) has the Wind Waker ISO at `~/GitHub/bluewakearchive/ref/`, a Mac module at
  `build/macos-main/composite-macos/gGZLE01_recomp.dylib` and the game files at `build/macos-main/game/`. It
  can develop and check the benchmark, but x86 numbers come from Windows or Linux: Chris's PC (Ryzen 7 5700U),
  or contributors (pdale-boop, i5-12600KF, Windows, builds in about 17 minutes; jkoehler11, Ryzen 9 5900X
  and a Steam Deck, Linux). Ask them on the issue for the phase, with exact commands.
- **Stop rules.** Two experiments in a row that tell you nothing new: stop and write down why. A result that
  needs a build you can't run: post the exact commands, move to the next phase, and come back.

### Phase 1: the benchmark tour (`scripts/bench_tour.py`)

*Goal:* one command that measures a build's speed in several places and proves two builds behave the same. Every
later phase uses it.

*What it does:*

1. Copy the tester's card to a scratch folder. Place quest log 1 on Outset with
   `scripts/card_set_restart.py IN OUT sea 44 0`. Never touch the player's own data folder: on Windows set
   `BLUEWAKE_DATA_DIR` to the scratch folder; on the Mac and Linux set `BLUEWAKE_CARD_PATH`, `BLUEWAKE_SRAM` and
   `BLUEWAKE_SETTINGS` (a scratch settings file).
2. Start the app with: `BLUEWAKE_MAX_RETRACES` (the tour's end), `BLUEWAKE_WALL_PACE=0` (as fast as the machine
   can), `BLUEWAKE_PERF_LOG=1`, `DOL_AURORA_FRAME_INTERP=0` (Smooth Motion off, so only the game is measured),
   `BLUEWAKE_PAD_BUTTONS=0x0100`, `BLUEWAKE_PAD_PULSE_ON_TITLE_READY=1`, `BLUEWAKE_PAD_PULSE_LENGTH=2`, and
   `BLUEWAKE_PAD_SCRIPT` set to an A press every 60 retraces from 340 to 1600. That is the continue route from
   `scripts/save_continue_acceptance.sh`, which reaches control (`control-ready retrace=N` in the log) at about 833.
3. Warp through the stops with `BLUEWAKE_TEST_WARP=retrace:stage:room:point,...` (`runtime/host/src/fast_load.c`),
   one stop every `--stop-retraces` (default 1800), starting at 2400. Default stops: `sea:44:0` (Outset),
   `sea:11:1` (Windfall), `M_NewD2:0:0` (Dragon Roost Cavern), `sea:41:0` (Forest Haven), `sea:1:100` (the sea by
   the Fortress). These are in the builders' training tour (`TRAINING_TOUR` in `scripts/windows/build.py`).
   Add `--untrained`, which uses stops the tour skips (pick two dungeons or islands not in `TRAINING_TOUR`, such as
   the Earth and Wind Temples, and confirm in the log that each warp loads). Phase 3 needs it.
4. Set `BLUEWAKE_GUEST_CHECKPOINT_INTERVAL=300` and collect the `[guest-checkpoint]` lines (`main.c`): hashes of
   the CPU state, both memory banks and the aliases.
5. For each stop, skip the first 300 retraces (loading and shader compiles) and report: game frames a second
   (retraces / 2 per wall second), the stop's `[fps-dip]` causes, and the game thread's busy share from `[perf]`.
   Run headless (`BLUEWAKE_RENDERER=headless`, the game alone) and rendered (the default, the game plus the
   graphics thread); `--mode both` runs both.
6. `--state FILE` instead of the card route: `BLUEWAKE_LOAD_STATE`, for scenes the tour can't reach. Chris makes
   two states privately with F5: just before the bird carries Tetra over Outset, and a boss fight.
7. Write `summary.json`. `bench_tour.py compare A.json B.json` prints the change at each stop and whether the
   checkpoints are identical; if not, the first retrace where they differ.

*Check:* on this Mac, two runs of the same build give identical checkpoints and stop speeds within 3%. Every warp
loads (the log's scene lines name each stage). Confirm on Windows that `BlueWake.exe` honours
`BLUEWAKE_RENDERER=headless` and `BLUEWAKE_WALL_PACE=0` (`windows/src/win_entry.c` only sets defaults); if
headless doesn't work there, use rendered only and say so in the script's help. Add a unit test that parses
synthetic log lines and compares two summaries. *Pull request:* the script, its test, and this section updated
with the real stop list. About a day.

### Phase 2: three small runtime fixes

One pull request each. They don't recompile the game module, so each is minutes to build.

**2a. Say when the renderer falls back** (lever 4). After Aurora starts, call `aurora_get_backend()`
(`aurora/aurora.h`). On Linux, if it isn't Vulkan, and on Windows, if it isn't D3D12 or Vulkan, write
`[renderer] fell back to NAME: ...` to the log and show a notice once on screen that names what to install
(Linux: the Vulkan loader, `libvulkan1` / `vulkan-loader`). Host code only (`runtime/host/src`, `linux/src`),
no runtime change. *Check:* Linux CI; ask fehnomenal or jkoehler11 to confirm with Vulkan missing (#107).

**2b. Smooth Motion keeps its frames when the CPU has cores to spare** (lever 3). In RecompCore
`GXRuntime/graphics/aurora/lib/gfx/frame_interp.cpp`, `slow_game` and `pace_steps` drop in-between frames
whenever the game runs slow, and after a slow drop wait 3 s, then double up to `kCalmMostSlow` (2 minutes). Change
two things. With `std::thread::hardware_concurrency() >= 8`, a slow game alone no longer drops in-between frames;
GPU overloads still do. And after a slow drop, come back after about 1 s of calm, not 3 s doubling. Keep the
behavior for fewer cores, where the comment there shows interpolation does slow the game (four E-cores: 24 to 27
game frames a second against 30). *Check:* the benchmark, rendered, with `DOL_AURORA_FRAME_INTERP=1` and 120
FPS on an 8-thread-or-more PC. Before and after: game frames a second must not fall by more than 2%, and the count
of `[interp-pace]` drops should fall. Windows build and a short play on Chris's PC; a row in
`docs/WINDOWS_TASKS.md`.

**2c. Smooth Motion starts off on four threads or fewer** (lever 6). Only as the default for a player who never
set it, in both menus, with a `[smooth-motion] off by default: N threads` log line. *Check:* the settings tests
(`tests/settings_state_test.cpp`) and Windows CI.

### Phase 3: code the training skipped (`--no-cold`, lever 2)

*Goal:* find whether compiling untrained code for size costs real speed. If jkoehler11 or pdale-boop answered on
#59, start from their numbers.

1. On one x86 machine and one commit: build normally, keep a copy of the app folder, then build with `--no-cold`
   (`scripts/windows/build.py DISC --no-cold` or `scripts/linux/build.py`). The second build reuses translation
   and training; only the compile repeats.
2. Run the benchmark on both: the default stops, `--untrained`, and the bird scene state if Chris has made it.
3. *Read it:* the trained stops should not move. If the untrained stops or the bird scene are 5% or more faster,
   it counts.
4. *If it counts,* choose the cheaper fix. One option is to add the slow untrained places to `TRAINING_TOUR`
   (longer training, same compile time), then rerun the comparison and keep the tour change if it recovers most of
   the gain. The other is to make `--no-cold` the default (about 25 more minutes of compiling). Record the build
   times too. *If it doesn't count,* write that down and leave the option off.

### Phase 4: lean block copies (lever 1, the big one)

*Goal:* the roughly 30% instruction gap.

*Background:* `scripts/windows/fast_blocks.py` makes prepaid copies of translated blocks. Elliott Tate's original
transform was imported in `0db6d35` (his authorship). On October 2, `42af7ac` made it conservative: it keeps
every PC store and leaves out any block with deadline refunds. The original differed in BlueWake's strict
boot-route comparison (final PC and 22 of 600 samples). Keeping the PC stores alone didn't fix it, so the
difference is in the refund blocks, which shift when interrupts land
([evidence](status/FORK_RECONCILIATION_EVIDENCE_2026-10-02.md#generic-prepaid-block-qualification)).
`scripts/windows/lean_memory.py` and `native_entries.py` (Elliott, `552ce1f`) only work on the original's
copies; that is why they change 0 accesses and certify 0 of 15 today (#179).

1. **Data first.** Read LiquidAzir's answer on #93 (his two Android builds, same spot: game speed, `[chassis]`
   lines, options). If his Wind Waker Recomp build differs in ways besides the block copies, note them here.
2. **The flag.** Add `--lean-blocks` to `scripts/windows/build.py` and `scripts/linux/build.py`. It runs the
   original transform as a mode of `fast_blocks.py` (restore its logic from `git show 0db6d35:scripts/windows/fast_blocks.py`,
   keeping Elliott's credit with a `Co-authored-by:` line), then `lean_memory.py` and `native_entries.py`.
   Off by default. It joins the source fingerprint and the training fingerprint, like the other options.
   Synthetic tests in the style of `tests/test_windows_prepared_cache.py`. *Check:* on prepared source,
   `native_entries.py` should report 15 of 15 certified and `lean_memory.py` a nonzero count. If not, stop and
   find out why before building.
3. **Build and measure.** On one x86 machine and commit: the default build and a `--lean-blocks` build. Run the
   benchmark on both, headless and rendered. On Linux, also `perf stat -e instructions` over the Outset stop.
   *Read it:* game frames a second at Outset, and instructions a retrace if measured.
4. **Does it play the same?** The checkpoints will differ (that is the known divergence). Check instead, with the
   `--lean-blocks` build:
   - the benchmark reaches every stop, in the same order, both modes;
   - the cold boot route's milestones (the `[boot-milestone]` lines, such as `file-select` and `play-scene`, and
     `control-ready`) land within 30 retraces of the default build;
   - `scripts/save_continue_acceptance.sh` logic: save, quit and reload work;
   - a 30-minute play on Windows by a tester: Outset, a cutscene with music, Dragon Roost Cavern, sailing, a fight.
     No crash, no soft lock, no new `[audio-lost]`, no fatal lines; the session log kept privately;
   - optionally, the 30,000-case function fixture from October 2, rebuilt optimized on x86.
5. **The gate.** If Outset is at least 10% faster and every check passes, open a pull request that turns
   `--lean-blocks` on by default for Windows and Linux. **It merges only with Chris's approval** of the acceptance
   standard (below). If it is under 10%, or a check fails, record it and stop. Bisecting the refund forms chunk by
   chunk is only worth it if Chris requires cycle-exact behavior.

### Phase 5 and later

- **Graphics thread** (lever 5): run the benchmark rendered on a four-core CPU, profile the graphics worker at
  Outset (`perf record -g` on Linux, Windows Performance Recorder on Windows), and name the hot paths in command
  conversion and vertex decoding. Ask the #97 reporter (i7-4790K) or jkoehler11's Steam Deck.
- **A maintainer-trained profile:** once phases 3 and 4 settle the build options, train once on a long tour that
  includes cutscenes and fights, with a compiler every supported toolchain can read (LLVM 18 format). Ship it
  with the builder so players skip training. That saves about 30 minutes a build and is what first-launch builds
  need ([DIRECTION.md](DIRECTION.md#1-no-game-code-in-any-release)).

### After each phase

Add a row to "Results". Update the plan in [PRIORITIES.md](PRIORITIES.md). When a change ships, tell the people
whose reports it answers (#137, #159, #86, #59, the Steam Deck users on #107), with the numbers.

## Decisions

| Decision | Status | Who |
| --- | --- | --- |
| Performance changes are accepted when the game "plays the same" (phase 4, step 4), not only when cycle-exact. Correctness fixes keep the strict comparison. | **Proposed October 8, waiting for Chris.** Phases 1 to 4 can build and measure behind flags without it; only turning `--lean-blocks` on by default needs it. | Chris, with Elliott |
| `--no-cold` or a longer training tour | After phase 3 | Chris |

## Results

| Date | Change | Machine | State | Before | After | Same behavior? |
| --- | --- | --- | --- | --- | --- | --- |
