# BlueWake 0.10.0 release record

The record for 0.10.0: what it contains, the exact artifacts, and every check with its status. Previous release:
[0.9.0](RELEASE_0.9.0.md), October 25. Only Chris publishes.

**Status, October 31, 2026:** the candidate (#514) and its runtime (RecompCore#50) are merged and 0.10.0 is frozen by
#516. Checked on the Mac and the iPad. Waiting on the Windows and Linux builds on Chris's PC
([WINDOWS_BUILD_0.10.0.md](WINDOWS_BUILD_0.10.0.md)), PadMint's player check, and Chris's approval to publish.

## The release

| | |
| --- | --- |
| Version | 0.10.0, build 9 (`version.json`) |
| Release commit | `edd88f15750211cca7a4596c9c5aa348f124efe3`, the merge of the freeze pull request #516 (October 31), after #514 (`a7136ab`) |
| RecompCore | `78c266d20059560fa5efe68be16062c36623af6a`: `bluewake-next` with RecompCore#45 and #48 (irregularthanis) and #47 plus its six follow-ups (pdale-boop), merged through RecompCore#50 |
| Previous release | 0.9.0, October 25 (`211abfd`) |

## Artifacts

Mac-side artifacts were built on October 31 from a clean checkout of `edd88f1` and uploaded to the v0.10.0 draft; GitHub's
digests match the local files. `scripts/release/check_public_assets.sh` ran on each with the maintainer's release gate,
and PadMint `audit` (`main` `44a3704`, Apple Python 3.9.6) passed each.

| Artifact | SHA-256 | Where it comes from | Content gate |
| --- | --- | --- | --- |
| `BlueWake-v0.10.0-ios-unsigned.ipa` | `73d433b8ea17565506a8c7c51a43c506453f455be8fe150e9871869c70c265bf` | `build.sh --app-only` from a clean checkout of `edd88f1`; reports 0.10.0 (9); its provenance names `edd88f1`, unmodified, `containsTranslatedGameCode: false`; no game module | Pass; PadMint `audit` pass (0 address-named functions) |
| `BlueWake-v0.10.0-source.zip` | `8d84193c3a3e8ebf896042e7e446f45cbe57b26ad8c961eff3d4d10b5effde39` | `git archive --format=zip --prefix=bluewake-0.10.0/` of `edd88f1` | Pass; PadMint `audit` pass (10 address-named functions, 0.9.0's had 8) |
| `BlueWake-v0.10.0-padmint.json` | `a8508bd7db4c4cdbc907be918cdcfcdc61678b174ecbd1a34f359da2b6098715` | `padmint.json` at `edd88f1`, byte-identical to 0.9.0's (PadMint fills in the version) | Pass; PadMint `audit` pass |
| `BlueWake-v0.10.0-windows-x64.zip` | Pending | Chris's Windows PC, from `edd88f1` and his own disc | Pending: the only accepted finding is `containsTranslatedGameCode: true` |
| `BlueWake-v0.10.0-linux-x86_64.AppImage` | Pending | WSL with Docker on Chris's PC, the CI container, from `edd88f1` and the same disc | Pending: unpacked with `7zz` on the Mac and the folder gated; the only accepted finding is translated game code |
| `SHA256SUMS` | Pending | Made from the downloaded files | Pending |

No Mac or iPhone/iPad build with game code is attached; personal builds stay local.

## What it contains

| Change | Issue | Ships to | Checked | Not checked by hand |
| --- | --- | --- | --- | --- |
| **Faster translated code:** `_savegpr`/`_restgpr` inlined across the DOL and RELs with the frame proven once (#480, irregularthanis and pdale-boop), the gather pipe's alias check cached (#495, RecompCore#45), the guest alias lookup in constant time (#501, RecompCore#48) | #215, #427 | Every platform (#495's cache: Windows and Linux) | Mac (M3 Max), own disc, both builds trained the same way: benchmark tour, four interleaved runs each from one state, medians +7.3% Outset, +7.9% Windfall, +9.9% Dragon Roost Cavern, +10.6% Forest Haven, +9.6% the sea by the Fortress; the same 31 checkpoints in all eight runs. Contributors: Steam Deck rendered +14.2% (lava room +33%), Windows D3D12 +6.6%, Linux +6.3%, M2 Mac +6.7%, checkpoints identical | Windows and the Deck by hand |
| **The HD look tuned for #491** (pdale-boop, #494 with #485, RecompCore#47 and 0187-0192): soft shading kept on its surface and faded with distance, effects before the motion blur's copy, bloom screened, the sky sharp, softer torches, sun shadow bias and far plane; a switch for each part in a pane; F2 takes the sun shadows | #491 | Every platform but Android | Windows and Linux by pdale-boop (frames, played). Mac (Metal): Outset's pier from one state with the look on in 0.9.0 and 0.10.0; with ambient occlusion on and off the far sea is identical in 0.10.0 (blue 207.5 both) and darker with it in 0.9.0 (205.2 against 207.6); the debug view keeps Link's own shading on him; sun shadows draw | The cut-bush flicker on the Mac; the iPad |
| **HD's sky and light, HD's clouds in the sky, sun shadows timed and shaped as in HD** (pdale-boop, #494); off until turned on, need a new HD import | #57 | Every platform but Android | Windows and Linux by pdale-boop. Mac: the sky code loads and writes game memory only with the HD look and the switch on (`hd_look.c`) | On the Mac and iPad |
| **The stars at night**, for everyone: three vertices left in a quad draw a triangle (RecompCore 0191) | none | Every platform | Mac: Windfall's pier at night, 0.9.0 no stars, 0.10.0 stars; with the HD look off three frames from one state are pixel-identical to 0.9.0's | Smooth Motion with stars on the Mac |
| **Linux: closing from Steam no longer resets the settings:** the launch marker is cleared after 600 frames, as on Windows (#512) | #511 | Linux | Builds in Linux CI and the Mac build | On Linux or a Deck by hand |
| **Mac: the audio queue no longer paces the game** (#513) | #509 | Mac | M3 Max, 60 s paced with sound from one state: throttle on and off both steady at 29.96 game FPS, no slow seconds, nothing dropped; the throttle never engaged on this Mac (`throttles=0`) | A slower Mac |
| **Menus and controls** (pdale-boop, #485): empty switches show their default; a controller scrolls past a tab's end; greyed rows say why; **Nintendo button layout** replaces Swap A and B and Swap X and Y and turns itself on for a new player's Switch controller | #320 | Windows, Mac, Linux | Linux with a controller (pdale-boop); Windows with a Switch Pro controller on a fresh settings file (pdale-boop); settings tests; the Mac menu opened and drawn | Joy-Cons, the GameCube adapter |
| **Smooth Motion under sustained load:** Automatic or Never pause (#470, irregularthanis) | #473 | Windows, Mac, Linux | Settings tests; Steam Deck tours | By hand on Windows |
| **Export Dolphin Save** (#510, jkoehler11) | #508 | iPhone, iPad, Android | Pixel 8a; Dolphin round trip on Windows (pdale-boop); Chris's card on the Mac: all three quest logs out, the card unchanged, the `.gci` imports back with the same save data; the iOS app builds | On an iPhone or iPad |
| Fresh PadMint builds with Apple's Python 3.9 (#499) | #496 | Mac, iPhone, iPad | PadMint player check, October 29; CI runs the tests with `/usr/bin/python3` | |
| Linux first run checks the free space before the disc import (#486, jkoehler11) | #479 | Linux (the importer is shared with iOS and Android) | Sized tmpfs refusals; identical output to 0.9.0 on Linux and the Mac | |
| Windows builder adds the Windows SDK when `reg.exe` is blocked (#502, imcamilouribe) | none | Windows builds | That PC; tests | A PC without the policy |
| Android builds for Android 10 (#478, jkoehler11) | #474 | Android (experimental, own disc) | ELF and lint checks | An Android 10 phone |

Tools and records, nothing for players: the benchmark tour's kept state and its gameplay check (#450, #459), rendered
testing with an agent and `render_bench` (#492), the native comparison without `cycle_observation_suffix` (#481).

## Not in 0.10.0

| What | Why it waits |
| --- | --- |
| The `_savegpr_29` watch-list change (#497) | #480 inlines the same calls with its return-keyed rule |
| Android's first-run screen, disc checks and log (#488, #460, #477, #487), the Android HD import (#347) | No phone run yet |
| The AppImage's static libstdc++ (#472), Clang 18 for the Deck (#437), the Mac's slow-chunk flags (#392) | Draft, plan, or parked |
| Lava Room Smooth Motion and the next Outset work (#504, #503, RecompCore#49) | Plans and diagnostics |
| Object votes in the Smooth Motion camera (RecompCore#46) | Off by default with no switch; with the next Smooth Motion work |
| The cold first Picto Box photo (#342), iOS modules on Windows (#100) | Drafts |

## Evidence notes

- **Builds:** the Mac candidate and a Mac 0.9.0 (`211abfd`) were built on October 31 from Chris's disc on an M3 Max with
  the default preparations and local training, `--jobs 8` each, side by side. Both report the verified composite digest
  `54f54434`. The candidate fetched RecompCore `78c266d`; its prepare made 226,587 direct calls against 236,481, the
  difference being the calls #480 inlines.
- **Card:** Chris's card `4b54ec25…`, the one 0.9.0 was checked with, copied; it hashed the same afterwards.
- **Tour:** headless, unpaced, 1,800 retraces a stop; one setup state made by 0.9.0 and loaded by every run; order
  0.10.0, 0.9.0, 0.9.0, 0.10.0, 0.10.0, 0.9.0, 0.9.0, 0.10.0. Other work on the Mac slowed two candidate stops (busy
  53-82%); medians are used, and every candidate stop run at 95% busy or more beat every 0.9.0 run at that stop.
- **Stage sweep:** all 468 places on both builds from that card: 446 arrive in both, the other 22 end the same way in
  both (four test rooms, Wind Temple R04 and R08, late cutscenes), and the card is unchanged after all 468 in both.
- **iPad Pro (M2):** the candidate built from Chris's disc with local training, signed for development and installed over
  the app (data backed up first; the card `4b54ec25…` unchanged after the install and after the runs). The October 20
  iPad tour harness, headless, twice: Outset 98.8 / 99.8, Windfall 100.0 / 100.8, Dragon Roost Cavern 146.7 / 150.2,
  Forest Haven 83.4 / 85.2, the sea by the Fortress 75.0 / 76.1 game frames a second, against the 0.9.0 test build's
  91.1-92.1, 92.6-93.2, 133.4-135.8, 75.3-76.1 and 68.2-68.5 on October 24 (+8 to 11%). Every stop loaded with no dips
  and no fatal lines. Not interleaved, and the iPad route starts from a card continue, which even two runs of one build
  don't repeat (checkpoints differ from retrace 300), so these support the Mac result rather than repeat its proof.
- **CI on #514:** Linux host and AppImage, Windows host, macOS audit, sanitizers and attribution all passed.

## Checks before publishing

| Check | Status | Evidence |
| --- | --- | --- |
| Frozen commit and version | Passed | `edd88f1`, 0.10.0 build 9; the IPA names it |
| Mac: build, tour, sweep, pictures | Passed | Above |
| iPad | Passed (tour); hands-on play not done | Above |
| Windows build and checks | Pending | [WINDOWS_BUILD_0.10.0.md](WINDOWS_BUILD_0.10.0.md) |
| Linux AppImage build and audit | Pending | Same file, step 6; on the Mac it is unpacked and the folder gated |
| Content gate on every artifact | Mac-side passed; Windows and Linux pending | Artifacts above |
| PadMint `audit` | Passed | IPA, source zip, recipe |
| PadMint `player-check.sh` before tagging | Running | PadMint `main` `44a3704`, Apple Python 3.9.6, an empty PadMint folder, a clean clone of `edd88f1`, the release IPA |
| Release notes reconciled | Pending | On the draft, with the Windows and Linux numbers still to fill in |
| Draft state | Draft, untagged, three of six assets | |

## After publishing

| Check | Status |
| --- | --- |
| PadMint `player-check.sh` without a checkout, from `releases/latest` | Pending |
| Every asset downloads without signing in and matches `SHA256SUMS`; `releases/latest` leads to v0.10.0 | Pending |
| README and WINDOWS.md name 0.10.0's downloads, and the Linux AppImage | Pending |
| Replies on the fixed issues; #459 and #497 closed with a note | Pending (Chris approved posting) |
