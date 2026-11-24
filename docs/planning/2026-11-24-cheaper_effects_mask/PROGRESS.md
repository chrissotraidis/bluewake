# Progress — A cheaper effects mask for the HD look's shading

[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md) · [Progress](PROGRESS.md)

- 2026-11-24: measured on the owner's Deck from a save state (the Big Octo fight, sea room 17): 16 runs, in the PRD. The effects mask found to be 2 to 3 ms of a 24.6 ms frame; its second pass alone 2.1 ms. The plan written. No code yet.
- 2026-11-24: the code, first candidate: RecompCore #68 (the mask and the pass over white at half size, against a half-size depth; `DOL_AURORA_EFFECT_MASK_FAST`), pinned here; the Config switch "Faster effects mask" and its test entries. This branch now sits on #655's (both add a switch at the same place in the table). Settings tests pass locally; not yet built or measured.
- 2026-11-24: built and measured on the owner's Deck in Game Mode, the Big Octo state, 3x, about 25 s a run, interleaved. First build (`aaf3a36fa`, RecompCore #68 at `f612af1b`, the half-size depth made for each of the two passes): 50.9, 50.2 against 52.0, 51.5 retraces a second; `busy_ms` 24.6, 24.9 against 24.1, 23.6. Second build (`3e4823437`, #68 at its next commit: the half-size depth made once for both passes when no effect is at its own depth):

  | Effects mask | Retraces a second | GPU busy per shown frame |
  |---|---|---|
  | Full size (switch off) | 50.1, 49.8, 49.8 | 24.6, 24.6, 25.0 ms |
  | **Half size (switch on)** | **51.9, 51.6, 51.7** | **23.2, 23.3, 22.8 ms** |

  About 1.8 retraces a second and 1.6 ms. The mask's zones do not fall to a quarter: part of their cost is the draws themselves, not their fill. Leaving the pass over white out altogether measured 2.1 ms, so the second candidate (both targets in one pass) is where the rest is.
- 2026-11-24: pictures, switch off against on, the same state on a desktop with the Deck's settings (3x, a 2560x1440 window), the first build: PSNR 65.1, 78.0, 82.3 and 91.4 dB on four frames of the same game frame; the largest difference 19 of 255, 258 pixels over 8 on the worst frame. No validation errors. In `images/`. A capture now and then misses the octopus and the boat with the switch off as with it on (two switch-off runs differ so): the capture's, not the mask's; such pairs are left out. Still to do: a lava room (effects at their own depth), a door's dust, a plain view, 1x and 2x; and the pictures again on the second build.
- 2026-11-24: pictures on the second build (`3e4823437`), switch off against on, a desktop with the Deck's settings, four frames a scene 240 retraces apart; each scene run off, on and off again. A pair counts only when it is the same game frame (the HUD's hearts identical; 6 of 44 were a frame apart, the run's timing, and are left out):

  | Scene | Size | Same-frame pairs | PSNR, worst to best | Largest difference (of 255) | Pixels over 8 |
  |---|---|---|---|---|---|
  | Big Octo, 3x | 2560x1440 | 4 of 4 | 61.1 to 84.4 dB | 29 | 0.019 % |
  | Big Octo, 2x | 1707x960 | 1 of 4 | 59.5 dB | 28 | 0.028 % |
  | Big Octo, 1x | 853x480 | 4 of 4 | 57.0 to 80.8 dB | 29 | 0.046 % |
  | Outset, 3x | 2560x1440 | 3 of 4 | 63.0 to 66.0 dB | 23 | 0.024 % |
  | Busiest view, 3x | 2560x1440 | 3 of 4 | 63.7 to 64.1 dB | 20 | 0.023 % |
  | Heavy view, 3x | 2560x1440 | 4 of 4 | 79.6 to 87.7 dB | 6 | none |
  | Shipwreck, 3x | 2560x1440 | 4 of 4 | 87.5 to 89.7 dB | 4 | none |
  | Hyrule Castle, 3x | 2560x1440 | 4 of 4 | identical | 0 | none |
  | Lava room, 3x, 2x, 1x | | 10 of 12 | identical | 0 | none |

  No validation errors in any run. The lava room state draws no effects mask at all (no mask zones in its GPU profile), so it says nothing; Hyrule Castle draws it and is identical. **Not covered by any saved state:** effects at their own depth (a lava geyser, a particle's dust), which take the sea mask's depth path, and the dark-effect case the pass over white exists for (a door's dust in Dragon Roost Cavern). Both need states made for them.
- 2026-11-24: the second candidate, under the same switch: both targets in one pass (RecompCore #68's next commit). Each effect gets a two-target variant of its pipeline (`PipelineConfig::softTarget` 2, no new field, so the pipeline cache keeps what it has); a pass whose effects all have theirs ready writes the mask and the one over white at once, any other keeps the two passes. Effects with destination alpha, a depth texture or an early depth pass have no variant. `DOL_AURORA_EFFECT_MASK_SINGLE=0` keeps the two passes, for measuring. Build `4cc7b447b`, the owner's Deck in Game Mode, the Big Octo state, 3x, three runs each, interleaved:

  | Effects mask | Retraces a second | GPU busy per shown frame |
  |---|---|---|
  | Full size, two passes (switch off) | 49.9, 50.3, 50.0 | 24.2, 24.5, 24.9 ms |
  | Half size, two passes (`…_SINGLE=0`) | 52.6, 52.1, 52.5 | 22.5, 22.8, 22.8 ms |
  | **Half size, one pass (switch on)** | **53.1, 52.8, 53.2** | **21.9, 22.2, 22.0 ms** |

  Switch on against off: +3.0 retraces a second (+5.9 %), −2.5 ms (−10.2 %). The one pass is in use in this scene ("Effects mask (both)" in the GPU profile, no two-pass zones). Pictures on a desktop: the one-pass frames are identical, pixel for pixel, to the half-size two-pass ones (three same-frame pairs), so the scene table above stands for both. No validation errors. Pipelines made in a run: about the same with the switch on as off (the variants compile once and are cached); a first-use hitch was not looked for specifically.
- 2026-11-24: the brief and the technical plan brought up to what was built (both parts under one switch, the third idea dropped, the switch's final name).
- 2026-11-24: the render bench on the owner's Deck (`scripts/render_bench --display gamemode --paced --pairs 2 --states`), through the owner's Steam shortcut with the owner's settings (3x, HD look, Smooth Motion, the texture pack, #655's switch on; the FPS counter off), build `4cc7b447b`, the switch on (A) against off (B), ten save states of 1800 retraces: 8 measured runs A/B/B/A/A/B/B/A and a picture run of each. Guest checkpoints identical across every run (6 each). Medians over the four runs of each:

  | Scene | Game frames a second, off → on | GPU busy per shown frame, off → on | Change |
  |---|---|---|---|
  | Outset: bench view | 25.0 → 25.2 | 21.7 → 21.0 ms | −0.7 ms |
  | Outset: coast (butterflies) | 30.7 → 30.9 | 19.7 → 19.0 ms | −0.6 ms |
  | Outset: ordinary play | 29.9 → 30.0 | 17.7 → 17.4 ms | −0.3 ms |
  | Outset: busy view | 29.4 → 28.9 | 19.8 → 19.8 ms | 0.0 ms |
  | Outset: busiest view | 25.9 → 26.2 | 20.1 → 19.5 ms | −0.6 ms |
  | Dragon Roost Cavern: lava room | 29.9 → 30.0 | 18.3 → 18.5 ms | +0.2 ms |
  | Hyrule Castle | 30.0 → 29.9 | 16.5 → 16.9 ms | +0.4 ms |
  | Sea: shipwreck | 29.7 → 29.5 | 18.2 → 18.5 ms | +0.4 ms |
  | Windfall: night rain | 29.3 → 29.9 | 17.8 → 16.6 ms | −1.2 ms |
  | Sea: Big Octo fight | 24.9 → 26.3 | 25.1 → 22.5 ms | −2.6 ms |

  Overall (each run's geometric mean over the scenes, the median over runs): 28.37 → 28.61 game frames a second, +0.84 %. The gains are in the scenes below full speed; in scenes already at 30 the GPU time is level or a little up (the Deck's GPU lowers its clock with its load, 200 to 1600 MHz, so a millisecond is not the same work there; a clock log is being taken with the next bench to check this).
- 2026-11-24: two things about the tool in this mode. Its analysis found no numbers at first ("0/8 runs loaded every scene", empty tables): Game Mode's session log has a time before each line, and `bench_tour.scan_log` wants `[perf]` at the line's start. The table above is from the same `analyze` over the logs with the times taken off. And its picture verdicts (four "failed", four "changed") compare frames that are mostly not the same game frame in Game Mode: of 50 pairs, 13 are (the HUD's hearts identical). Those 13: identical in Hyrule Castle; 63.9 dB or better elsewhere (largest difference 18 of 255, at most 0.022 % of pixels over 8). Sheets of the worst same-frame pair of each scene that has one are in `images/bench-*.jpg`; the Big Octo fight, the lava room, ordinary play and the busy view had none on the Deck (the Big Octo sheet is the desktop's same-frame pair).
- 2026-11-24: a second bench the same way, build `4cc7b447b` three ways by its switches (neither, #655's faster blur, both), twelve measured runs interleaved, the median of four each. Both switches against neither:

  | Scene | Game fps, neither switch | Game fps, both switches | Change | GPU time, neither switch | GPU time, both switches | Change |
  |---|---|---|---|---|---|---|
  | Windfall: night rain | 27.2 | 29.9 | **+9.8 %** | 18.7 ms | 16.6 ms | **−11.6 %** |
  | Outset: busy view | 27.4 | 28.9 | **+5.2 %** | 20.6 ms | 20.0 ms | **−3.0 %** |
  | Outset: bench view | 23.6 | 25.1 | **+6.1 %** | 22.9 ms | 21.0 ms | **−8.5 %** |
  | Sea: Big Octo fight | 23.4 | 26.2 | **+11.7 %** | 26.6 ms | 22.7 ms | **−14.6 %** |
  | Sea: shipwreck | 28.3 | 29.4 | **+3.9 %** | 18.9 ms | 18.5 ms | −1.7 % |
  | Outset: busiest view | 24.7 | 26.0 | **+5.5 %** | 22.0 ms | 19.5 ms | **−11.0 %** |
  | Outset: coast (butterflies) | 29.4 | 31.0 | **+5.3 %** | 21.3 ms | 19.3 ms | **−9.3 %** |
  | Outset: ordinary play | 29.4 | 30.0 | +1.8 % | 18.1 ms | 17.9 ms | −1.3 % |
  | Hyrule Castle | 29.9 | 29.9 | 0.0 % | 17.9 ms | 16.9 ms | **−5.6 %** |
  | Dragon Roost Cavern: lava room | 30.0 | 29.9 | −0.2 % | 18.3 ms | 17.6 ms | **−3.9 %** |
  Overall: 27.23 game frames a second with neither, 28.32 with the blur (+4.0 %), 28.54 with both (+4.8 %). This switch over the blur alone: the Big Octo fight 24.6 → 26.2 (+6.3 %, GPU time −10.0 %), the rainy Windfall 29.3 → 29.9 (+2.2 %, −8.8 %), which repeats the first bench; the Outset busy view 29.3 → 28.9 (−1.4 %) and the shipwreck −0.9 %, neither explained. In percent the first bench reads: Big Octo +5.6 % and −10.5 %, the rain +2.0 % and −7.0 %, the others within 2 % of game frames a second.
- 2026-11-24: the Deck's GPU clock, logged about once a second through that bench (`/sys/class/drm/card*/device/pp_dpm_sclk`, samples with the GPU over 80 % busy): lower on average with this switch on in eight of the ten scenes, by 1 to 5 % (the shipwreck 1230 → 1206 MHz, the lava room 1157 → 1127, the Big Octo fight 1429 → 1359). So GPU time understates the saving where the clock fell; it does not explain Hyrule Castle (level clock, GPU time up 2 to 3 % in both benches).
