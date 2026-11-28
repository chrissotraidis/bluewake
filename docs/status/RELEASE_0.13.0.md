# BlueWake 0.13.0 release record

The record for 0.13.0: what it contains, the exact artifacts, and every check with its status. Previous release:
[0.12.0](RELEASE_0.12.0.md), November 15. Only Chris publishes.

**Status, November 28, 2026:** frozen again as **build 15** (the freeze pull request "Freeze 0.13.0, build 15"). Build 14
(`bb790d4`, November 22) was never published. Its draft's files were made, and the Windows and Linux ones checked
([WINDOWS_BUILD_0.13.0.md](WINDOWS_BUILD_0.13.0.md#results-build-14-not-published)), but by then `main` had the fix for
the door freeze (#621) and more contributor work. Chris decided to keep the version number and rebuild every file from
build 15. The draft's build 14 files are replaced, and nothing from build 14 ships.

**November 28, evening:** the Apple-side files are in the draft and passed the release gate, PadMint's `audit` and its
player check. Next: the Windows zip and the AppImage on Chris's PC
([WINDOWS_BUILD_0.13.0.md](WINDOWS_BUILD_0.13.0.md), build 15), then `SHA256SUMS`, then Chris publishes.

## The release

| | |
| --- | --- |
| Version | 0.13.0, build 15 (`version.json`) |
| Release commit | `c4167ab72285ef3fd55e115066c58910510fe0e2`, the merge of the freeze pull request #667 |
| RecompCore | `d733290f1d47acc12a464c77de5e50099b680890`: RecompCore#70's head, on `bluewake-next` after RecompCore#66 (`7d3c18b1`). #66 is #65, #62, #63, #58, #59, #51, #64 and #54 with a fix for #65 with #51 (patches 0209-0218). #70 is #68 with #67, then #69 (patches 0219-0221) |
| Previous release | 0.12.0, November 15 (`df9e46f`) |

## Artifacts

| Artifact | SHA-256 | Where it comes from | Content gate |
| --- | --- | --- | --- |
| `BlueWake-v0.13.0-ios-unsigned.ipa` | `0250ae0e4f3cd2984989d4cd0a9f239da0e3ef8c74eed13cb0f6e612f2f7ee92` | `build.sh --app-only` on the Mac into a fresh build folder from a clean checkout of `c4167ab` (765 build steps); its provenance names `c4167ab`, unmodified, `containsTranslatedGameCode: false`; RecompCore `d733290` | Pass; PadMint `audit` pass (0 address-named functions) |
| `BlueWake-v0.13.0-source.zip` | `1dad9514304a1395b884c48f3fe6b35faf4f63dd08ceff11a15aa4832b60b0a0` | `git archive --format=zip --prefix=bluewake-0.13.0/` of `c4167ab` (21 MB; build 14's was 11 MB, the difference is #655's, #658's and #661's pictures in `docs/planning`) | Pass; PadMint `audit` pass (11 address-named functions, as 0.12.0's) |
| `BlueWake-v0.13.0-padmint.json` | `f9dcb0d04324e1fa94bb2e2aceb27f78c90f21e9cc6bff15711922275e7b5195` | `padmint.json` at `c4167ab`, byte for byte 0.12.0's | Pass; PadMint `audit` pass |
| `BlueWake-v0.13.0-windows-x64.zip` | To come | Chris's PC, from the release commit and his own disc | |
| `BlueWake-v0.13.0-linux-x86_64.AppImage` | To come | WSL with Docker on Chris's PC, the CI container | |
| `SHA256SUMS` | To come | From the files downloaded back from the draft | |

## What it contains

Two contents pull requests: #636 (November 22) and #666 (November 28). Each change was reviewed on its own pull request.
The six merged to `main` in between are listed with them.

| Change | Issue | Ships to | Checked | Not checked by hand |
| --- | --- | --- | --- | --- |
| **Heat haze in Smooth Motion** (#615, RecompCore#62; pdale-boop): the haze stays with the scene when the camera turns | | Every platform, with Smooth Motion | Windows by hand (pdale-boop); Steam Deck and a Linux desktop at 60 and 240 fps (irregularthanis); Mac: real frames identical to 0.12.0's in Dragon Roost Cavern with Smooth Motion at 120 | The haze on a Mac (the captured view had none) |
| **The Picto Box in rooms with heat haze** (#617, RecompCore#63; pdale-boop) | | Every platform (on for everyone) | Windows by hand, Smooth Motion on and off (pdale-boop); Mac: frames with the defaults pixel-identical to 0.12.0's at four places | The Picto Box on a Mac |
| **The heart in Smooth Motion** (#625, RecompCore#64; pdale-boop) | | Every platform, with Smooth Motion | Windows by hand at 200 Hz (pdale-boop); Mac: in-between frames in Dragon Roost Cavern, 0.12.0's last heart oversized, 0.13.0's not | 240 Hz |
| **No shading jumps with the HD look in Smooth Motion** (#594, RecompCore#58; pdale-boop) | | Every platform, with Smooth Motion and the HD look | Windows at 180 Hz, Linux at 60 Hz (pdale-boop) | The Mac |
| **Step down (experimental)**, off by default (#595, RecompCore#59; pdale-boop) | | Every platform | Windows at 120 and 180 Hz, Linux at 60 Hz (pdale-boop) | The Mac, a screen without VRR |
| **The HD look: HD's sunlight, cloud shadows off by default, Windfall's arch, light on characters; Wind Waker HD's own light and shadows** (experimental, off) (#533, RecompCore#51; pdale-boop) | | Every platform (the HD look stays off by default) | Windows and Linux against HD in Cemu (pdale-boop); Steam Deck with #586 (irregularthanis); Mac: a pack imported from an HD dump with this build's importer (121 stages, 307 object models); the HD look, HD's sky and HD's own light run, Link's sun shadow and HD's hair shade show | Against HD on a Mac; the menu's importer |
| **Config › Performance: four switches**, off (#586, RecompCore#54; irregularthanis) | #215 | Every platform (two for Linux and gamescope) | Steam Deck Game Mode, seven scenes, +9.1% (irregularthanis); Android (jkoehler11): no gain there | The Mac |
| **Mods › HD textures: the count, Keep the game's shading tables, Save the textures the pack lacks** (#627, RecompCore#65; Maxlemoar) | #336 | Windows, Mac, Linux | Mac rendered (Maxlemoar); the batch's fix for #65 with #51 compiled on Windows CI | Windows by hand |
| **Widescreen: 21:9 · 2560x1080 and 21:9 · 3440x1440** (#623; jkoehler11) | #619 | Every platform | Linux build at both presets (jkoehler11); generator test; Mac: frame buffers 2.3708 and 2.3896 wide, HUD at the edges; Windows build 14: the bars in a 16:9 window measured | A real ultrawide panel |
| **No heart pulsing**, off (#628; jkoehler11) | #620 | Every platform | The site in the translation (jkoehler11); Mac: the hearts hold still with it on; Windows build 14: absent in all 42 samples | |
| **Save state slots** (#632; irregularthanis) | | Windows, Mac, Linux | Steam Deck (irregularthanis); Windows build 14, automated menu input | The Mac |
| **The boat's zoom and Sail on Z**, Sail on Z off (#633, #634; irregularthanis); the boat zoom leaves the game's camera data alone while it's off (#643) | | Every platform | Steam Deck (irregularthanis); the boat styles' distances against the disc | The Mac and Windows |
| **The Windows zip's stage select names** (#616; pdale-boop) | | Windows | Windows build 14's zip | |
| **No freeze or crash after a door into a new area with Native cutscene framing on** (#653; CallNous, Chris) | #621, likely #657 | Every platform, with Native cutscene framing (Mods, off by default) | CallNous on a MacBook Air M4 with a heap watch, before and after; the held trim is dropped when the camera changes | #657 on a build with the fix (asked on the issue) |
| **Speed up while Tab is held**, off, 1.5x to 5x (#629; jkoehler11) | #620 | Every platform, with a keyboard | Mac candidate `36190741`: 1,800 retraces with 600 at 3x took 22 s against 29 s; Linux (jkoehler11): 1.95x over a run, with a test hook holding the key | Tab pressed by hand on any platform |
| **Distance blur starts** reads 1.00x to 10.00x in quarters (#651; pdale-boop) | #598 | Every platform | Settings tests; Windows 11: a saved 57 opened at 3.99x | The steps on screen on a Mac |
| **The gamescope switch on Linux only** (#642; irregularthanis) | | Linux | Steam Deck (irregularthanis); settings tests | |
| **Swift Sail: a steadier boat** (#662; irregularthanis) | | Every platform, with Better Wind Waker's Swift or Brisk Sail | Steam Deck by the owner, with and without Swift and Brisk Sail; `sail_steady_test` | The Mac and Windows |
| **Faster Light distance blur** and **Faster effects mask**, off (#655, #658, RecompCore#67, #68; irregularthanis) | #215 | Every desktop | Steam Deck Game Mode: +4% over ten scenes (+7% in a rainy Windfall); the mask −10% GPU time and +5.6% game fps in the Big Octo fight | The Mac and Windows |
| **Edge smoothing**, FXAA or SMAA, off (#661, RecompCore#69; irregularthanis) | | Windows, Mac, Linux | Steam Deck: each level measured and pictured against plain 2x | The Mac and Windows |
| **Android 10 and 11: the ⋯ menu opens** (#665; jkoehler11) | #664 | Android (built by the player) | Android lint's NewApi check and a compile | An Android 10 or 11 phone |
| **Linux performance panel on an integrated GPU** (#649; jkoehler11) | #535 | Linux | Unit tests with a prepared sysfs tree | A real APU (the Deck) |
| **Tools:** the Windows training DLL within the loader limit (#646), render_bench's paced and Game Mode runs (#590), the Android container build (#579) and stalled playbacks (#574), research notes (#549), the native generators' inline frame form (#654) and their Linux test harnesses (#660) | #578, #571, #545, #451 | | Their own checks; #646 on Windows build 14 | |

Not in 0.13.0: #648 (the menu in other languages; not yet seen on a screen), #626 (the Steam library shortcut; its
Deck acceptance run is to come), #559 (the Android panel; not run on a phone), #584 (needs a rebase, and hook id 8),
and the drafts #551, #562, #618, #342 and #347.

## Checks before publishing

| # | Check | Status | Evidence |
| --- | --- | --- | --- |
| 1 | Scope | Passed | Above; every new setting is off by default except #617's fix (checked on Windows by its author, frames unchanged on the Mac), #625's (Smooth Motion only), #651's slider labels, #649's panel and #665's Android fix |
| 2 | Windows tasks | To do | WINDOWS_BUILD_0.13.0.md, build 15 |
| 3 | What changed for PadMint | Passed | `git diff v0.12.0 c4167ab -- padmint.json` is empty; `check_padmint_catalog.py`: the recipe and PadMint both offer macos-arm64, windows-arm64, windows-x86_64 |
| 4 | Artifacts | Apple-side passed; Windows and Linux to come | Above: `check_public_assets.sh` (the maintainer's release gate) and PadMint `audit` (PadMint `main` `087a083`, Apple Python 3.9.6) on the IPA, the source zip and `padmint.json`. Build 14's five files were deleted from the draft and these three uploaded on November 28; GitHub's SHA-256 digests match. The draft targets `c4167ab` |
| 5 | PadMint player check, with the checkout | Passed | PadMint `main` `087a083`, Apple Python 3.9.6, an empty PadMint folder, a clean checkout of `c4167ab`, the release IPA and the GZLE01 disc: passed in 10,558 s (2 h 56 min, November 28, with other builds and an Android emulator running on the same Mac), with local training through the iPhone module, signing and packaging. No catalog changes were listed. The personal build was kept private and its temporary folder moved to the Trash |
| 6 | Player-facing text | To do after publishing | #641 names 0.13.0's downloads |
| 7 | `SHA256SUMS` | To do | From the draft's downloads, after the Windows and Linux files are uploaded |
| 8-12 | After publishing | To do | |

**Build 15 on the Mac (M3 Max), November 28:** a personal `--no-train` build of #666 (the release commit's code;
#655's planning notes and the freeze's docs came after) from Chris's disc. It compiled RecompCore `d733290` with
Metal, and a rendered warp to Outset (`sea:44:0`) arrived and exited 0. Against build 14's candidate `36190741`, built
the same way, seven headless tours loaded the same setup state (old, new, new, old, old, new, old). All 31 checkpoints
matched in every pairing, guest memory included, so the defaults compute exactly what build 14 did. The speed numbers
aren't a result: the Mac was busy with other work for part of the session (two of the old runs show the game thread
busy only 62-80% at some stops). The one adjacent pair taken without that (runs 6 and 7) differs by −2.7% to +1.1% per
stop. The rendered picture comparison at four places was stopped after one run because it kept opening the game's
window on Chris's screen. The new switches weren't tried on the Mac.

## Build 14 (not published)

Frozen November 22 at `bb790d4` (#639). Its Apple-side files were made and passed the content gate and PadMint's
`audit`. PadMint's player check passed with the checkout (PadMint `main` `bf95a5f`, Apple Python 3.9.6, 9,002 s).
The Windows zip and the AppImage were built from `314b7e8a` (the freeze with #646) and checked
([results](WINDOWS_BUILD_0.13.0.md#results-build-14-not-published)). None of them is published.

**Speed and exactness (Mac, M3 Max, headless, `--no-train` builds from one setup state):** 0.12.0 four runs, build 14's
candidate five, interleaved; all 31 checkpoints matched within and across versions, guest memory included. Means:
Outset 115.09 → 113.73 (−1.2%), Windfall 121.92 → 119.92 (−1.6%), Dragon Roost Cavern 177.41 → 178.03 (+0.4%), Forest
Haven 97.22 → 99.18 (+2.0%), the sea 88.91 → 91.07 (+2.4%); runs of one build spread 6-9%. The candidate was
`36190741` (#636 with #629, off by default).
