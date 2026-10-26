# BlueWake 0.9.0 release record

The record for 0.9.0: what it contains, the exact artifacts, and every check with its status. Previous release:
[0.8.0](RELEASE_0.8.0.md), October 18. Only Chris publishes.

**Status, October 25, 2026:** frozen at `211abfd`. The v0.9.0 draft holds the five final artifacts below, each
verified against GitHub's digest and passed through the content gate. Waiting on Chris's approval to publish. Until
it is published, only docs and fixes for what the checks find go into `main`.

## The release

| | |
| --- | --- |
| Version | 0.9.0, build 8 (`version.json`) |
| Release commit | `211abfdecf13beb5b9f2120459a1a3cde50dd84e`, the merge of the freeze pull request #464 (October 24) |
| RecompCore | `1446450c79046bd34e43abadc27522eeab768a6e` on `bluewake-next`: RecompCore#43 and #44 (Dawn with BMI2), #40 (Smooth Motion), #42 (sun shadows) |
| Draft | v0.9.0, a draft targeting `211abfd`. The tag is created only when Chris publishes |
| Previous release | 0.8.0, October 18 (`f2611b1`) |

## Artifacts

All five were downloaded from the draft into a private staging folder on the Mac on October 25 and match GitHub's
digests. `scripts/release/check_public_assets.sh` ran on each with the maintainer's release gate
(`~/.codex/release-gate/release_gate.py`, which reports every finding in every member rather than stopping at
the first).

| Artifact | SHA-256 | Where it comes from | Content gate |
| --- | --- | --- | --- |
| `BlueWake-v0.9.0-ios-unsigned.ipa` | `675afb8162684d73e1daa81d079d90d4316f3f53d66f436fe488fffdb68b0375` | `build.sh --app-only` from a clean checkout of `211abfd`; reports 0.9.0 (8); its provenance names `211abfd`, unmodified, `containsTranslatedGameCode: false` | Pass; PadMint `audit` pass. No game module, disc data, saves, keys, provisioning profile or signing files |
| `BlueWake-v0.9.0-source.zip` | `c72a3a0bc7ea235b1c9eb7de2f49ffd58b0b36a748aa7770a5daaf9929bb9499` | `git archive --prefix=bluewake-0.9.0/` of `211abfd`; regenerated on October 25 to the same SHA-256 | Pass; PadMint `audit` pass |
| `BlueWake-v0.9.0-padmint.json` | `a8508bd7db4c4cdbc907be918cdcfcdc61678b174ecbd1a34f359da2b6098715` | `padmint.json` at `211abfd` (the same bytes as 0.8.0's: the version is filled in by PadMint) | Pass; PadMint `audit` pass |
| `BlueWake-v0.9.0-windows-x64.zip` | `3c0edc36241fc1783693e1a9fa942995f4f32efec14fb3a9afc889a595da61ce` | Chris's Windows PC, from `211abfd` and his own disc ([results](WINDOWS_BUILD_0.9.0.md#results)); module `795d1937…985a`, composite digest `54f54434` | The only finding is the accepted `containsTranslatedGameCode: true` (its `BuilderProvenance.json`). 35 members: no disc image, disc files, saves, cards, keys, `nodtool.exe` or signing files. `dsp/dsp_rom.bin` and `dsp_coef.bin` are the open replacement DSP ROMs from RecompCore's `Data/Sys/GC`, byte-identical to 0.8.0's |
| `SHA256SUMS` | `73298b14f66aeb95bf63c523aa68132e56f5df478db1f3dda0a6bc4e65889b29` | The four lines above, made from the downloaded files and checked with `shasum -c` | Pass |

No Mac or iPhone/iPad build with game code is attached; personal builds stay local.

## What it contains

Everything below is in `211abfd`.

| Change | Issue | Ships to | Checked | Not checked by hand |
| --- | --- | --- | --- | --- |
| **Mac, iPhone and iPad as fast as the PC builds:** the Windows and Linux module preparations and behaviour timing by default; `--conservative` keeps the plain translation (#431) | #104 | Mac, iPhone, iPad | iPad Pro (M2) tour: +75 to 79% over 0.8.0 at every stop; game thread 34 to 52% busy at 30 FPS instead of 51 to 88% (below). M3 Max on October 20: +64 to 77% ([record](APPLE_RECIPE_TOUR_2026-10-20.md)). Chris played the iPad build | The Mac by hand |
| **The camera no longer dips for a frame every few seconds** with Direct right-stick camera and aiming on (jslay88, #455) | #401, #430 | Every platform | Linux play and trace (jslay88); dispatch and handoff tests. Scripted stand-still from the Outset state: Mac 0 of 5,410 frames off the held view in three minutes, 20 of 5,103 with only #455 reverted; iPad Pro 0 of 1,843 | Climbing, quick doors and the draw tags; Windows (skipped) |
| **Smooth Motion without the flicker** in the HD look, clouds, wave crests and swimming (pdale-boop, #371, RecompCore#40); off by default | #136 | Every platform | Windows (Direct3D 12) and Linux play by pdale-boop; Linux by jslay88; Chris used Smooth Motion on the iPad build | The Mac by eye; Windows (skipped) |
| **Sun shadows from a shadow map** (pdale-boop, #398, RecompCore#42); off by default, under Mods › Experimental | #57 | Every platform but Android | Windows and Linux play by pdale-boop. Mac frames at all five tour stops with and without them, no artifacts; repeat runs identical. Turning them on changes guest memory by design (palm cull boxes, room culling), the same way every run | By eye on the Mac and iPad, with the quality switches |
| **Linux builds at the player's CPU target again:** Dawn with BMI2 (jslay88's RecompCore#43, #44; #446) | #433 | Linux, and every build through the pin | Linux/AppImage CI at `-march=x86-64-v3`; contributors' own-disc AppImages (below); on the Mac the existing Dawn and pipeline caches loaded and were rewritten over seven launches with no errors | The Windows cache restart (skipped) |
| **One desktop menu** for Mac, Linux and Windows from one table of settings, with controller navigation, contrast and text size, a Config tab and greyed settings that say why (pdale-boop, #384 to #388) | #320 | Windows, Mac, Linux | CI on all three; the F1 menu opened and quit on the Windows build | A controller on the Mac and Windows |
| **Controls:** a controller can play while another window is in front, off by default (#465); "The right stick aims too (first person, items)", on by default (#466) | #442, #402 | Windows, Mac, Linux | Settings table test; Mac scripted: first person turns with the right stick only with the switch on | A controller by hand on any platform |
| **iPhone and iPad: the menu's switches keep the menu open** (#316) | #104 | iPhone, iPad | A UI test on the iPadOS 27 simulator; Chris used the menu on the iPad build | Two switches tapped in a row, checked on purpose |
| **HD textures from a real Wind Waker HD disc image:** the hashed WUD/WUX content (#443); a key file, a pasted key or a `keys.txt`, the form kept after a failure, the result in colour (#444) (jkoehler11) | #336 | Every platform but Android | Importer tests; a USA WUX on Linux with a `keys.txt`: 3,546 imported | A Windows import from the menu; an iPhone or iPad import |
| A sea compass switch, off by default (jslay88, #456) | none | Every platform | Linux screenshots at all four headings, on and off | The clock hand; Windows |
| **Share Session Log** sends this session's log and the one before (#440) | none | iPhone, iPad, Android | Built | A crash, a relaunch and a share by hand |
| The Android builder runs on macOS (benwolfram, #391); Android training refuses a run that never reaches control (#445) | #75 | Android (experimental, own-disc) | Tests; a Mac run to a signed APK | Fresh measurements on a phone |

Tools and records, nothing for players: the benchmark tour's kept state (#450, still open), the Windows CI timeouts
(#462), the Deck records (#452 to #454), the game-thread plan (#421), the worker tests (#424), the native and translated
code guide (#435).

## Evidence notes

- **The tested builds are the release code.** The Mac and iPad test builds were made from `dee2a223`, which differs from
  `211abfd` only in `version.json`; their game module is the one measured on October 24 (iPad module `00028fb0…`).
- **iPad Pro (M2) benchmark tour,** headless game frames a second, two runs, against the October 20 runs of 0.8.0 with
  the same harness, card and settings: Outset 91.1 / 91.9 (0.8.0: 52.2 / 52.1), Windfall 92.6 / 92.6 (52.0 / 53.0),
  Dragon Roost Cavern 134.3 / 135.8 (75.1 / 77.0), Forest Haven 75.6 / 76.1 (42.3 / 42.6), the sea by the Fortress
  68.5 / 68.2 (39.1 / 39.0). Every stop loaded with no dips; the rebuilt app repeated it (92.1, 93.2, 133.4, 75.3, 68.4).
  Chris's card hashed `4b54ec25…` before every install and after every run.
- **The 468-place stage sweep** on the Mac: 446 arrive and the card is unchanged after all 468, the same outcome at
  every place as 0.8.0. The other 22 are test-room hangs, Wind Temple R04 and R08 (#318) and late cutscenes an early
  file can't reach.
- **Windows:** built from `211abfd` in 85 min 8 s, 6 jobs, clang 22.1.3, both optimization profiles. Check a is partial:
  a 479.3-minute portable title and opening session with Direct3D 12, draw fusion and no fatal lines, quit cleanly; Forest
  Haven play and visual correctness were not verified. Checks b to e (the camera standing still, Smooth Motion with the HD
  look, Dragon Roost with saving and reloading, the controller options) were skipped at Chris's direction. Chris accepted
  publishing without them; they are not passes. [Results](WINDOWS_BUILD_0.9.0.md#results).
- **Linux:** jkoehler11 built the AppImage from `211abfd` with his own disc in the CI container (`-march=x86-64-v3`,
  RecompCore `1446450`): `check_appimage_abi.py` reported 14 ELF files, GLIBC 2.38 at most and no private game files,
  and it boots (#433). pdale-boop built one from `c5ebeca` too. Neither file is attached: Linux ships as source for now,
  and the notes say an AppImage follows once it's checked and uploaded.
- **PadMint player check before tagging** (PadMint `main` `a723ae0`, 0.5.13, Apple Python 3.9.6, an empty PadMint folder,
  a fresh clone of `211abfd`, the release IPA as the app): completed in about 2 h 56 min. Its record names `211abfd` and
  0.9.0 build 8. Its own publication gate labels the personal app it made "personal build, not publishable" (translated
  game code), which is correct; that app stays local.

## Not in 0.9.0

| What | Why it waits |
| --- | --- |
| Speed on Windows, Linux, the Steam Deck and Android | No new speedup in this release; the next gains need native generation fixed (#451) and the draw-plan work (#454) |
| Android's first run and disc check (#460, #477), Android 10 (#478), the Android HD import (#347) | Phone checks; after the release |
| The cold first Picto Box photo (#342, RecompCore#39) | Conflicts with `main`; platform checks |
| The Mac's slow-chunk compile flags (#392) | With #431 only 5 of 859 chunks take over three minutes; likely parked |
| Smooth Motion pacing (#470, #473), the AppImage's libstdc++ linking (#472), Clang 18 for the Deck (#437), newer contributor changes (#480, #481) | Opened or still measuring during the freeze |
| Building the iPhone and iPad app on Windows (#100) | Draft |

## Release notes

The notes are on the v0.9.0 draft, in Chris's voice. They give the Windows zip's SHA-256 with
[If Windows says it's a virus](../WINDOWS.md#if-windows-says-its-a-virus), say plainly that 0.9.0 doesn't make
Windows, Linux, the Steam Deck or Android faster, say which checks Windows did and didn't get, and give the Linux AppImage
status.

## Checks before publishing

| Check | Status | Evidence |
| --- | --- | --- |
| Frozen commit and version | Passed | `211abfd`, 0.9.0 build 8; Windows, IPA and PadMint records all name it |
| Artifact hashes | Passed | All five match GitHub's digests; the Windows zip matches the hand-off's `3c0edc36…` |
| Content gate on every artifact | Passed | Apple and source artifacts pass outright; the Windows zip's only finding is the accepted one |
| PadMint `audit` | Passed | IPA, source zip and recipe |
| PadMint `player-check.sh` before tagging | Passed | Above |
| iPad play | Passed (short) | Chris, October 24: stable at 30 FPS, no dips, Smooth Motion, touch controls and mods fine |
| iPad and Mac measurements | Passed | Tour, sweep, scripted camera and aim checks, HD look frames, Dawn cache |
| Mac by hand | Not done | No hands-on Mac session; Mac ships as source |
| Windows build | Passed | 85 min, from `211abfd` |
| Windows play checks | Partial: a partial, b to e skipped | Accepted by Chris without further validation; not passes |
| Linux AppImage | Not in this release | Built and booted from `211abfd` by a contributor; not attached |
| Release notes reconciled | Passed | Windows hash, unsigned guidance, checks and known problems match this record |
| Draft state | Passed | Draft, untagged, five assets |

## After publishing

| Check | Status |
| --- | --- |
| PadMint `player-check.sh` without a checkout, from `releases/latest` | Pending; can only run after publishing |
| Every asset downloads without signing in and matches `SHA256SUMS`; `releases/latest` leads to v0.9.0 | Pending |
| A Defender report, if a player has one | Handle once, as AGENTS.md says |
| The Linux AppImage, if a contributor hands one over | Gate it, then add it and update `SHA256SUMS` |

Then post on each issue that 0.9.0 has the change and ask the reporter to confirm. Close an issue only when the reporter
does, or under the 14-day rule (AGENTS.md).

| Issue | What to ask |
| --- | --- |
| #401, #430 The camera dip | Whether the camera holds still with Direct right-stick camera and aiming on |
| #136 Flickering clouds and waves | Whether the flicker is gone with Smooth Motion above 60 FPS |
| #433 Linux build at x86-64-v3 | Whether the container and AppImage build finish for them |
| #104 Built BlueWake with PadMint? | How long the build took and how it runs, on which device |
| #336 Texture pack status | Whether the import's result and counts are clear |
| #57 The Wii U HD look | Whether the sun shadows look right |
| #442 Background input | Whether the controller works with another window in front |
| #402 Camera and aiming apart | Whether the bow feels right with "The right stick aims too" off |

