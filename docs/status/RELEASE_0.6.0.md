# BlueWake 0.6.0 release record

The working record for 0.6.0. The steps are in [GOAL_LOOP.md](../GOAL_LOOP.md). Fill in each check with the device,
the commit and what was seen. A row stays "not yet" until someone runs it.

## Candidate

| | |
| --- | --- |
| Version | 0.6.0, build 5 (`version.json`) |
| Candidate commit | `2c8a659416233864ab9d7a069e759bf5af558e2b` (merge of #170) |
| Release commit | The `main` commit tagged `v0.6.0`: the candidate plus #172 (intro fix on by default on Windows and Linux), #181 (iPhone and iPad first-screen text), the README download name (#174) and docs. The Windows zip was built from `4eb41f0` (#172's head); its game and app code is the same as the release commit's, because #181 only changes `apple/ios/src/first_run.m`. |
| RecompCore | `35e037f285ded1b766b7110783b47c976fe2ed10` (patches 0157 to 0159) |
| DolRecomp | `b8b534591cba8ca7cd43943a655ee6e2591cf5de` |
| Previous release | 0.5.0, `0d1f821`, October 5 |

## Release notes (draft, for the release page)

Fixes and changes since 0.5.0:

- Dungeon maps show their grid and rooms again (#74).
- The history intro after the title screen has its music on Mac, iPhone and iPad (#97). Windows: see below.
- Controllers that were detected but did nothing now play as player 1, including ones recognized through
  `gamecontrollerdb.txt` and a second controller left over after the first is unplugged (#61).
- Conducting with the Wind Waker on a controller is no longer mirrored (#156).
- The left stick has no big dead zone or sudden jump anymore, and reaches full speed near the end of its travel (#138).
- New option under Controls: invert the left stick's up and down while aiming in first person or with an item (#154).
- Touch controls are released when Apple's menus open (iPhone and iPad).
- Quitting is cleaner on every platform (a fix from the Linux port).
- Building from source works with Visual Studio 2022 again (#153).
- The session log names the controller mapping in use, to help with controller reports.

Windows: the intro-music fix is [on / off, after step 4]. To try it when it's off, start BlueWake from PowerShell with
`$env:BLUEWAKE_DEFER_DVD_COMPLETION="1"`.

## Checks

| Check | Platform and device | Commit | Result |
| --- | --- | --- | --- |
| App-only IPA, PadMint audit, release check | Mac (M3 Max) | `2c8a659` | Pass: `BlueWake-v0.6.0-ios-unsigned.ipa` reports 0.6.0 build 5 and has no Frameworks folder (no game module); PadMint 0.4.10 `audit` and `check_public_assets.sh` pass (0 address-named functions). The source zip (960 files) and the recipe (unchanged since 0.5.0, `check-manifest` ok) pass too. |
| Full PadMint build from an owned disc | Mac (M3 Max), PadMint 0.4.10 (`v0.4.10`), scratch `PADMINT_HOME` | `2c8a659` | Pass: `padmint make bluewake ios --ref main` with the new app-only IPA, fresh clone at the candidate, RecompCore `35e037f`. About 2 hours with 16 jobs (training about 26 minutes, compile about 72). The personal IPA reports 0.6.0 build 5 and holds the iOS module; it stays private. |
| Intro after title music, captured audio | Mac (M3 Max), headless, builder-configured host | `c82f375` (the candidate differs only in docs and `version.json`) | Pass: the default is on; `1tale.afc` plays through 3600; last 10 s 639,010 nonzero samples. A Windfall save loads (play scene at 609). All 72 host tests pass. |
| Intro after title music, with the PadMint-built module | iOS Simulator (iPad Pro 12.9), container backed up first | `2c8a659` | Pass: log shows `deferred completion=on (default)`; `1tale.afc` starts at 1953; last 10 s of a 59.8 s capture have 639,118 nonzero samples. The card and settings were unchanged afterwards. |
| Load a save, dungeon map, quit | iOS Simulator or a device | | Not run on Apple devices for this release; covered on the Mac host above |
| Windows build | Chris's PC (Ryzen 7 5700U), [results](WINDOWS_BUILD_0.6.0.md#results) | `4eb41f0` | Pass: clang 22.1.3 with the optimization profile, about 4 hours with local training; packaging allowlist passed (33 files). |
| Intro bug on players' Windows PCs (cause, not the fix) | Windows 10 (KTroopA9, #97) and Windows 11 i7-6500U (#159), both on 0.5.0 | 0.5.0 | Same cause as on the Mac: after the title music plays, `1tale.afc` reaches state 4 and stops 2 retraces later (1941 to 1943; 4620 to 4622). Starting a new file before the title music plays avoids it. 0.5.0 has no `BLUEWAKE_DEFER_DVD_COMPLETION`, so these logs do not test the fix; check a does. |
| Intro, fix on by default | Windows, Chris's PC | `4eb41f0` | Accepted by Chris from the log: `deferred completion=on (default)`, `1tale.afc` decoded to its last sample, no stream ended early in a 531-minute session, no fatal lines. Not separately confirmed by listening or by the exact wait-at-title route; the fix-off comparison was not run. |
| Dungeon map, save, reload, quit | Windows | | Not run (no card at the documented path). Shutdown counters `unsupported_texgen=0` and `tev_stages_over=0` for what was drawn. |
| Controller rows (player 1, baton, left stick, aim option) | Windows | | Not run |
| Windows intro default decided | | | On: #172 merged October 8 on Chris's acceptance of the Windows run above |
| iPhone and iPad app without game code, rebuilt for #181 | Mac (M3 Max) | release commit | Pass: reports 0.6.0 build 5, no game module, contains the new first-screen text; PadMint 0.4.10 `audit` passes (0 address-named functions). |
| Release check on all five assets | Mac | release commit | Pass. Windows zip SHA-256 matches the PC (`ed94d759…`); its only content-gate finding is `containsTranslatedGameCode: true`, the accepted one. Its `dsp/` files are RecompCore's public `Data/Sys/GC` replacements, as in 0.5.0. IPA, source zip and recipe pass; `check-manifest` ok. |
| Published | GitHub, by Chris | `806d65c` (tag `v0.6.0`) | October 8, 2026, marked latest. The public IPA and recipe match `SHA256SUMS`, and the Windows zip downloads from `releases/latest` at the right size. PadMint 0.4.10 `doctor bluewake --target ios` reports "recipe: bluewake v0.6.0 release" and Ready on the Mac. #74, #97, #61, #156, #138, #154 and #153 ask their reporters to confirm. |
