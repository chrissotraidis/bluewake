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
