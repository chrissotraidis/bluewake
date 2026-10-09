# Making BlueWake faster

How BlueWake gets faster, in three steps of growing size, and how to prove each step. [GOAL_LOOP.md](GOAL_LOOP.md)
says which step is being done on which day. Owner: Chris. Updated October 9, 2026; add every result to "Results".

## The plan

Almost every slow second in players' logs is the **game thread**: the translated game code plus the bookkeeping
around it. Resolution and GPU settings don't change it. On the same phone at the same spot on Outset, Wind Waker
Recomp's translation runs at 100% speed with 110 M instructions a retrace, and BlueWake 0.6.0 at 69% with 173 M.
Turning on every other BlueWake option moved that by nothing. So the plan is to take work off the game thread,
biggest cut first, and to measure every change the same way.

| Horizon | When | Change | Expected | Gate |
| --- | --- | --- | --- | --- |
| **1. Lean what we have** | 0.6.1 and 0.7.0, days | Lean block copies (`--lean-blocks`). Smooth Motion keeps its frames on big CPUs and recovers in a second, starts off on four threads or fewer. Say when the renderer falls back. | 10 to 30% less game-thread work; fewer visible drops | Outset at least 10% faster, and the game plays the same |
| **2. Behavior, not cycles** | 0.8, weeks | Charge cycles per block and deliver interrupts at block or function boundaries. Natives for the loops that cross chunks most (collision, J3D drawing, particles). Cache converted static display lists. The graphics thread on four-core CPUs. | Not measured yet; one native that removed round trips saved 21.9% in a heavy view | The benchmark, and plays the same; off by default until tested |
| **3. Follow the decompilation** | Months | Drawing at the GX/J3D API level through Aurora, then matched scenes ported from source | The largest; the HD project's draw batching alone was worth 47 to 53% | Each piece checked against the recompilation |

**The decompilation ratchet.** The Wind Waker decompilation ([zeldaret/tww](https://github.com/zeldaret/tww)) has
79.1% of its code matched (October 9) and keeps growing. A function compiled from its source costs 6 to 30 times
fewer host instructions than the same function translated. So BlueWake can get faster as the decompilation does, a
piece at a time and without a rewrite: each month, profile the benchmark, list the hot functions the decompilation
has matched, and replace the ones that cost the most (above all those that cross chunks often), each with a
comparison test against the translated version. The recompilation stays the reference and the fallback throughout.
Wind Waker HD Recomp uses the same decompilation the same way, to name and find its hot code.

**What we take from Wind Waker HD Recomp** (details under "Background"): it emulates behavior instead of cycles,
keeping timing at vsync, flips and GPU completion; it draws at the graphics API level instead of parsing a command
stream; and it measures fixed scenes from save states. Horizons 2 and 3 are those moves for the GameCube. Its HD art
and its Wii U renderer we can't take.

**Rules for every change.** One change per build. Measured from save states with the same states on both builds. A
row in "Results". Off by default until tested on the platform it affects. Performance changes are judged by "plays
the same"; correctness fixes keep the strict cycle-exact comparison.

## Decisions

| Decision | Status | Who |
| --- | --- | --- |
| Performance changes are accepted when the game "plays the same" (phase 4, step 4), not only when cycle-exact. Correctness fixes keep the strict comparison. | **Proposed October 8, waiting for Chris.** Phases 1 to 4 can build and measure behind flags without it; only turning `--lean-blocks` on by default needs it. | Chris, with Elliott |
| `--no-cold` or a longer training tour | **Decided by the numbers, October 8: neither.** `--no-cold` is slower; the tour stays. | |
| `--lean-blocks` on by default for Windows and Linux in 0.7.0 | **Waiting for the October 10 numbers;** decided October 11 ([GOAL_LOOP.md](GOAL_LOOP.md)). Needs the first decision. | Chris |

## Results

| Date | Change | Machine | State | Before | After | Same behavior? |
| --- | --- | --- | --- | --- | --- | --- |
| Oct 8 | `--no-cold` | i5-12600KF, Windows (pdale-boop) | Bird scene, headless, unpaced | 82.1 / 82.7 retraces a second | 80.9 / 80.9 (−1.8%) | Yes, identical checkpoints |
| Oct 8 | `--no-cold` | i5-12600KF | Outset; Tower room 0; Wind and Earth Temples; Dragon Roost; Savage Labyrinth; Molgera; Gohma | | −1.0 to −3.5% (Molgera +0.3%) | Yes |
| Oct 8 | `--no-cold` | i5-12600KF, 4 E-cores | Tower room 0 | 46.7 | 45.3 (−2.9%) | Yes |
| Oct 8 | `--no-cold` | Ryzen 9 5900X, Linux (jkoehler11) | Bird scene, 2,000 retraces unpaced | 26.27 s | 26.86 s (−2.2%) | Yes, identical blocks and checkpoints |
| Oct 8 | Wind Waker Recomp's lean copies vs BlueWake 0.6.0 (reference) | Galaxy Z Fold 7 (LiquidAzir) | Outset pier, cool | BlueWake 69%, 173 M instr./retrace | WWR port 100%, 110 M | Different builds |

## The runbook

Written so an agent can carry it out without any earlier conversation. [GOAL_LOOP.md](GOAL_LOOP.md) schedules the
phases. Each one ends in a pull request, a row in "Results" above, and the row marked done in GOAL_LOOP.md.

### Before you start

- Read [AGENTS.md](../AGENTS.md), then [GOAL_LOOP.md](GOAL_LOOP.md) and this file. Check open pull requests and
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

**2a. Say when the renderer falls back** (lever 4). After Aurora starts, read the backend it actually chose: the `backend` field of the `AuroraInfo` that
`aurora_initialize` returns in RecompCore's `dol_aurora_initialize` (`GXRuntime/backends/aurora/aurora_backend.cpp`),
which needs a small getter there, for example `dol_aurora_backend()`. `aurora_get_backend()` is not it: it returns the
requested backend (usually `BACKEND_AUTO`). On Linux, if it isn't Vulkan, and on Windows, if it isn't D3D12 or Vulkan, write
`[renderer] fell back to NAME: ...` to the log and show a notice once on screen that names what to install
(Linux: the Vulkan loader, `libvulkan1` / `vulkan-loader`). The getter goes to RecompCore; the message is host code (`runtime/host/src`, `linux/src`). *Check:* Linux CI; ask fehnomenal or jkoehler11 to confirm with Vulkan missing (#107).

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

**Done October 8: no gain, the option stays off** (lever 2 under "Background", and "Results"). Had it helped, the
next steps were a longer training tour with cutscenes and fights, or a maintainer-trained profile; the profile is
still worth making for first-launch builds (phase 5).

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

1. **Data first. Done October 8:** LiquidAzir's phone numbers (lever 1 under "Background") point at the copies alone.
2. **The flag. Done October 9.** `--lean-blocks` in both builders runs `fast_blocks.py --lean`: Elliott's original
   copy logic from `0db6d35`, with today's exclusions for the certified native leaves, under its own marker so a
   chunk prepared in one mode is refused by the other. Off by default; it joins the source, receipt, training and
   provenance records. `lean_memory.py` accepts lean copies, and `--lean-memory` now requires `--gather-pipe`
   (its accesses call that header's helpers). Checked on the October 4 translated source on the Mac: 443,166
   copies in 813 chunks (the October 2 donor count exactly; the conservative mode still gives 197,459 in 812),
   then the builders' order (inline helpers, lean copies, direct calls with 235,556 calls, lean memory with
   501,016 accesses), and every one of the 813 chunks passes a syntax-only compile. Not checked: a full compile,
   a run, and `native_entries.py` on a source with the Windows native preparations.
3. **Build and measure, in this order**, on one x86 machine and commit, each against the default build:
   - `--lean-blocks` alone. This is what Wind Waker Recomp ships, and it should close most of the gap.
   - `--lean-blocks --lean-memory`, a second step only if the first holds.
   - `--native-entries` with both: its log says how many of the 15 certify. With jkoehler11's Linux loader
     (#194), the comparison tests can be rerun against that module.

   Run the benchmark on each, headless and rendered, from the same states. On Linux, also
   `perf stat -e instructions` over the Outset state. LiquidAzir offered to run any lean build on his phone at
   Outset and at sea (#93). *Read it:* game frames a second at Outset, and instructions a retrace.

   **Also measure the cost of size.** Lean copies make the translated source 64% bigger (1.03 GB to 1.70 GB on the
   October 4 source; the conservative copies add 10%), and `--no-cold` showed that a module only 4% bigger costs 2 to
   3% on cores with small caches. So run the Outset state on four E-cores too (pdale-boop's slow-CPU stand-in), and
   record the compile and training times of both builds. If the E-cores gain much less than the P-cores, or the
   build gets too long for players, the follow-up is lean copies only in the chunks the training ran hot
   (`fast_blocks.py` given the profile's hot list, as the builders' tiering already reads it), which keeps most of the
   speed and little of the size.
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
   standard ("Decisions" above). If it is under 10%, or a check fails, record it and stop. Bisecting the refund forms chunk by
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

Add a row to "Results". Mark the row done in [GOAL_LOOP.md](GOAL_LOOP.md) and update [PRIORITIES.md](PRIORITIES.md). When a change ships, tell the people
whose reports it answers (#137, #159, #86, #59, the Steam Deck users on #107), with the numbers.

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
- **Measure from save states made once.** Continuing a card is not repeatable: two runs of one build already
  differ in `[guest-checkpoint]` by retrace 600, before the save loads (pdale-boop, #59). So the tour first makes
  a state from the card (`BLUEWAKE_SAVE_STATE=PATH@2390`, once control is reached), and every measurement starts
  from states (`BLUEWAKE_LOAD_STATE`). From states, both runs of a build and both builds gave identical checkpoints
  at all nine of pdale-boop's spots. States also cover what warps can't reach: the bird scene, a boss. They're
  made from a maintainer's or tester's own card and never committed or attached.
- **Slow-CPU stand-in:** pin the game to the efficiency cores of a hybrid Intel CPU (an i5-12600KF's E-cores run
  it at 55% of a P-core). They show code-layout effects more clearly than a fast core.
- **Benchmarking a Windows PC remotely:** start rendered runs in the desktop session (a scheduled task with `/IT`);
  from SSH they run in session 0, without a real display, at 40 to 52 retraces a second whatever the scene.
- **Runtime and host changes** (levers 3, 4 and 6) rebuild the app without recompiling the game module.
- **Build-flag changes** (levers 1 and 2) are one unattended build each, on the fastest machine available: pdale-boop's
  i5-12600KF builds in about 17 minutes, jkoehler11's Ryzen 9 5900X on Linux.
- **One change per build,** and its result written in "Results".

## Background

The evidence behind the plan, kept here so the sections above can stay short.

### What players hit

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

### Ruled out or small

- **Denormal floats:** the game sets non-IEEE mode at boot, so flush-to-zero is on all session (jkoehler11, #59).
- **The native entries** (#179): measured on October 2 at about 1% of the game thread together
  ([NATIVE_ENTRIES_2026-10-02.md](status/NATIVE_ENTRIES_2026-10-02.md)). Worth having, not a fix.
- **GPU and resolution:** the logs show the GPU waiting, not working.
- **`-O3` and `-mcpu`:** neutral on the Mac (September).
- **The DSP:** already high-level on every platform.
- **The Linux bird scene at 4%:** an OpenGL ES fallback; with Vulkan its lowest is 74%.

### The levers and their evidence

The runbook's phases refer to these numbers.

**1. The instruction gap: lean block copies.** On the same Galaxy Z Fold 7 (LiquidAzir, October 8, Outset's pier,
started cool, `simpleperf`), the Wind Waker Recomp port runs at 100% speed with 110 M instructions a retrace and
BlueWake 0.6.0 at 69% with 173 M. Turning on direct calls, the gather pipe and the natives moved BlueWake from
172.7 M to 173.2 M. His WWR build uses Elliott's lean block copies and neither lean memory nor native entries, so
the copies are the gap. Elliott's builder makes lean copies of almost every block (443,166) and drops the
bookkeeping stores of plain loads and stores; BlueWake's conservative copies skip nearly every block that touches
memory. *Why it isn't on:* on October 2 Elliott's version passed 30,000 function comparisons but differed in
BlueWake's strict boot-route comparison (the final PC and 22 of 600 samples). Dropping the bookkeeping stores moves
when interrupts land by a few cycles, so this is expected and is not, on its own, evidence of a gameplay bug.

**2. Code the training skipped (tested, no gain).** `--no-cold` compiles every chunk at `-O2`. On October 8 it was
1 to 3.5% *slower* everywhere, the bird scene and untrained dungeons included, on an i5-12600KF (pdale-boop) and a
Ryzen 9 5900X (jkoehler11), with identical checkpoints. The bigger module crowds the instruction cache. It stays
off and the training tour stays as it is.

**3. Smooth Motion steps down and stays down.** On #137's eight-core Ryzen at 120 FPS, Smooth Motion went from 3
in-between frames to 1, then 0, waiting 3, then 6, then 12 seconds before coming back. Half of that log's slow
seconds kept full game speed: they were the pacing, not the game. The rule was tuned on four efficiency cores,
where in-between frames compete with the game thread; on eight cores they don't. Wind Waker HD Recomp's v0.2.7
comes back about a second after a hitch. *Expected:* fewer visible drops, not a faster game.

**4. Say when the renderer falls back.** A missing Vulkan loader on Linux silently meant OpenGL ES: the bird scene
ran at 4% instead of 74% or more.

**5. The graphics thread on four-core CPUs.** The i7-4790K's graphics thread is 70 to 77% busy when the game is
slow, and #86's is 92 to 98%; it competes for the same few cores.

**6. Smooth Motion on two-core CPUs.** #159's two cores run the game at 38 to 65% even with Smooth Motion off, so
starting it off only keeps it from making things worse.

### Why Wind Waker HD Recomp is fast

From its own [how-it-works](https://github.com/ZeldaWWHDRecomp/ZeldaWWHDRecomp/blob/main/docs/how-it-works.md) and
[performance](https://github.com/ZeldaWWHDRecomp/ZeldaWWHDRecomp/blob/main/docs/performance.md) notes:

- **It emulates behavior, not cycles.** Translated functions are plain C functions. Timing lives at the API level:
  vsync, flips, GPU completion and thread priorities. There is no per-instruction cycle accounting. Each guest
  thread is a host thread, and preemption happens at function entry.
- **Graphics at the API level.** It implements the Wii U's GX2 calls directly on Metal and Vulkan, so no command
  stream is encoded and then parsed again. Batching its draws was worth 47 to 53% in its own test.
- **Release builds at -O3,** high-resolution timers on Windows, and a fixed-scene benchmark from save states.
- **It uses the decompilation for knowledge:** about 14,000 HD functions are named by matching them to the
  GameCube decompilation, which is how it finds, for example, the camera code it interpolates.

BlueWake emulates the GameCube's cycle timing (each block charges cycles, stores its pc, and checks deadlines) and
converts the game's raw GX command stream on the CPU. On the one function measured on September 22
(`J3DSys::reinitTevStages`), the translation is 5.8 emitted statements per guest instruction: 21% are pc stores
and 9% cycle bookkeeping. Compiled from the decompilation's source, the same function is 0.91 host instructions
per guest instruction, six to thirty times cheaper ([CURRENT.md](status/CURRENT.md), September 22).

### Where the decompilation is

| Date | Code matched | Main executable (engine, SDK) | Actor modules (RELs) | Functions matched |
| --- | --- | --- | --- | --- |
| August 13 | about 80% | | | |
| October 9 | 79.1% | 87.1% of code, 95.8% of functions | 73.1% | 32,848 of 39,324 (83.5%) |

From [decomp.dev](https://decomp.dev/zeldaret/tww). Add a row each month. The August figure is from
[DECISIONS.md](status/DECISIONS.md); it was measured differently.

### Route B, the source port BlueWake already tried

In August and September BlueWake built a native port from the decompilation's source alongside the recompilation
("route B"; its records are [DECISIONS.md](status/DECISIONS.md) and `docs/status/ROUTE_B_*.md`; the code itself
is not in this repository). It drew the original Nintendo logo
through Aurora and compiled most of the process layer. It stopped on porting effort, not speed: the source assumes
32-bit big-endian types and pointers, and Dusklight (the Twilight Princess port) needed about 1,500 `TARGET_PC`
conditionals across 293 files. The recompilation shipped instead.
