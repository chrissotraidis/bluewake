# BlueWake 0.12.0 release record

The record for 0.12.0: what it contains, the exact artifacts, and every check with its status. Previous release:
[0.11.0](RELEASE_0.11.0.md), November 8. 0.12.0 replaces the unpublished 0.11.1 draft ([RELEASE_0.11.1.md](RELEASE_0.11.1.md)),
whose two iPhone and iPad fixes it contains. Only Chris publishes.

**Status, November 16, 2026:** published by Chris on November 15 at 23:20 UTC from `df9e46f` (the `v0.12.0` tag), with the six artifacts below; `releases/latest` leads to it.

## The release

| | |
| --- | --- |
| Version | 0.12.0, build 12 (`version.json`) |
| Release commit | `df9e46fe0be5c0f95d46a3b194f59a4b96186640`, the merge of the freeze pull request #608 (November 14) |
| RecompCore | `2159e88b10f1d4c77cfb67041382b623b4462205`: RecompCore#61's head, merged into `bluewake-next` as `b16f5027`; `bluewake-next` with #60 and #61 |
| Previous release | 0.11.0, November 8 (`0d84fa1`) |

## Artifacts

The Apple-side artifacts were made on November 15 from a clean checkout of `df9e46f`; the Windows zip and the AppImage on
Chris's PC from the same commit and his own disc ([results](WINDOWS_BUILD_0.12.0.md#results)). All six were downloaded back
from the draft for the checks below; GitHub's digests match.

| Artifact | SHA-256 | Where it comes from | Content gate |
| --- | --- | --- | --- |
| `BlueWake-v0.12.0-ios-unsigned.ipa` | `103aeaf9284b58586569b1fa5073f80092cb70cf2ba14d2e6daf74ea5aa58ce9` | `build.sh --app-only`; its provenance names `df9e46f`, unmodified, `containsTranslatedGameCode: false` | Pass; PadMint `audit` pass (0 address-named functions) |
| `BlueWake-v0.12.0-source.zip` | `4f87c96aca91a4d586ecb797878137da8cd483d1138bc34b376e9c5f66169c9a` | `git archive --format=zip --prefix=bluewake-0.12.0/` of `df9e46f` | Pass; PadMint `audit` pass (11 address-named functions, as 0.11.0's) |
| `BlueWake-v0.12.0-padmint.json` | `f9dcb0d04324e1fa94bb2e2aceb27f78c90f21e9cc6bff15711922275e7b5195` | `padmint.json` at `df9e46f`, byte for byte 0.11.0's | Pass; PadMint `audit` pass |
| `BlueWake-v0.12.0-windows-x64.zip` | `3ff3915f5c893539a0ce9f1052d769d8c984826b140c907a61166508e6dc58b9` | Chris's PC, clang 22.1.3, local training; module `59f1231f…` | The only finding is the accepted `containsTranslatedGameCode: true`. The same 35 members as 0.11.0's; no disc, saves, cards, keys, `nodtool.exe` or signing files; `dsp_rom.bin` identical to 0.11.0's open replacement |
| `BlueWake-v0.12.0-linux-x86_64.AppImage` | `d1cc82d1e173719f1da41cabbee53a18227c35c44498134fe4319768acd60a6a` | WSL with Docker on Chris's PC, the CI container (clang 22.1.8), from `df9e46f` and the same disc | Unpacked with `7zz` on the Mac: 44 files, 0.11.0's plus #585's four `usr/bin/stage_select` files. The gate's only finding is `usr/bin/gGZLE01_recomp.so`'s 592 address-named symbols (the translated game code, accepted); the 43 other files pass. GLIBC 2.38 at most |
| `SHA256SUMS` | `ca8a94443a4b1d2e574445634011f31bf1d927737116abc8d96d21d2da9dbf9c` | The five lines above, from the files downloaded from the draft; `shasum -c` passes | Pass |

## What it contains

| Change | Issue | Ships to | Checked | Not checked by hand |
| --- | --- | --- | --- | --- |
| **The distance blur blurs only the distance** (#605, RecompCore#61; pdale-boop): the depth copy read as Z16; Display › Distance blur and Distance blur starts, starting as the game has them | #598 | Every platform (the settings on the desktop) | Windows by hand (pdale-boop); Mac rendered: 0.11.0 softens Link at 1x, the settings move the blur as described; Windows rendered at 3x | Sailing zoomed in, by hand, on the release build |
| **Module art linked high** (#583, pdale-boop): the Earth Temple's banners and the pirate ship's sail | #545 | Every platform | Linux and Windows (pdale-boop); Mac and the Windows release build: banners and (Mac) sail fixed, the rest of each frame the same | iPhone, iPad |
| **Native cutscene framing**, off by default (#599, pdale-boop) | #597 | Every platform | Windows and Linux (pdale-boop); Mac: no bars in 15 frames of "Tetra falls" at 16:9, bars with it off | The Windows release build (its warp didn't start the cutscene) |
| **Dolphin saves in the desktop menu** (#604, pdale-boop) | #600, #508 | Windows, Mac, Linux | Windows by hand (pdale-boop); Windows release build: export, import into an empty quest log, backup, restart, play | The Mac (automation couldn't drive the menu), a `.raw` card |
| **Save states:** a state taken with the stage select off loads in play (#582, pdale-boop) | | Every platform | Mac (0.11.0 against 0.12.0 with a state 0.11.0 saved) and the Windows release build | |
| **Shorter Triforce quest**, off by default (#589, Maxlemoar) | #152 | Every platform | Mac: the option loads, its hook runs only behind its flag | A real chart chest; the chest list against Wind Waker HD |
| **Show Touch Controls with a controller** (#601) and **SDL3's scene geometry** (#602, RecompCore#60) | | iPhone, iPad | See [RELEASE_0.11.1.md](RELEASE_0.11.1.md); the release IPA carries both | A physical iPad with a controller, a TV |
| **Windows builder:** the `translated.new` rename retried while antivirus has the files open (#596) | padmint#255 | Windows (PadMint's iPhone route and the desktop builder) | Windows ARM64 test; the release build went past translate without `WinError 5` | The reporter's PC |
| **Linux:** the AppImage's stage select names (#585), first-run Cancel (#540, jkoehler11) | #523 | Linux | Names: the release AppImage's match `config/stage_select`; Cancel: jkoehler11 on Linux | Cancel on the release AppImage |

## Checks before publishing

| # | Check | Status | Evidence |
| --- | --- | --- | --- |
| 1 | Scope | Passed | Above |
| 2 | Windows tasks | Passed, with gaps named | [WINDOWS_BUILD_0.12.0.md](WINDOWS_BUILD_0.12.0.md#results): a–f, the speed tour, the AppImage. The sail, the cutscene bars and hands-on play not checked on Windows; the Mac covers the first two |
| 3 | What changed for PadMint | Passed | `git diff v0.11.0 -- padmint.json` is empty; `check_padmint_catalog.py`: recipe and PadMint both offer macos-arm64, windows-arm64, windows-x86_64 |
| 4 | Artifacts | Passed | Above |
| 5 | PadMint player check, with the checkout | Passed | PadMint `main` `de14bd3` (0.6.5), Apple Python 3.9.6, an empty PadMint folder, a clean clone of `df9e46f`, the release IPA and the GZLE01 disc: passed in 8,980 s (2 h 30 min), with local training |
| 6 | Player-facing text | Passed | #611 merged after publishing: names 0.12.0's downloads and drops #609's "not released yet" |
| 7 | `SHA256SUMS` | Passed | From the draft's downloads; `shasum -c` passes |
| 8–12 | After publishing | Below | |

## After publishing

| Check | Status |
| --- | --- |
| The tag | `v0.12.0` points at `df9e46fe0be5c0f95d46a3b194f59a4b96186640`; `releases/latest` is v0.12.0 (published by Chris, November 15 at 23:20 UTC) |
| Every asset downloads without signing in and matches `SHA256SUMS` | Passed: all six through `releases/latest/download` with no token (HTTP 200); all five lines of `SHA256SUMS` OK |
| README and guides name 0.12.0's downloads | #611, merged after publishing |
| The 0.11.1 draft | Deleted after publishing, with Chris's approval: 0.12.0 contains its two fixes. It never had a tag |
| Replies | Asked to confirm: #598, #597 (reopened: #599's "Fixes" closed it at merge), #600, #152, #69, #523; #545 updated and left open for #584 |
| PadMint player check without a checkout, from `releases/latest` | Passed on 16 November after disk space was freed: `scripts/player-check.sh bluewake ios GZLE01.iso`, PadMint main `3bbf6c0` (0.6.6), Apple Python 3.9.6, an empty PadMint folder and no checkout. The public v0.12.0 recipe passed its catalog audit, then the full local-training, iPhone module, signing and packaging path passed in 84.7 minutes (build stage 5,068.52 seconds). The personal build was kept private and its temporary folder removed. This is build evidence, not new physical-device acceptance |
| PadMint offers what the release builds | `check_padmint_catalog.py` passes against PadMint `main`: recipe and PadMint both offer macos-arm64, windows-arm64, windows-x86_64. No catalog change was needed (`padmint.json` is 0.11.0's) |
| Reports since publishing | Read on November 22 (every issue and comment updated since November 15, and PadMint's issues). None is a problem with the 0.12.0 release. #619: 21:9 leaves 40-pixel bars on a 3440x1440 panel, because 21:9 is 2.3333 and the panel 2.3889; not a regression, and #623 adds presets for 2560x1080 and 3440x1440. #620: requests (no heart pulsing, a speed-up key), #628 and #629. #621: a freeze leaving Dragon Roost Cavern's room 9 on a personal Mac build of 0.12.0, a zeldaHeap block header overwritten 4 bytes past a `d_a_door10`; cause unknown, not shown to be new in 0.12.0; the reporter offers to test. #622: Android 17 dies at the first swapchain inside Dawn (a contributor's own build; Android has no download). #624: a question about the camera stick, to answer. #631: a question about AGENTS.md's attribution rule, answered and closed. padmint#255 (#596's WinError 5, in 0.12.0): no reply yet. Asked to confirm on November 15 and not yet answered: #598, #597, #600, #152, #69, #523 (14 days on November 29) |


**Speed and exactness (Windows, headless, from 0.11.0's tour state):** four sets of four runs; every run loaded all five
stops and all 31 checkpoints matched within and across versions, guest memory included. The last set, 0.11.0 against
0.12.0: Outset 63.49 → 60.78 (−4.27%), Windfall +1.29%, Dragon Roost Cavern −1.68%, Forest Haven −1.10%, the sea −0.19%.
The 3% gate was waived by Chris, not passed; the cause isn't established (this PC's runs varied about 6% at a stop for
0.10.0). Nothing in 0.12.0 that is on by default changes the game's code paths beyond the watched addresses #599 adds.

**The Mac candidate:** `26c40f98` (#607's head, the same tree as the freeze but for `version.json` and comments), built
from Chris's disc with `--no-train`, compared against 0.11.0's freeze build from copies of Chris's card `4b54ec25…`;
the details are in #607.
