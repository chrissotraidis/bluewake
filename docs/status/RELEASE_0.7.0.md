# BlueWake 0.7.0 release record

The working record for 0.7.0. The steps are in [GOAL_LOOP.md](../GOAL_LOOP.md); the Windows build is
[WINDOWS_BUILD_0.7.0.md](WINDOWS_BUILD_0.7.0.md). Fill in each check with the device, the commit and what was seen. A
row stays "not yet" until someone runs it. 0.7.0 replaces the 0.6.1 planned for October 9: once lean block copies
became the default, the release was more than fixes.

## Candidate

| | |
| --- | --- |
| Version | 0.7.0, build 6 (`version.json`, set at the freeze) |
| Candidate commit | `c6094ec76c82700d5a56a28bf627c8179f686542` (#229, October 10): what the Windows build and the IPA were built from |
| Release commit | `66b33becc84edc425fd6185ec22bd0b0e3673afb` (#239): the candidate's code plus the fixed PadMint recipe and docs. The draft targets it. |
| RecompCore | `ddd031b2a7f45fcbda37a49daac9b308b8d95da1` (patches 0160 to 0168) |
| Previous release | 0.6.0, October 8 |

## What it contains

| Change | Issue | Ships to | Checked so far | Still to check |
| --- | --- | --- | --- | --- |
| **Faster:** lean block copies on by default, 5 to 12% (Elliott Tate's original, #196, #220; measured by pdale-boop, #208) | #59, #137, #159 | Windows, Linux builders | Windows: 15½ min of play; Linux i5-6500: 9.9% fewer instructions | The release build |
| **Faster:** draw fusion, a tenth of the draws (Elliott Tate, #225, patch 0166) | #86, #137 | Windows (on); Mac, Linux (off until played) | Elliott on Windows: 30 to 60% more frames on four E-cores, positions identical | The release build played |
| **Faster:** single-precision loads take the hardware's widening for normal floats (Elliott Tate, #228) | #59 | Windows, Linux builders | 0 mismatches over all 2^32 patterns (x86 by Elliott, arm64 here) | The release build |
| **Faster on Linux:** the module compiles twice as fast and runs up to 10% faster on slow cores (pdale-boop, #218) | #215 | Linux builder | Identical checkpoints at 468 places | Steam Deck |
| Smooth Motion keeps its frames on CPUs with 8 threads or more (#224, patch 0164) | #137 | Every desktop | CI | Smooth Motion at 120 on an 8-thread PC |
| The log names the renderer; Linux says when it fell back to slow OpenGL (#224, patch 0163) | #56 | Every desktop | CI | Linux without Vulkan |
| A rare crash (`unmapped pc=0x8180fff0`) fixed (pdale-boop, #211) | none | Every platform | Windows: the crashing save and the stage select's places run | A long session |
| Controllers plugged in at launch get the smooth stick and player 1 (pdale-boop, #195) | #138, #155 | Windows, Mac, Linux | pdale-boop, two controllers | Mac |
| Player 1 goes to the controller you press while player 1 is idle (an adapter's empty ports) (#222) | #155 | Windows, Mac, Linux | CI test | A Mayflash adapter |
| Rumble only on player 1's controller (#222) | #190 | Windows, Mac, Linux | CI | Two controllers |
| Pictobox: the left stick works at "keep this picture?" and in the gallery (#223) | #191, #13 | Windows, Mac, Linux | CI | A photo with a controller |
| Optional controller camera zoom: hold the right-stick click and move it up or down (cforain, #221) | #203 | Windows, Mac, Linux | cforain on Linux | Windows |
| The FPS counter's position under Display (saulob, #106) | none | Windows, Mac, Linux | Windows 11 by saulob | Mac menu |
| Windows: the window opens in place (saulob, #197); portable mode keeps remaps in `user` (#184) | #89, #64 | Windows | CI | A Windows PC |
| Building your own copy: training 30 → 12 minutes (pdale-boop, #202); a second build into the same folder works (#209) | #59 | Windows, Linux builders | i5-12600KF, i5-6500 | The release build |
| Linux: an AppImage with a first-run disc setup (cforain, #226), checked for SteamOS's glibc in CI (#217) | #56, #214 | Linux | cforain on Gentoo and a Steam Deck | A contributor's release AppImage |

Not in 0.7.0: draw fusion on Mac and Linux by default (until played there), Elliott's other work since October 3
(being ported, [PERFORMANCE.md](../PERFORMANCE.md#the-plan)), Android (#93).

## Release notes (draft, for the release page)

BlueWake 0.7.0 is faster, especially on older and smaller CPUs, and fixes several controller problems. Every version
needs your own USA (GZLE01) disc image. Nothing from the game is included except in the ready-made Windows and Linux
builds.

- **Faster.** Elliott Tate's lean block copies are now the default: 5 to 12% more speed, most on slower CPUs (measured
  by pdale-boop). On Windows, his draw fusion sends a tenth as many draws, so the game no longer waits on graphics in
  busy places like Forest Haven and Hyrule.
- **Smoother.** With Smooth Motion on, CPUs with 8 threads or more no longer drop to 30 for seconds after a hitch.
- **Controllers.** A controller already plugged in at launch gets the smooth stick (#138). With a GameCube adapter,
  press a button on your controller and it becomes player 1, even if it's not in port 1 (#155). Rumble only shakes the
  controller you're playing with (#190). The left stick works at the Pictobox's "keep this picture?" (#191). New,
  optional: hold the right-stick click and move it up or down to zoom the camera (thanks to cforain).
- **Fixed:** a rare crash that could stop the game anywhere (`unmapped pc=0x8180fff0`), tracked down by pdale-boop.
- **Windows:** the window opens where you left it (thanks to saulob), portable mode keeps everything in its folder,
  and you can place the FPS counter in a corner.
- **Linux:** an AppImage that asks for your disc the first time and can add itself to your menu (thanks to cforain
  and jkoehler11). If you build your own, the game compiles about twice as fast now (thanks to pdale-boop).

Thanks to Elliott Tate, pdale-boop, jkoehler11, cforain, saulob and LiquidAzir.

## Checks

| Check | Platform and device | Commit | Result |
| --- | --- | --- | --- |
| Windows build from the commit ([WINDOWS_BUILD_0.7.0.md](WINDOWS_BUILD_0.7.0.md)) | Ryzen 7 5700U, Windows (Chris's PC) | `9422bd4` (code = `c6094ec`) | Pass: 92 minutes with 6 jobs (0.6.0: 4 hours), clang 22.1.3, `lean prepaid block copies: 443155 blocks in 813 chunks`. A 27-minute title session with `[gx] draw fusion on`, `[renderer] D3D12`, 102 million draws fused and no fatal lines. |
| Windows play: Outset, Forest Haven, Dragon Roost, with fusion on (and `DOL_GX_FUSE=0` for comparison) | | | Not run (skipped at Chris's direction). Not a pass. |
| Windows controllers: at launch, rumble, Pictobox prompt, zoom | | | Not run. |
| Linux AppImage from a contributor's own disc, then the release check | | | not yet |
| Mac: ten minutes of play with a controller connected at launch | | | not yet |
| App-only IPA, PadMint audit, release check | Mac (M3 Max) | `c6094ec` (built from #229's branch, the same tree) | Pass: `BlueWake-v0.7.0-ios-unsigned.ipa` (5.4 MB, SHA-256 `8395fa00…61bf4`) reports 0.7.0 build 6 and holds no game module; PadMint 0.4.9 `audit` (0 address-named functions) and `check_public_assets.sh` pass. Uploaded to the v0.7.0 draft. |
| Source zip and recipe | Mac | `66b33be` | Pass (above). |
| `check_public_assets.sh` on every asset | Mac (M3 Max) | `66b33be` | Pass. Windows zip: SHA-256 matches the PC (`1880822f…3957c`); 33 files, the same layout as 0.6.0; the gate's only finding is `containsTranslatedGameCode: true`, the accepted one. IPA, source zip (1,006 files) and recipe pass. All five files and `SHA256SUMS` are on the v0.7.0 **draft**; GitHub's digests match. |
| PadMint recipe | Mac, PadMint 0.4.12 and 0.4.10 | `66b33be` | Pass after #239. At `c6094ec` every PadMint refused it ("needs a newer PadMint (it uses cc)"): the Linux target listed host compilers PadMint doesn't provide, which would have stopped every iPhone and iPad build. |

