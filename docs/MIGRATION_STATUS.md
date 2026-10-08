# Wind Waker Recomp is moving to BlueWake

Latest checkpoint: [October 6 input pass](status/SIMULATOR_INPUT_2026-10-06.md), including a stale-touch
repair and settings persistence checks. Earlier [Pictobox, map and audio evidence](status/SIMULATOR_PASS_2026-10-06.md)
remains applicable. Physical-device checks remain separate.

Updated October 6, 2026.

Current bug and support queue: [Priorities](PRIORITIES.md).

Elliott's [Wind-Waker-Recomp](https://github.com/elliotttate/Wind-Waker-Recomp) and BlueWake are
combining into one project. BlueWake is where development continues. This page tracks what is done
and what is still open. The [migration log](WIND_WAKER_RECOMP_MIGRATION.md) records each step and
what was changed on Wind-Waker-Recomp.

New bug reports, feature requests and pull requests should go to
[BlueWake](https://github.com/chrissotraidis/bluewake/issues).

## Done

- Elliott's Windows port, rendering, camera, controls, settings, save state and performance
  work through his v0.4.0 release, and his later Windows branch, is merged into BlueWake `main` ([#37](https://github.com/chrissotraidis/bluewake/pull/37),
  [#38](https://github.com/chrissotraidis/bluewake/pull/38)). His commits keep his authorship, and
  jointly adapted changes credit him as co-author.
- The shared runtime lives in [chrissotraidis/RecompCore](https://github.com/chrissotraidis/RecompCore),
  branch `bluewake-next`. BlueWake's dependency lock selects the exact commit.
- On an Apple Silicon Mac, a complete build from a player's own disc works through PadMint and the
  BlueWake builder: translation, local training, compilation and packaging. The result was played
  with an actual game save followed by a separate reload.
- On a physical iPad, an in-place update kept existing saves and settings, and an actual game save
  reloaded correctly.
- On Windows, the app builds and passes its automated tests in GitHub Actions.
- October 5: BlueWake 0.5.0 is the first Windows release built from BlueWake `main` (`0d1f821`), on
  Chris's PC and checked there ([Windows build for 0.5.0](status/WINDOWS_BUILD_0.5.0.md)). It replaces
  the Wind Waker Recomp 0.4.0 build that the October 4 release carried.
- October 4: BlueWake's latest release
  carried the Windows build from Wind Waker Recomp 0.4.0, with the same program files. It leaves out
  `nodtool.exe`, which contains Wii encryption keys, so a Dolphin `.rvz` must be converted to ISO first.
  Elliott's source archive for that build is attached beside it.
- October 4, on an Apple Silicon Mac: the current `main` host (with Elliott's October 3 runtime) and a
  module built from a player's disc boot to the title screen on Metal with Smooth Motion on, drawn
  correctly. This is a short check, not a gameplay session.
- Original 30 FPS game logic stays the default, and Smooth Motion is off unless a player turns it on.
  Wind Waker Recomp 0.4.0's experimental 60 FPS game logic is not in BlueWake `main`.

## Open

| Item | Owner | Status |
| --- | --- | --- |
| Windows build and gameplay on real hardware | Chris and Elliott | 0.5.0 was built from `main` and checked on one PC (Ryzen 7 5700U, integrated Radeon graphics). Still open: the checks that need a controller or the menu by hand, other GPUs and longer play ([WINDOWS_TASKS.md](WINDOWS_TASKS.md)). |
| Elliott's work after v0.4.0 | BlueWake and Elliott | Merged: [#40](https://github.com/chrissotraidis/bluewake/pull/40), [#43](https://github.com/chrissotraidis/bluewake/pull/43), [#45](https://github.com/chrissotraidis/bluewake/pull/45), and on October 4 his October 3 work: Mac water and HUD Smooth Motion and HD packs without shimmer ([#50](https://github.com/chrissotraidis/bluewake/pull/50)), the Wind Waker HD texture importer and Forest Water options ([#52](https://github.com/chrissotraidis/bluewake/pull/52)), and his performance write-ups ([#53](https://github.com/chrissotraidis/bluewake/pull/53)). His second and third sets of native functions and `lean_memory.py` are in BlueWake behind the Windows builder's `--native-entries` and `--lean-memory` options, off by default. Still open: turning them on. They apply only where the translated code matches what his Windows builder produces (on BlueWake's own iOS translation, 0 of 15 certify), so they need his newer `fast_blocks.py` and step order brought over and a Windows build from a disc to confirm. |
| Random slowdowns | BlueWake | Logs name the cause of each slow second and show when Smooth Motion is paused. The reported slow scenes (#59, #72, #86) and the 120 FPS switching (#79) don't reproduce on the Mac; their reporters have been asked for session logs from the current Windows download. [Stability plan](status/STABILITY_PLAN_2026-10-03.md). |
| Elliott's new work | Elliott | Opened as BlueWake pull requests from now on ([AGENTS.md](../AGENTS.md)). Elliott still needs to accept his BlueWake collaborator invite, sent October 1. |
| Move open issues | Chris | Done October 4: all 28 open issues are handled. 27 are recreated as [#54-#80](https://github.com/chrissotraidis/bluewake/issues?q=label%3Afrom-wind-waker-recomp) with the label `from-wind-waker-recomp`; Wind-Waker-Recomp #19 is BlueWake [#13](https://github.com/chrissotraidis/bluewake/issues/13). Each original has a comment with its new link, and all 28 were closed on October 4. |
| Move open pull requests | Authors | October 4: the authors of Wind-Waker-Recomp [#17](https://github.com/elliotttate/Wind-Waker-Recomp/pull/17), [#15](https://github.com/elliotttate/Wind-Waker-Recomp/pull/15) and [#13](https://github.com/elliotttate/Wind-Waker-Recomp/pull/13) (Android) were asked to reopen them against BlueWake. |
| Fresh build of current `main` | BlueWake | Done October 4 on the Mac: a complete build from a disc at `1c50db5` (88-minute compile, local training) plays a new game to control on Outset with Smooth Motion at 120 FPS, no slow seconds and no drops. The iPad app from the same source does the same in the iOS Simulator. Physical iPad not rechecked. |
| Longer gameplay session | BlueWake | About 30 minutes of real play with a controller and audio on Mac and iPad, to catch problems short checks miss. |
| Matched performance comparison | BlueWake and Elliott | Compare BlueWake with Wind-Waker-Recomp in the same scenes, settings and hardware. |
| Pictobox freeze | BlueWake | [#13](https://github.com/chrissotraidis/bluewake/issues/13). 0.5.0 no longer freezes for a Windows tester. An opt-in shared-host fix repairs stale second previews in matched Mac and iPad simulator runs; regular/Deluxe simulator previews, scripted photo selection and regular-photo normal save/relaunch/reload passed. Physical iPad/Windows and controller checks remain. See [latest simulator evidence](status/SIMULATOR_PASS_2026-10-06.md#pictobox-13). |
| Point Wind-Waker-Recomp to BlueWake | Chris and Elliott | Done October 3: its README and issue form point here ([7b6a2c3](https://github.com/elliotttate/Wind-Waker-Recomp/commit/7b6a2c3aafa7619229a2dd9316251872f1cb9c9d)). The repository description needs Elliott (admin). Its ten releases were unpublished on October 4 (kept as drafts, files intact) and its download links point here ([adac1e6](https://github.com/elliotttate/Wind-Waker-Recomp/commit/adac1e616376ab0d7d57eda51d82acf4dc607079)). Archiving the fork waits until issues are moved, Windows is checked and both agree. |

Other game regions and an Android port are later work and not part of this move. Linux: on October 4
the maintainers decided BlueWake may publish a ready-made Linux build, like the Windows one
([AGENTS.md](../AGENTS.md#releases)). The native port is under review in [PR #107](https://github.com/chrissotraidis/bluewake/pull/107).
Its contributor reports near-steady 30 FPS; full log review, CI, package checks and a shutdown memory
error remain before official support. See TECH_DEBT.md.

## Releases

Windows: a ready-made build is on the Releases page and needs your own disc image. Linux: a ready-made
build is allowed under the same rules once a native port is in BlueWake. Mac, iPhone and iPad: build
BlueWake yourself from your own disc with PadMint. Builds you make yourself contain game code and must
never be uploaded or shared.

## How to help

- Windows testers with a Direct3D 12 GPU: follow the [Windows checklist](WINDOWS_ACCEPTANCE.md) and
  report results in a BlueWake issue.
- Report bugs with your platform, the BlueWake commit you built and steps to reproduce.
- Attach your session log (iPhone/iPad: Help & Feedback › Share Session Log; Windows:
  `%APPDATA%\BlueWake\logs`). `python3 scripts/triage_session_log.py LOG` summarizes one.
- Keep game files, saves and personal builds out of issues and pull requests.

Detailed evidence: [reconciliation ledger](status/FORK_RECONCILIATION_2026-10-02.md).
