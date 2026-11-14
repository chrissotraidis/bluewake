# BlueWake 0.11.1 release record

The record for 0.11.1: what it contains, the exact artifacts, and every check with its status. Previous release:
[0.11.0](RELEASE_0.11.0.md), November 8. Only Chris publishes.

**Status, November 14, 2026:** frozen; the Apple-side artifacts are built, gated and on the v0.11.1 draft with
`SHA256SUMS`. Open before publishing: PadMint's player check with the release checkout (line 5), a decision on
Windows and Linux builds, and a physical TV and controller check on an iPad.

## The release

| | |
| --- | --- |
| Version | 0.11.1, build 11 (`version.json`) |
| Release commit | `a41f80c486395ec09f1cdd5cdfa8ec091e3bd925`, the merge of the freeze pull request #603 (November 14) |
| RecompCore | `82ceadf3`: `bluewake-next` with RecompCore#60 (SDL3 UIKit scene geometry) |
| Previous release | 0.11.0, November 8 (`0d84fa1`) |

## Artifacts

Made on November 14 from a clean checkout of `a41f80c` and uploaded to the v0.11.1 draft.
`scripts/release/check_public_assets.sh` passed each with the maintainer's release gate, and PadMint `audit`
(`main` `93ace90`, 0.6.5) passed each.

| Artifact | SHA-256 | Where it comes from | Content gate |
| --- | --- | --- | --- |
| `BlueWake-v0.11.1-ios-unsigned.ipa` | `18fc8470ede63ac6b39a8ea512899fa27f455854be48e994a33b5a836700d1a4` | `build.sh --app-only` from a clean checkout of `a41f80c`; its provenance names `a41f80c`, unmodified, `containsTranslatedGameCode: false`; no game module | Pass; PadMint `audit` pass (0 address-named functions) |
| `BlueWake-v0.11.1-source.zip` | `d182aac4edbb40f864eb621655955b99bc18afc74b37320c0f1bbe69d70c45e5` | `git archive --format=zip --prefix=bluewake-0.11.1/` of `a41f80c` | Pass; PadMint `audit` pass (11 address-named functions, as 0.11.0's) |
| `BlueWake-v0.11.1-padmint.json` | `f9dcb0d04324e1fa94bb2e2aceb27f78c90f21e9cc6bff15711922275e7b5195` | `padmint.json` at `a41f80c`, byte for byte 0.11.0's recipe | Pass; PadMint `audit` pass |
| `SHA256SUMS` | | The three lines above, made from the files downloaded from the draft and checked with `shasum -c` | Pass |

No Windows zip or Linux AppImage was made for 0.11.1. Both fixes change only the iPhone and iPad app, and those builds
are made on Chris's PC from his own disc. Chris decides whether 0.11.1 ships them; the README's Windows and Linux links
name the 0.11.0 files and keep working either way.

## What it contains

| Change | Ships to | Checked | Not checked by hand |
| --- | --- | --- | --- |
| **Show Touch Controls with a controller** (#601): the first ⋯ row while the controls are hidden; Touch Controls → Auto-hide Touch Controls with Controller; held touch released on hide, hidden controls ignore presses | iPhone, iPad | `BWGameOverlay.mm` compiles for arm64 iOS 17 with no warnings; `audit_repo.sh`, attribution | The menu on a device, a physical controller |
| **External displays and orientation** (#602): RecompCore `82ceadf3` adds SDL3's UIKit scene-geometry patch to the vendored Aurora SDL | iPhone, iPad | The release IPA contains `sceneDidDisconnect:`, `windowScene:didUpdateEffectiveGeometry:` and the `UIKit_*OwningScene*` functions; the shared SDL harness on the iPadOS 27 Simulator | A physical TV or external display |

## Checks before publishing

| # | Check | Status | Evidence |
| --- | --- | --- | --- |
| 1 | Scope | Passed | Above |
| 2 | Windows tasks | Not applicable | Nothing in 0.11.1 changes Windows; no Windows build was made |
| 3 | What changed for PadMint | Passed | `git diff v0.11.0 -- padmint.json` is empty; `check_padmint_catalog.py`: recipe and PadMint both offer macos-arm64, windows-arm64, windows-x86_64 |
| 4 | Artifacts | Passed | Above |
| 5 | PadMint player check, with the checkout | **Open** | Started twice on November 14 (PadMint `93ace90`, Apple Python 3.9.6, empty folder, `a41f80c` and the release IPA) and stopped during local training both times to keep the build Mac's free disk above its floor. Not a failure; it has to pass before tagging |
| 6 | Player-facing text | Passed | No README or guide promises 0.11.1; the Windows and Linux links name 0.11.0's files |
| 7 | `SHA256SUMS` | Passed | From the draft's downloads; `shasum -c` passes |
| 8–12 | After publishing | Not started | |
