# BlueWake 0.13.0 release record

The record for 0.13.0: what it contains, the exact artifacts, and every check with its status. Previous release:
[0.12.0](RELEASE_0.12.0.md), November 15. Only Chris publishes.

**Status, November 22, 2026:** frozen at `bb790d4` (the freeze pull request #639). The Apple-side artifacts are in
the v0.13.0 draft; the Windows zip and the Linux AppImage are to be built on Chris's PC
([WINDOWS_BUILD_0.13.0.md](WINDOWS_BUILD_0.13.0.md)).

## The release

| | |
| --- | --- |
| Version | 0.13.0, build 14 (`version.json`) |
| Release commit | `bb790d4996f139a253b38e947d0cc4ea156f12d5`, the merge of the freeze pull request #639 |
| RecompCore | `c0f5f918026efe3168bdde42f3af0a55a3325e5e`: RecompCore#66's head, merged into `bluewake-next` as `7d3c18b1` (#65, #62, #63, #58, #59, #51, #64, #54 and a fix for #65 with #51; patches 0209-0218) |
| Previous release | 0.12.0, November 15 (`df9e46f`) |

## Artifacts

| Artifact | SHA-256 | Where it comes from | Content gate |
| --- | --- | --- | --- |
| `BlueWake-v0.13.0-ios-unsigned.ipa` | `06d8f8b14cb5d5682cac4f4a964d54ffe820b2c9ccd6784fb381d498b7631758` | `build.sh --app-only` on the Mac from a clean checkout of `bb790d4`; its provenance names `bb790d4`, unmodified, `containsTranslatedGameCode: false` | Pass; PadMint `audit` pass (0 address-named functions) |
| `BlueWake-v0.13.0-source.zip` | `20ba6974a8979a8e1455b7ab53b0a25b28c78774101b82f0d8bb727d4bd0c3ed` | `git archive --format=zip --prefix=bluewake-0.13.0/` of `bb790d4` | Pass; PadMint `audit` pass (11 address-named functions, as 0.12.0's) |
| `BlueWake-v0.13.0-padmint.json` | `f9dcb0d04324e1fa94bb2e2aceb27f78c90f21e9cc6bff15711922275e7b5195` | `padmint.json` at `bb790d4`, byte for byte 0.12.0's | Pass; PadMint `audit` pass |
| `BlueWake-v0.13.0-windows-x64.zip` | To come | Chris's PC, from `bb790d4` and his own disc | |
| `BlueWake-v0.13.0-linux-x86_64.AppImage` | To come | WSL with Docker on Chris's PC, the CI container | |
| `SHA256SUMS` | To come | From the files downloaded back from the draft | |

## What it contains

The contents pull request is #636; each change was reviewed on its own pull request.

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
| **Widescreen: 21:9 · 2560x1080 and 21:9 · 3440x1440** (#623; jkoehler11) | #619 | Every platform | Linux build at both presets (jkoehler11); generator test; Mac: frame buffers 2.3708 and 2.3896 wide, HUD at the edges | A real ultrawide panel |
| **No heart pulsing**, off (#628; jkoehler11) | #620 | Every platform | The site in the translation (jkoehler11); Mac: the hearts hold still with it on | |
| **Save state slots** (#632; irregularthanis) | | Windows, Mac, Linux | Steam Deck (irregularthanis) | The Mac and Windows |
| **The boat's zoom and Sail on Z**, Sail on Z off (#633, #634; irregularthanis) | | Every platform | Steam Deck (irregularthanis); the boat styles' distances against the disc | The Mac and Windows |
| **The Windows zip's stage select names** (#616; pdale-boop) | | Windows | A zip packaged on Windows (pdale-boop) | The release zip (Windows build, check g) |
| **Tools:** render_bench's paced and Game Mode runs (#590), the Android container build (#579) and stalled playbacks (#574), research notes (#549) | #578, #571, #545 | | Their own checks | |

Not in 0.13.0: #629 (changes requested), #584 (needs a rebase).

## Checks before publishing

| # | Check | Status | Evidence |
| --- | --- | --- | --- |
| 1 | Scope | Passed | Above; every new setting is off by default except #617's fix (checked on Windows by its author, frames unchanged on the Mac) and #625's (Smooth Motion only) |
| 2 | Windows tasks | To do | WINDOWS_BUILD_0.13.0.md |
| 3 | What changed for PadMint | Passed | `git diff v0.12.0 -- padmint.json` is empty; `check_padmint_catalog.py`: recipe and PadMint both offer macos-arm64, windows-arm64, windows-x86_64 |
| 4 | Artifacts | Apple-side passed; Windows and Linux to come | Above: `check_public_assets.sh` and PadMint `audit` on the IPA, the source zip and `padmint.json` |
| 5 | PadMint player check, with the checkout | Passed | PadMint `main` `bf95a5f`, Apple Python 3.9.6, an empty PadMint folder, a clean copy of `bb790d4`, the release IPA and the GZLE01 disc: passed in 9,002 s (2 h 30 min), with local training through the iPhone module, signing and packaging. The personal build was kept private and its temporary folder moved to the Trash |
| 6 | Player-facing text | To do after publishing | A README pull request names 0.13.0's downloads |
| 7 | `SHA256SUMS` | To do | From the draft's downloads, after the Windows and Linux files are uploaded |
| 8-12 | After publishing | To do | |

**Speed and exactness (Mac, M3 Max, headless, `--no-train` builds from one setup state):** 0.12.0 four runs, 0.13.0 five, interleaved;
all 31 checkpoints matched within and across versions, guest memory included. Means: Outset 115.09 → 113.73 (−1.2%),
Windfall 121.92 → 119.92 (−1.6%), Dragon Roost Cavern 177.41 → 178.03 (+0.4%), Forest Haven 97.22 → 99.18 (+2.0%), the
sea 88.91 → 91.07 (+2.4%); runs of one build spread 6-9%. The candidate was `36190741` (#636 with #629, off by default);
#629 was taken out afterwards.

