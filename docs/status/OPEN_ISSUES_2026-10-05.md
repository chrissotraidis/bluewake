# Open issues and reports, October 5, 2026

Every open GitHub issue on October 5, 2026, plus problems players reported in the BlueWake Discord on
October 4 and 5. Discord reports are recorded here to be fixed in the next version; they don't get replies
or their own issues. This is a snapshot: [GOAL_LOOP.md](../GOAL_LOOP.md) has the work and its progress.

Every Windows player is still on Wind Waker Recomp 0.4.0, so a Windows report doesn't show whether
`main` has the problem. Logs attached to issues were read with `scripts/triage_session_log.py` and by hand.

## The first Windows build from `main`

Built on Chris's Windows PC from `c56d6b6` (this morning's `main`) and packaged as the draft release `v0.5.0-windows` ([report](WINDOWS_BUILD_2026-10-05.md)). The release check on the Mac passes it with only the accepted `containsTranslatedGameCode` finding; the source zip passes. On that PC the game runs, the intro cutscene plays all four sounds (as on the Mac), Exact sound starts without crashing, and the zip works from a fresh folder. It holds the fixes listed above as "fixed in `main`" and #58's crash fix, but nothing merged later today (mouse and keyboard rebinding, the Quit button, the AVX2 message, `gamecontrollerdb.txt`, the plain recovery message, the disc prompt, portable caches); those need the second Windows run in WINDOWS_TASKS.md. New from it: portable mode left Aurora's caches in `%APPDATA%\BlueWake` (#124 fixes the caches; `imgui.ini` needs RecompCore), the disc prompt offered .rvz without `nodtool.exe` (#123), and the triage script miscounted a startup line (#122).

## What the October 5 loop found

| Report | Result |
| --- | --- |
| #58 Exact sound crash | Already fixed in `main`; recovery message made plain in [#111](https://github.com/chrissotraidis/bluewake/pull/111). Moved to "fixed in `main`" above. |
| Camera inverting by itself | Two causes. In 0.4.0, the camera switched between the fast stick camera and the game's own camera (swimming, the boat, targeting), which turns the other way; #44 in `main` fixes that. On the Mac, the right stick's invert setting didn't reach the game's own camera; fixed in [#112](https://github.com/chrissotraidis/bluewake/pull/112). Better Wind Waker's "Invert camera left and right" now says what it affects ([#114](https://github.com/chrissotraidis/bluewake/pull/114)). |
| Controls settings reverting | `main` stopped launch overrides from overwriting saved Windows settings on October 1 and 2 (`8b4611d`, `e3536b1`), after 0.4.0. A crash recovery also resets settings (with a backup), and now says so. |
| Mouse and keyboard rebinding | Done on Mac and Windows ([#113](https://github.com/chrissotraidis/bluewake/pull/113)). Checked on the Mac: right-click set to B logged B; A moved to L logged A on L and nothing on J. |
| Quit from the menu, Brisk Sail and Unrestricted boat | Windows has "Quit the game"; both menus explain the two options ([#114](https://github.com/chrissotraidis/bluewake/pull/114)). |
| #74 dungeon map, sea charts | **Fixed in `main` after 0.5.0 (RecompCore patch 0157), checked on the Mac.** The map's quads carry all eight raw texture coordinates and use eight texgens and twelve TEV stages; RecompCore allowed 5 and 8, and its fixed vertex layout five coordinates. With the limits raised and TEX5..7 carried in the unused normal slots, the Dragon Roost Cavern map draws its grid and rooms (`unsupported_texgen` and `tev_stages_over` at 0), and Windfall, the pirate ship and Molgera's arena draw bit for bit as before. The first finding follows. **Reproduced on the Mac, cause found.** A copy of a community save with its restart place moved into Dragon Roost Cavern (`scripts/save_set_restart.py`) shows the bug: the large map has only the door marker, while Dolphin with the same save draws the dark grid and the green rooms, and the corner minimap draws fine. Only the map screen logs draws that RecompCore truncates: 8 texture coordinate generators where it allows 5 (`kMaxTexGens`) and 12 TEV stages where it allows 8 (`kMaxTevStages`, GXRuntime `shader.hpp`; the GameCube allows 8 and 16, and the comments say the caps were the early-game maximum). Raising the TEV cap to 16 in a local test brought back part of the grid; raising the texture coordinate cap needs the fixed vertex layout (locations 0-13, `rawtex0..4`) widened. That is a RecompCore change for `bluewake-next`, with a one-time shader recompile for players. The sea chart and Charts screen draw correctly. |
| #65, #97 cutscene sound | On the Mac, the opening cutscene logs `cues=4 sounds=4 missing=0 silent=0.4s` with default settings, mouse camera off, Better Wind Waker on and 16:9 (no HD pack installed to try). None of them drops its sound on the Mac, so the cause is on Windows or in a later scene (the bird dropping Tetra); a Windows `[demo] end` line decides it. |
| Forsaken Fortress map and compass | No BlueWake patch touches dungeon items (only Tingle Chest markers and the new-game sea chart reveal), and the Mac shows the fortress minimap on the first visit with no patch involved. Most likely the original game; left as is. |
| #77 and Discord: `BlueWake.exe` does nothing on older CPUs | A message box now names the AVX2 requirement instead of a silent exit ([#117](https://github.com/chrissotraidis/bluewake/pull/117)); checked by compiling for Windows on the Mac, still to run under Intel SDE on Windows (task 2). |
| #61 8BitDo GameCube mod kit | Its Switch mode works with A and B swapped (Swap A and B is in `main`). Its generic mode needs an SDL mapping: BlueWake now loads `gamecontrollerdb.txt` from the folder with the saves ([#116](https://github.com/chrissotraidis/bluewake/pull/116)); the reporter was told how to try it with the next Windows build. |
| #126, Discord (Dale): no floor in the Wind Temple boss room | **Reproduced on the Mac, fixed in `main`'s next build.** Dale's log skips one draw a frame in `kazeB` (`array_unresolved`): Molgera's sand floor, which the boss module draws with arrays and a texture in its own data. The game passes the array through `OSCachedToPhysical` (`0xC06B0DA0` becomes `0x406B0DA0`) and the texture through SETIMAGE3's 24 bits (`0xC06A0DA0` becomes `0x006A0DA0`), so neither was found where BlueWake links modules. The host now reads both at the module's address (RecompCore patch 0156 for the texture). With a copy of a save moved into the room, the floor was black before and is sand after; Windfall and the pirate ship draw bit for bit as before. Still to check on Windows. |
| Discord (davioxx): PadMint stops at `generate_composite.py` with `write_text() got an unexpected keyword argument 'newline'` | macOS's own `python3` is 3.9, and that argument needs 3.10. The builder scripts write their files another way ([#127](https://github.com/chrissotraidis/bluewake/pull/127)); checked with `/usr/bin/python3` 3.9.6, which regenerates the same composite and mod variants. |
| Discord (SAUCE): Tingle Tuner | The Tuner needs a Game Boy Advance linked to the GameCube, which BlueWake doesn't emulate, so it isn't supported. Better Wind Waker's "Tingle Chests without the Tingle Tuner" (on by default) opens the chests with ordinary bombs. |
| Discord (Awesome): change the Xbox 360 controller's buttons | Controller button remapping and Swap A and B / Swap X and Y are in `main` and in the 0.5.0 draft (#66, #55); not yet pressed on a Windows PC with a controller. |

## Fixed in `main`, waiting for a Windows build

These close once a Windows build from `main` is out and the reporter confirms.

| Issue | What | Note |
| --- | --- | --- |
| #13 | Pictobox photo freezes the picture | Fixed on the Mac. |
| #55 | Switch Pro controller A and B reversed | Swap A and B, Swap X and Y. |
| #64 | Portable mode | `portable.txt` beside `BlueWake.exe` (#103). |
| #66 | Controller remapping | 0.4.0 has none, which is why Danither3al can't find it. |
| #71 | Jump and sprint always on | Off by default in `main`. |
| #73 | Camera turns the other way in water | #44. DonatelloEsq also sees it after talking to someone (#65). |
| #79 | 120 Hz Smooth Motion keeps switching | The log shows the game at 13.7 to 25.9 game frames a second on an Intel Arc handheld, so Smooth Motion stepped down as designed. The reporter says newer builds don't do it. |
| #58 | "Exact" sound crashes, then every launch crashes | The crash (a call to address 0) was fixed on October 1 (`ef29510`, RecompCore patch 0114), after the 0.4.0 download. A crash before the game runs is now also recovered on the next launch, with a plain message (#111). |

## Bugs

| Report | Where | What we know | Loop step |
| --- | --- | --- | --- |
| #65, #97, Discord (Xand3r, Dale, shargul, Aleximo, GinOkami428): music, Link's voice and effects missing in cutscenes; the intro story silent; Tower of the Gods rises silently | Windows 0.4.0 | The intro has sound for some players and not others. DonatelloEsq got the bird dropping Tetra back by turning off the HD texture pack, the Better Wind Waker options and mouse camera; Xand3r says 4:3 helps. On the Mac the opening cutscene logs `cues=4 sounds=4 missing=0`. | 8 |
| #74, Discord (Xand3r, Chris): dungeon map shows the room icons but not the floor drawing | Windows 0.3.0 and 0.4.0 | Seen in Forbidden Woods and Dragon Roost Cavern on every floor. Not yet tried on the Mac. | 7 |
| Discord (Dale): every sea chart shows the same island, changing as the story goes on | Windows | Dale had "Reveal the full sea chart" off, as far as he knows. | 7 |
| Discord (Xand3r, ∀˥∩⅁I˥∀Ɔ), #65: camera left and right inverts by itself; Controls settings revert | Windows 0.4.0 | 0.4.0 has two left-and-right invert settings, one under Controls and Better Wind Waker's under Mods, and they interact (`bluewake_game_options_invert_camera_x`). | 4 |
| Discord (shargul): Forsaken Fortress map and compass in the inventory from the start | Windows | Possibly a Better Wind Waker option. | 9 |
| #77, Discord (Rai): `BlueWake.exe` does nothing | Windows | DheikoGW's CPU has no AVX2; a clear message is Windows task 2. ImNotRyan01 hasn't said which CPU. | WINDOWS_TASKS |
| #76: soft lock in the Forsaken Fortress after a Moblin falls | Windows | Needs a Windows check; may be the original game. | WINDOWS_TASKS |
| #80: HD pack shading and orange hair on an AMD GPU | Windows | Asked whether the PNG pack does it too. | needs info |
| #61: 8BitDo GameCube mod kit controller not seen | Windows | Needs a Windows check. | WINDOWS_TASKS |
| #69: pirate ship flag has no texture | Windows 0.3.0 | Asked for a check on 0.4.0. The pirate ship's sail module (`d_a_sail`) keeps a texture in its own data, like Molgera's floor in #126, so the #126 fix may cover it; not seen on the Mac yet. | needs info |

## Slow scenes

Not in this loop. Recorded so the evidence isn't lost.

| Report | What we know |
| --- | --- |
| #86 (i7-6950X, RTX 3080), #59, #72 | #86's log: the game's GX thread is 92 to 95% busy and the game runs at 85% speed at sea; the GPU waits about 1 ms. The limit is on the CPU side of the renderer. #59 and #72 were asked to retest on 0.4.0. A closer read: 3,302 of #86's slow batches are large (about 100 KB of GX commands translated on the CPU); the other 238 are the end-of-frame display copy waiting about 21 ms a frame, at a 3840x2160 framebuffer. Smooth Motion was on and the game ran at its own 30. The Mac logs almost no slow batches in the same places. |
| Discord (Greatwhitedragon, Pharoah): 30 FPS on Steam Deck, 20 under Proton on a faster PC | Pharoah got steady speed by turning on the Steam Deck's CPU boost. Same CPU-side limit as #86. |

## Requests

| Request | Who | Loop step |
| --- | --- | --- |
| Bind the right mouse button (for example to B), attack with a click instead of K, change keyboard keys | Discord (ᏦOI TAIYO, umut) | 5 |
| Quit from the menu or with a controller | Discord (sliv) | 6 (the Mac menu has "Quit the game"; Windows doesn't) |
| Explain Brisk Sail and Unrestricted Boat | Discord (kaiiboraka) | 6 |
| Button remapping on Windows | Discord (Neo Drayk), #66 | Waiting for Windows |
| Ultrawide | #70, Discord (Nael) | Later |
| Hotkey to switch the Wind Waker HD and GameCube renderers | #108 | Later; the screenshot is from a newer Wind Waker Recomp build |
| Wind Waker HD effects and HD textures in the download | Discord (Aleximo, Pharoah, yunghiphopmaster) | Not in a BlueWake release yet |
| Rumble on by default | Discord (∀˥∩⅁I˥∀Ɔ) | Question |

## Not planned or waiting on others

| Issue | State |
| --- | --- |
| #56 Linux | jkoehler11's port is PR #107. |
| #75 Android | LiquidAzir's port is PR #93. |
| #60 European disc, #57 Wii U interface, #62 Switch, #48 Intel Mac | Not planned now. |
| #104 PadMint feedback | Open thread for players. |
