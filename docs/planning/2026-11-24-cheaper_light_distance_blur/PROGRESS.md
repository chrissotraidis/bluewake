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
