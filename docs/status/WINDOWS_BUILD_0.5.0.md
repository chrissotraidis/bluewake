# Windows build for BlueWake 0.5.0, October 5, 2026

The Windows files for 0.5.0 are on the draft release **BlueWake 0.5.0** (tag `v0.5.0`, not published). Chris's Mac
still has to run the release gate, add the iPhone and iPad files, and publish.

| | |
| --- | --- |
| Commit | `0d1f821b40905f19eceb2f39dfff7ef5179added` (version 0.5.0, build 4), clean checkout |
| RecompCore | `95a6a477001e52aa73227c09f34c8e27a05d7e2f` (the build log: "fetching RecompCore 95a6a47..."), DolRecomp `b8b5345` |
| Generated source | digest `54f54434…`, the verified tree |
| Build | `python scripts/windows/build.py C:\BlueWake-private\GZLE01.iso --jobs 3`, one run, **129 min** (training 62 min, optimized compile 57 min), clang 22.1.3 |
| Module | SHA-256 `45bb3f502043e7663034bdd3613d07870012764b0c5d093380105341895bb0d5`, trained (425 of 813 functions ran) |
| `BlueWake-v0.5.0-windows-x64.zip` | `435de0964b193ac4d278b65d25ecb80bc3a3001ba8a1d7242993cf363ce96626` (33 files, no `game\`, disc or `nodtool.exe`) |
| `BlueWake-v0.5.0-source.zip` | `191501d69956a8cee7b74e2c0ee31374278830bdcedd1d04b276d0a7de2499fc` |
| PC | Ryzen 7 5700U, Radeon integrated graphics, 12.9 GB RAM, Windows 11 Pro |

## The second Windows run (items 1 to 9)

I ran every check once, from the unpacked release zip with `portable.txt`.

| # | Check | Result |
| --- | --- | --- |
| 1 | A crashed launch is recovered (#58) | **Pass.** I set `betterww=1`, ended the process 3 s after start, and started again. The "Launch recovery" box said: "BlueWake didn't finish starting last time, so this time it starts with Fast sound, 4:3 and the mods off. Your earlier settings were saved beside settings.ini..." `settings.ini.before-safe-mode-*` held `betterww=1`, and the live file had `betterww=0`. The next normal launch showed no message. |
| 2 | Camera-stick inversion (#73) | **Not tested** (no controller). |
| 3 | Mouse buttons and keyboard keys | **Keys pass; mouse not tested.** With `key_map=15,14,24,12,20,8,21,40` and `BLUEWAKE_PAD_TRACE=1`, L gave `button=0x0100`, J gave nothing, and L gave `0x0100` again. My injected mouse clicks never reached the window (see below), so the `mouse_buttons=A,-,B,-,-` part and the change by hand in F1 › Controls are untested. |
| 4 | "Quit the game" and the three option notes | **Not tested.** F1 opens the menu, but I couldn't click tabs or buttons (see below). |
| 5 | Older CPUs get a message (#77) | **Pass.** Under Intel SDE 10.13.1 `-nhm`, a message box says "BlueWake can't run on this processor. This download needs a CPU with AVX2...". Normal launches on this Ryzen start as before. I didn't run `-hsw`. |
| 6 | `gamecontrollerdb.txt` (#61) | **Partial.** With the SDL_GameControllerDB file in the portable `user` folder, the log shows `[pad] 578 controller mappings from ...\user\gamecontrollerdb.txt`. I had no controller to confirm one still works. |
| 7 | The disc prompt without `nodtool.exe` | **Pass.** From the zip, the prompt asks for ".iso or .gcm file" and says how to convert an .rvz in Dolphin. The picker's type reads "GameCube disc images (*.iso, *." and showed only the ISO. I couldn't open the drop-down, but the code's no-nodtool filter is `*.iso;*.gcm`. I didn't check a self-built folder. |
| 8 | Portable mode keeps the caches in `user` (#64) | **Pass.** `dawn_cache.db` and `pipeline_cache.db` were created in `user`. In `%APPDATA%\BlueWake` (left over from my October 4 test), only `imgui.ini` changed, as expected until RecompCore lets the host set Aurora's `userPath`. |
| 9 | Molgera's sand floor (#126) | **Pass.** I used cbartondock's `7_windfall.gci`, moved it with `save_set_restart.py ... kazeB 0 0`, injected it into a copy of a fresh card, and loaded slot 1 with A presses at retraces 340 to 760. The screenshot shows Link in the arena on a sand floor, and `[gx-core] shutdown` has `array_unresolved=0` (with `unsupported_texgen=0` and `tev_stages_over=0`). |

## Odd things

- **Injected mouse input doesn't reach BlueWake or the file dialog in this remote session.** Keys sent with
  WScript.Shell's SendKeys after AppActivate work, but `SendInput` mouse clicks (and my `SendInput` F1) did nothing.
  That's why items 3 (mouse), 4 and the change by hand are untested. It's a limit of my test setup and says
  nothing about BlueWake. Someone at the PC can check them in a minute.
- Arrow keys and Ctrl+Tab didn't move between the menu's tabs, although the menu turns on ImGui keyboard navigation.
  That may be the same input limit, so I'm not calling it a bug.
- The training profile and module differ from the October 5 morning draft (`c56d6b6`) only because the code
  changed. The build steps and their times were the same.
