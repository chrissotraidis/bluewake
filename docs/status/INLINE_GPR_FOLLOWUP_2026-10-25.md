# Inline register saves after #480: what helped, what didn't (October 24–25, 2026)

The question: #480 makes the inline `_savegpr`/`_restgpr` eligibility check one range test instead of one per
register, and measured +2.4% on a Steam Deck. Does that hold with the training profile held fixed, and is there more
to take in the same place?

The short answer: #480's gain is real and comes from the code (+1.1 to +1.6% here). Writing the frame directly once it
is proven adds about 1 to 2.4%. Inlining the REL modules' calls, which were never inlined, adds about 5.4% more, but
only in a build trained on that source: with `main`'s profile the same source is 3.4% *slower* than `main`. The full
stack, trained, is +7.9% and +8.2% over `main` on the benchmark tour in two separate sets of rounds, with identical
guest checkpoints.

## How it was measured

- **Builds.** All in the Linux CI container (`--ci-container`, Clang 18, `-march=x86-64-v3`, default switches,
  `--portable`) from `main` `211abfd`, on an i5-12600KF (Arch). "Trained" means a normal build with its own local
  training. "With `main`'s profile" means the source was rebuilt with `--host-only` in the folder of a trained `main`
  build, which keeps that folder's profile whatever source has changed.
- **Profile match.** Checked rather than assumed: sample chunks were recompiled with `-Wprofile-instr-out-of-date`
  back on (the builder passes `-Wno-profile-instr-out-of-date`). The changed builds report 1 or 2 mismatched
  functions of 45 to 93 per chunk (the rewritten helpers), so every other function keeps its counts.
- **The benchmark tour.** `scripts/bench_tour.py` from #459 (with #450's `--setup-state`), headless, on an i5-6500
  (Debian 13, all four cores): Outset, Windfall, Dragon Roost Cavern, Forest Haven and the sea by the Fortress,
  1,800 retraces each with the first 300 skipped. One setup state, made once from a copy of a card, is loaded by
  every run, so every build measures the same guest state. Three rounds unless noted, with the order of the builds
  rotated each round. Each run took about 80 seconds.
- **Three kept states** (Outset, the bird, Tower of the Gods by boat; 2,400 to 2,800 retraces, headless, unpaced,
  `perf stat` instruction counts) for the first comparisons, before the tour was used.
- **Rendered** runs on four E-cores of the i5-12600KF (`taskset -c 12-15`), RTX 5070 Ti, Vulkan, unpaced, Smooth
  Motion off.

## 1. #480 at a fixed profile

A 2×2 of source and profile from `main` and #480 at `7d6a30d` (RecompCore `47a5229`). Kept states, mean over the
three, headless:

| Comparison | i5-6500 | E-cores |
| --- | ---: | ---: |
| #480's source at `main`'s profile | +1.4% (9/9 pairs) | +1.1% (9/9) |
| #480's source at #480's profile | +2.2% (9/9) | +2.2% (9/9) |
| `main` with #480's profile instead of its own | −0.9% | −0.9% |
| #480 with `main`'s profile instead of its own | −0.1% | +0.2% |
| Each with its own training | +1.3% | +1.3% |

The gain does not depend on retraining. #480's profile mostly costs `main`, so the source-at-#480's-profile figure
overstates it. Instructions drop 1.8 to 3.3%. Rendered on the E-cores, each with its own training: +1.4% (8/9). On the
tour, at `main`'s profile: **+1.6%**, every stop. Posted on #480, with the plan section in irregularthanis/bluewake#1.

## 2. Direct frame writes

Once `bw_inline_gpr_memory_ready()` has proven the frame plain MEM1 with no journal, `bw_inline_gpr_frame()` returns
its bytes and, for a save, clears a reservation on any granule the frame covers once. The generated code then uses
`write_be32`/`read_be32` at fixed offsets instead of 18 `mem_write32`/`mem_read32` calls. Each of those rechecks the
alias flag, the bounds, the reservation and the journal, and the compiler cannot merge those checks across the stores
because each byte store may alias them. `tests/inline_gpr_memory_test.c` checks the direct path against the helpers
for every first register, both stack forms and a reservation on every word in both forms. It passes, and a copy with
the reservation range cut short fails it.

At `main`'s profile, on top of #480:

| Where | Change |
| --- | ---: |
| Kept states, i5-6500 | +2.2% (9/9), instructions −0.7 to −1.2% more |
| Kept states, rendered, E-cores | +0.6% (9/9) |
| Benchmark tour, i5-6500 | **+1.0%**, every stop (+2.6% over `main`) |

The module is slightly smaller than `main`'s (370.9 MB of code against 372.6).

## 3. What the boundary census found

`BLUEWAKE_BOUNDARY_CENSUS_BY_ADDRESS` over the three kept states, with #480 and the direct writes.

**Caveat:** the census turns on `g_diagnostics_need_full`, which takes the full edge service and so turns off the
native actor search (`host_actor_search_native`). Its three addresses (`0x80245640`, `0x80245664`, `0x80040068`,
about 35% of boundaries in the census) are a census artefact. Ordinary runs log `[actor-search-native]` with about
1.6 million runs and 183 million nodes; census runs log none.

Of the rest:

- **`_savegpr`/`_restgpr` were still about 8.6% of boundaries** (`0x80328F34` to `0x80328F8C`). Two reasons:
  - **The REL modules' calls were never inlined.** They call through the mirror (`bl 0xC0328Fxx`), and the
    generator's call pattern only matched `bl 0x80328Fxx`. That is about 9,800 call sites, none inlined. The direct
    calls already treat them by their 0x8 form.
  - **2,610 calls in `main.dol` were skipped because their entry is "watched".** `0x80328F84` (`_restgpr_27`) is
    named for the GroundCross check, which only looks at returns to `0x80246A04`. `0x80328F40` (`_savegpr_29`) is
    named in a run-queue trace, also tied to one return address. Those returns are watched too, so excluding every
    other caller of the two entries was not needed.
- **`strcmp` from the actor search by name** (`0x8032DB44` and its call at `0x80041574`): about 6% of boundaries,
  17.6% at Outset. `native_search.c` covers it, behind `--native-entries`, which is off: on October 2 the natives
  measured about 1% of the game thread together (on Elliott's module), each must be certified against the BlueWake
  translation (#179, #361), and generation is currently blocked by lean deadlines' `_ato` forms (#451). Counted in
  boundaries, the natives' targets (`strcmp`, `ChkGrpThrough` at `0x800A9684` at 1.3%, `ChkSameActorPid` at
  `0x8024734C` at 0.5%) are a larger share than that 1% suggests.

## 4. Inlining the REL modules' calls: lost, then won

The generator now matches `bl 0xC0328Fxx` and checks the entry by its 0x8 form. `RETURN_KEYED` lets every caller
of `0x80328F84` and `0x80328F40` run inline except the ones whose return is watched (`0x80246A04` for the first;
`0x802B09C4` and `0x802B4100` for the second). A test fails if the host ever names either entry without one of
those returns, so a later hook that ignores `lr` cannot be skipped silently; it found the second return of
`0x80328F40`. Inline sites in the module
source went from 11,648 to 28,590, and 496 REL chunks now have them (none before).

| Build | Module code | Tour mean | vs `main` |
| --- | ---: | ---: | ---: |
| `main` | 372.6 MB | 53.99 | — |
| #480 + direct writes, `main`'s profile | 370.9 MB | 55.28 | +2.4% |
| + REL inlining, **`main`'s profile**\* | **397.2 MB** | 52.07 | **−3.4%** (−5.9% against the line above) |
| + REL inlining, **its own training** | **374.3 MB** | 58.31 | **+7.9%** |

\* From the first set of tour rounds, where `main` was 53.92 and #480 + direct writes 55.33; the other rows are from
the second set. Both sets loaded the same setup state.

Per stop, the trained build against `main` (three rounds each, game frames a second):

| Stop | `main` | Full stack, trained | Change |
| --- | ---: | ---: | ---: |
| Outset | 53.56 | 57.51 | +7.4% |
| Windfall | 46.12 | 49.86 | +8.1% |
| Dragon Roost Cavern | 66.05 | 72.59 | +9.9% |
| Forest Haven | 47.97 | 51.51 | +7.4% |
| Sea by the Fortress | 56.26 | 60.07 | +6.8% |

A second set of three rounds, later the same night, gave +8.2% (53.93 to 58.37). Guest checkpoints were identical
across every build in every run.

**Why the same source lost with `main`'s profile:** not because the profile stopped matching (the check above found 1
or 2 mismatched functions per chunk), but because of size. With `main`'s profile the new code in the REL chunks was
compiled at full size and the module grew 7%. The October finding was that a module only 4% bigger costs 2 to 3% on
small-cache cores, and the i5-6500 is one. Trained on its own source, the compiler optimized the cold sites for size,
and the module stayed at `main`'s size.

## 5. A shared helper instead: no better

To make the REL sites smaller without relying on training, each REL and return-keyed site called one out-of-line
`bw_gpr_run()` per source file instead of carrying its own stores (the main executable's sites kept the inline form).

| Build | Module code | Tour | Against the full-inline form |
| --- | ---: | ---: | ---: |
| Helper, `main`'s profile | 394.1 MB | 51.75 (2 rounds) | −0.5% (against full inline, `main`'s profile) |
| Helper, its own training | 372.0 MB | 58.01 | **−0.6%** (against full inline, trained), slower at every stop |

So the size came from the profile, not from the inlined stores, and the extra call costs a little. The full-inline
form stays.

## 6. Real AppImages, rendered: `main` today against the full stack, with and without #472

Three AppImages, each packaged with `scripts/linux/make_appimage.sh` from an app built in the CI container, and each
passing `check_appimage_abi.py`:

| AppImage | Game module | Runtime | Size | ELF files |
| --- | --- | --- | ---: | ---: |
| `main` today | `main`'s, trained | today's: LLVM libc++, Dawn compiled from source | 236 MB | 14 |
| Full stack | the full stack's, trained (§4) | today's: LLVM libc++, Dawn compiled from source | 237 MB | 14 |
| Full stack + #472 | the same module, byte for byte | #472's: static libstdc++, the prebuilt Dawn package | 182 MB | 12 |

The tour, rendered, on the four E-cores. A local copy of `bench_tour.py` also set `BLUEWAKE_DATA_DIR` to its scratch
folder and pointed `BLUEWAKE_DISC`, `BLUEWAKE_DOL` and `BLUEWAKE_RELS_DIR` at a prepared game folder: an AppImage
carries no game files, and without that it opens its setup window and writes into the player's own data folder. The
two full-stack AppImages ran three rounds with the order rotated; `main`'s ran one pass. Checkpoints were identical
across all three.

| Stop | `main` today | Full stack | Change | Full stack + #472 | Change |
| --- | ---: | ---: | ---: | ---: | ---: |
| Outset | 43.54 | 45.90 | +5.4% | 47.77 | +9.7% |
| Windfall | 37.52 | 40.00 | +6.6% | 41.06 | +9.4% |
| Dragon Roost Cavern | 54.15 | 58.17 | +7.4% | 60.73 | +12.2% |
| Forest Haven | 38.36 | 40.61 | +5.9% | 42.02 | +9.5% |
| Sea by the Fortress | 44.80 | 47.46 | +5.9% | 48.90 | +9.2% |
| Mean | 43.67 | 46.43 | **+6.3%** | 48.10 | **+10.1%** |

So against the AppImage players have today, the full stack is +6.3% rendered on these cores, and with #472's runtime
+10.1%, faster at every stop. #472 alone, the two full-stack AppImages against each other, is **+3.6%**, faster at every
stop in 3/3 rounds. That is in line with the folder-app measurements on #472 (+2.5 to +3%).

For the game code without the runtime, one pass of `main`'s folder app (libstdc++ and the prebuilt Dawn, like #472's
AppImage) gave 44.85, 38.30, 55.83, 39.47 and 45.70 (mean 44.83): the full stack + #472 is +7.3% against it, close to
the +7.9 and +8.2% headless, and libc++ costs `main` 2.6% (43.67 against 44.83).

One rendered run (the first of the three-round pass, the full stack in today's AppImage) had checkpoints that differ
from the others in `mem1` only, at every checkpoint, with the CPU state identical throughout: memory the CPU's state
never depends on, most likely frames copied from the GPU into guest RAM while the driver's shader cache was cold for
that binary. Every other rendered run, and every headless run, matched.

## 7. Without training, everywhere, and with more training

**Untrained builds** (`--no-train`, both from the same commits; the tour, headless, i5-6500, three rounds, checkpoints
identical): `main` 43.04, the full stack **45.97 (+6.8%)**, every stop +6.1 to +7.9%. The full stack's module is no
bigger untrained (338.8 MB against 339.4), so the growth in §4 came from `main`'s profile applied to the new code, not
from the code. Training itself is still worth about 25% (`main` 43.04 untrained, 53.99 trained).

**The whole stage select, timed.** `scripts/stage_select_sweep.py` over all 468 places, `main` against the full stack,
headless on the i5-6500, one place at a time with the builds alternating in chunks of 30, 600 play frames after Link
arrives, `BLUEWAKE_LOOSE_REL_RETRACES=1` (the Wind Temple's R04 and R08), cost read as the median `[perf]` rate after
arrival. The 30 places marked (before) ran from quest log 2 (before the Helmaroc King fight on this card), because the
command-line warp keeps the file's own half of the story; the menu sets it. 463 places were timed on both builds:

| Rooms | Places | Full stack against `main` (median) | Slower | Below +3% |
| --- | ---: | ---: | ---: | ---: |
| In the training tour | 39 | +7.0% | 0 | 0 |
| Not in the training tour | 406 | +6.7% | 3 | 7 |

So the REL inlining does not depend on training reaching a room. Every place ended the same way on both builds (the same
outcome and end pc). The sweep's checkpoints cannot compare builds: each run starts with a card continue, and two
continues of one build already differ (#450). Two of the three slower places are within noise (−1.3%, −0.9%). The
third, "Before Master Sword" (32.22), is an idle cutscene wait at 400 to 670 retraces a second where every build runs
the same 4,287,879 blocks: `main` 660, #480 594, + direct writes 417, + REL inlining with `main`'s profile 669, the same
source trained 417. The same source is the fastest and the slowest build there, so it is code placement in a tight loop,
not a cost of any change, and every build stays far above full speed.

**Adding rooms to the training tour.** Every untrained room slower than the trained rooms' median in the sweep (the
costliest first: SubD51, Hyrule Castle's inside, the warp maze, Sea sectors 2B and 2E, the Forsaken Fortress at night,
Tower of the Gods rooms…), added to `TRAINING_TOUR`: TA with the costlier 10, TB with all 20. Three rotated rounds:

| Where | TA (+10 rooms) | TB (+20 rooms) |
| --- | ---: | ---: |
| The default tour | +0.9% | +0.8% |
| The 20 added rooms | +1.0% | +0.8% |
| … the 10 TA trains | +0.8% | +0.6% |
| … the 10 only TB trains | +1.2% | +1.0% |

About +1%, and not specific to the rooms added: TA gains as much in rooms only TB trains, and 20 rooms are no better than
10. Each added stop costs about 13 seconds of training on 16 threads (five playbacks at once), and proportionally more
with fewer cores (one playback on four). **Open:** where the sweet spot is. Since the gain looks general rather than per
room, trials with fewer stops (8 and 12 were suggested, or fewer) would show how much of the +1% a short addition keeps.

## 8. Hand play

About 40 minutes by hand on Arch (i5-12600KF, RTX 5070 Ti, Vulkan) with an Xbox controller, on October 25: the full
stack in #472's AppImage (21 minutes) and in the folder app with a menu-fix host on the same module (20 minutes), from a real card. The sea
around Windfall (sea rooms 0, 3, 4, 10, 11, 12, 19, 26 and 27), Windfall, the Tower of the Gods' first room, and a lot of
sailing. No crash or hang; the game held 60 frames a second (the rate dipped below 58 twice in 2,025 one-second samples).
What went wrong was in the menu and is on `main` too, not the inlining; those fixes are chrissotraidis/bluewake#485.

## 9. Steam Deck

irregularthanis ran the headless benchmark tour on a Steam Deck (SteamOS) on October 25: the 0.9.0 AppImage
(`main-090`) against the full stack as an AppImage (`gpr-direct-ex`), 10 runs each, interleaved A/B/B/A, one fixed
setup state, every run loading 5/5 stops with 31/31 checkpoints, identical across both builds
([his comment on #480](https://github.com/chrissotraidis/bluewake/pull/480#issuecomment-6405842013)).

| Stop | `main-090` | Full stack | Change |
|---|---:|---:|---:|
| Outset | 41.268 | 44.537 | +7.92% |
| Windfall | 43.444 | 47.053 | +8.31% |
| Dragon Roost Cavern | 58.570 | 64.704 | +10.47% |
| Forest Haven | 35.804 | 37.397 | +4.45% |
| Sea by the Fortress | 33.194 | 35.770 | +7.76% |

The geometric mean of the per-run medians: **+7.68%**, against +7.9% and +8.2% on the i5-6500 (§4). The gain holds on
AMD. Headless, so #472's rendering gain isn't in it.

## Builds and branches

- `pdale-boop/bluewake` `exp/gpr-direct`: #480 at `7d6a30d`, then `043495d` (direct frame writes) and `52583a4` (REL
  and return-keyed inlining). This is the full stack measured above.
- `pdale-boop/bluewake` `exp/gpr-helper`: `4345be3`, the shared-helper experiment on top of `52583a4`. Kept for the
  record, not proposed.

## Not done yet

- `main`'s AppImage rendered for three rounds (§6 has one pass).
- Windows and a Mac. The Steam Deck (§9) is the only AMD CPU; the rest is Intel x86-64.
- The Steam Deck rendered, with #472's AppImage, to see whether it reaches the game's full 30 frames a second.
- A second independent training of the full stack, to put a number on profile luck. It moved results by about ±1% in
  §1, small against +8%.
- The training-tour sweet spot (§7).
- Hand play on other machines (§8 is one PC, about 40 minutes).
