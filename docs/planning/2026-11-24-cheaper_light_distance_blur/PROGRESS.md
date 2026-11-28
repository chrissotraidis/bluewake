# Progress — A cheaper "Light" distance blur

[Product brief](PRD.md) · [Technical plan](TECH_PLAN.md) · [Progress](PROGRESS.md)

- 2026-11-24: measured on the owner's Deck from a save state (rainy Windfall at night, sea room 11). Distance blur: Light found to cost 4–5 retraces a second at 3x; the plan written. No code yet.
- 2026-11-24: reruns with ambient occlusion off (59.6, 58.7) and the HD look off (60.0) ran to the end; the two crashes first read as startup crashes were near the end of their runs and did not repeat. Reported apart.
- 2026-11-24: the code: RecompCore #67 (the half-size blur, `DOL_AURORA_GAME_DOF_FAST`), pinned here; the Config switch "Faster Light distance blur" and its test entries. Settings tests pass locally; not yet built or measured.
- 2026-11-24: built (`128888488`: this branch, RecompCore #67 at `7558a865`, two local diagnostics) and measured on the owner's Deck in Game Mode, the rain state, 3x, about 25 s a run, two runs each, interleaved:

  | Distance blur | Retraces a second | GPU busy per shown frame | The blur's zone |
  |---|---|---|---|
  | Light, switch off | 54.7, 54.3 | 18.6, 18.3 ms | "IB EFB copy blur" 0.95, 0.85 ms |
  | **Light, switch on** | **58.9, 58.2** | 17.9, 17.9 ms | "IB EFB copy blur (fast)" 0.24, 0.24 ms |
  | Default | 59.8, 59.8 | 16.5, 16.8 ms | none |

- 2026-11-24: pictures, switch off against on, the same state on a desktop (both at full speed, so the captured frames are the same game frames; the Deck's settings, 3x, a 2560x1440 window): PSNR 62.47, 62.78 and 62.46 dB at retraces 101400, 101700 and 102001, the largest difference 8 to 10 of 255 on any channel. A fourth pair was captured a game frame apart (the rain had moved) and is left out. Pictures in `images/` (the difference times 16). Still to do: a clear-day distance view, and 1x and 2x.
- 2026-11-24: the technical plan brought up to what was built (one half-size pass and a blend back, not two blur passes and a composite; the switch does not depend on another setting; the menu does not pass it on). The PRD's first success line is not quite met: 0.9 to 1.6 retraces a second short of Default, not within 1. A render bench over ten states, switch on against off, is to follow.
- 2026-11-24: the render bench on the owner's Deck in Game Mode over ten save states (`scripts/render_bench --display gamemode --paced --pairs 2`, the owner's shortcut and settings: 3x, the HD look, Smooth Motion, the texture pack), build `4cc7b447b` three ways by its switches: neither, this switch, this switch and #658's. Twelve measured runs interleaved, the median of four each; guest checkpoints identical. This switch on against off:

  | Scene | Game fps, switch off | Game fps, switch on | Change | GPU time, switch off | GPU time, switch on | Change |
  |---|---|---|---|---|---|---|
  | Windfall: night rain | 27.2 | 29.3 | **+7.4 %** | 18.7 ms | 18.2 ms | **−3.0 %** |
  | Outset: busy view | 27.4 | 29.3 | **+6.7 %** | 20.6 ms | 19.8 ms | **−3.9 %** |
  | Outset: bench view | 23.6 | 24.9 | **+5.3 %** | 22.9 ms | 21.5 ms | **−6.1 %** |
  | Sea: Big Octo fight | 23.4 | 24.6 | **+5.1 %** | 26.6 ms | 25.2 ms | **−5.1 %** |
  | Sea: shipwreck | 28.3 | 29.6 | **+4.8 %** | 18.9 ms | 18.0 ms | **−4.6 %** |
  | Outset: busiest view | 24.7 | 25.8 | **+4.6 %** | 22.0 ms | 20.2 ms | **−8.1 %** |
  | Outset: coast (butterflies) | 29.4 | 30.7 | **+4.3 %** | 21.3 ms | 20.0 ms | **−5.8 %** |
  | Outset: ordinary play | 29.4 | 30.0 | +1.8 % | 18.1 ms | 17.3 ms | **−4.3 %** |
  | Hyrule Castle | 29.9 | 30.0 | +0.1 % | 17.9 ms | 16.5 ms | **−8.1 %** |
  | Dragon Roost Cavern: lava room | 30.0 | 30.0 | 0.0 % | 18.3 ms | 18.7 ms | **+2.3 %** |

  Overall (each run's geometric mean over the scenes, the median over runs): 27.23 → 28.32 game frames a second, +4.0 %; with #658's switch as well 28.54, +4.8 %. The gain is in seven scenes of ten, not only the rain. The lava room's GPU time is up 2.3 % at an unchanged 30: the Deck's GPU clock, logged about once a second through the bench, averaged 1289 MHz there with the switch off and 1157 MHz with it on (lower with the switch on in eight of ten scenes), so a millisecond is less work with the switch on.
- 2026-11-24: pictures from the bench's picture runs on the Deck (`images/bench-*.jpg`: switch off, switch on, the difference times 16). 17 of 50 pairs were the same game frame (largest difference 30 of 255 or less; the others are a frame apart and differ by over 100). The worst same-frame pair of each scene: 53.9 dB (Outset busiest view, largest difference 19, 0.065 % of pixels over 8) to identical (Hyrule Castle). The first picture runs did not start: a network drop left Steam waiting on a "cloud sync failed" dialog; they were taken again after it was dismissed.
- 2026-11-24: two more states from the owner, made for #658 (a lava geyser, and dark ash at Dragon Roost Cavern's entrance on a new file), benched the same three ways in their own session, the median of four runs. This switch on against off: the geyser 25.8 → 27.3 game frames a second (+6.1 %), GPU time 22.1 → 20.4 ms (−7.4 %); the ash 25.3 → 27.0 (+6.7 %), 18.5 → 17.2 ms (−6.8 %). With #658's switch as well: 29.3 (+13.9 %) and 27.8 (+9.9 %). No validation errors, guest checkpoints identical. Same-frame pictures: the geyser 60.9 dB (largest difference 13 of 255), the ash 57.4 and 57.5 dB (largest difference 13, 0.002 % of pixels over 8). The ash state was saved after loading a state made with mods off, so the mod's changes may not all be in it.
