# Windows testing handoff for Elliott

The current list of Windows work is [WINDOWS_TASKS.md](WINDOWS_TASKS.md); this checklist is its task 1.

The source consolidation is merged into BlueWake `main` in [#37](https://github.com/chrissotraidis/bluewake/pull/37),
with the Windows local-training follow-up in [#38](https://github.com/chrissotraidis/bluewake/pull/38).
Please use BlueWake for new changes and preserve contributor attribution.
Any uncommitted fixes in Wind-Waker-Recomp should become focused BlueWake PRs.

The physical iPad update, actual save/separate-launch reload and preservation of
existing saves/settings passed. Its Documents and Library were backed up locally
before installation. Source changes are pushed; personal builds and backups stay local.
Windows CI builds the host and passes 58 runtime and 24 builder checks.
Native Windows gameplay and performance still need testing. Broader sustained
Apple gameplay/performance coverage remains separately tracked in the
[reconciliation ledger](status/FORK_RECONCILIATION_2026-10-02.md).

## Hardware and checkout

If you have access to an x64 Windows PC with a Direct3D 12 GPU, please run the
checks below. Chris has no Windows PC. If you do not have one either, report that
so a tester can be found. A headless server or ARM64 VM does not establish native
graphics, controller, audio and performance acceptance.

Follow [Windows prerequisites and build instructions](WINDOWS.md#what-you-need).
For a new checkout:

```powershell
git clone https://github.com/chrissotraidis/bluewake.git
cd bluewake
git rev-parse HEAD
```

For an existing checkout, preserve any local work before updating to `main`.
Let the builder select the exact dependencies from `config/dependencies.lock.json`.
The maintained runtime repository is `chrissotraidis/RecompCore`; its integration
branch is `bluewake-next`, but builds use the lock's exact commit.

## Build and test

1. **Clean personal build:** use your own supported USA revision-0 disc and a
   fresh output directory:

   ```powershell
   python scripts/windows/build.py "D:\Games\Wind Waker.iso" --out "build\windows-acceptance"
   ```

   Keep default local training enabled. Verify both plain/modded training runs
   reach player control and produce the trained O2 module. Retain local logs and
   provenance. Check interruption/resume by rerunning the same command after a
   normal interruption. Use copied saves; preserve the original disc and saves.

2. **Launch and persistence:** run `build\windows-acceptance\BlueWake\BlueWake.exe`.
   Test a fresh start and existing save, disc import/recovery, settings and restart,
   fullscreen, save states, actual game save followed by a separate-launch reload,
   and an in-place update with saves/settings preserved.

3. **Reported problem areas:** test Windfall Pictobox preview, shutter, cancel,
   repeat and saved-album reload; startup/settings freezes; narrated intro and
   scripted music; the bird/abduction slowdown; and the camera's left and right
   while swimming with the right stick (it should no longer flip, #24). Record
   reproducible failures with steps. A Mac result does not close the
   corresponding Windows check.

4. **Controls and sustained play:** test keyboard/mouse and a physical controller,
   button layouts, menus, reconnect and vibration. Listen for music/effects and
   dropouts. Play at least 30 minutes with real progression and scene transitions,
   including settings and save/reload. Try the new menu entries: fast right-stick
   camera and its speeds, wall climbing and stamina, and "Compile shaders before
   playing". With the FPS counter on and Smooth Motion at 60, it should say
   "Smooth Motion paused" rather than 60 whenever the in-between frames stop.

5. **Donor comparison:** compare BlueWake with your recorded Wind-Waker-Recomp
   baseline on the same machine, using the same scenes and settings: Outset, a busy
   area and the reported slow scripted scene. Keep original 30 Hz logic, Smooth
   Motion Off and experimental 60 Hz Off for the baseline. Record frame-time
   spikes/stalls and audio as well as averages. Keep displayed FPS distinct from
   game speed. Report any regression and investigate optional optimizations
   separately. The builder now prepares your optimization set by default (fixed
   CPU/RAM, inline helpers, prepaid blocks, direct calls, natives; `--conservative`
   turns them off), and the session log's `[chassis]` lines show which are on.
   Your second set of natives and `lean_memory.py` are not in BlueWake yet.
   `python3 scripts/triage_session_log.py` summarizes a session log's slow seconds
   by cause.

## Results to return

Record BlueWake and donor commit IDs, dependency pins, Windows version, CPU,
GPU/driver, compiler, settings and app/module hashes. Use a short table:

| Check | Pass / fail / untested | Evidence or reproduction steps |
| --- | --- | --- |
| Clean disc build, training and resume | | |
| Startup, import/recovery, settings and fullscreen | | |
| Save/reload, states and update preservation | | |
| Pictobox | | |
| Intro/scripted music and audio | | |
| Keyboard/mouse, controller and reconnect | | |
| 30-minute progression | | |
| Matched donor performance comparison | | |

Return sanitized textual findings and focused source fix PRs. Keep personal apps,
modules, generated game source, profiles, discs and saves local; do not attach them
to GitHub. Releases follow [AGENTS.md](../AGENTS.md). Repository redirects, issue/PR migration
and archival wait for the migration decision; this handoff is for Windows testing.
