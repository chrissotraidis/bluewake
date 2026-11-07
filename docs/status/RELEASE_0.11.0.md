# BlueWake 0.11.0 release record

The record for 0.11.0: what it contains, the exact artifacts, and every check with its status. Previous release:
[0.10.0](RELEASE_0.10.0.md), November 1. Only Chris publishes.

**Status, November 7, 2026:** frozen; the Windows zip and the Linux AppImage are being built on Chris's PC
([WINDOWS_BUILD_0.11.0.md](WINDOWS_BUILD_0.11.0.md)); the Mac-side checks are below.

## The release

| | |
| --- | --- |
| Version | 0.11.0, build 10 (`version.json`) |
| Release commit | `0d84fa1969a1ac9a9e9333d85dd425816ea3ce23`, the merge of the freeze pull request #570 (November 7) |
| RecompCore | `f9e3f4e59be4d49f10130bf6e68bc5d5fa0451e6`: `bluewake-next` with RecompCore#49 (irregularthanis) merged; the same tree as `a4dbaaa`, which every contributor measurement used |
| Previous release | 0.10.0, November 1 (`edd88f1`) |

## Artifacts

The Apple-side artifacts were made on November 7 from a clean checkout of `0d84fa1` and uploaded to the v0.11.0 draft;
GitHub's digests match the local files. `scripts/release/check_public_assets.sh` passed each with the maintainer's
release gate, and PadMint `audit` (`main` `d200545`, 0.5.32, Apple Python 3.9.6) passed each.

| Artifact | SHA-256 | Where it comes from | Content gate |
| --- | --- | --- | --- |
| `BlueWake-v0.11.0-ios-unsigned.ipa` | `9ddd60a75a50eaf2bf9fa0f0e70464d14b3c00c0b5fdf5e0c005df5fea909f79` | `build.sh --app-only` from a clean checkout of `0d84fa1`; its provenance names `0d84fa1`, unmodified, `containsTranslatedGameCode: false`; no game module | Pass; PadMint `audit` pass (0 address-named functions) |
| `BlueWake-v0.11.0-source.zip` | `99de98182a8224c398785ece979b74837e37814b49b51b22284b575f4d6d6e04` | `git archive --format=zip --prefix=bluewake-0.11.0/` of `0d84fa1` | Pass; PadMint `audit` pass (11 address-named functions, 0.10.0's had 10) |
| `BlueWake-v0.11.0-padmint.json` | `f9dcb0d04324e1fa94bb2e2aceb27f78c90f21e9cc6bff15711922275e7b5195` | `padmint.json` at `0d84fa1`: schema 2 since #100 (the Windows and Linux iPhone-module hosts) | Pass; PadMint `audit` pass |
| `BlueWake-v0.11.0-windows-x64.zip` | WINZIP | Chris's PC ([WINDOWS_BUILD_0.11.0.md](WINDOWS_BUILD_0.11.0.md)) | Pending |
| `BlueWake-v0.11.0-linux-x86_64.AppImage` | APPIMAGE | WSL with Docker on Chris's PC | Pending |
| `SHA256SUMS` | | The five lines above | Pending |

## What it contains

| Change | Issue | Ships to | Checked | Not checked by hand |
| --- | --- | --- | --- | --- |
| **Faster translated code** (irregularthanis): cheaper exact FP code, on by default (#553); GX writes handed over in runs, on by default (#531); the direct-call check settled at build time and the actor search's lookup inlined (#503); Clang 22 for Linux builds (#550); every builder wired for it (#555, #556, #558) | #215, #427 | Every platform | Mac (M3 Max), own disc, one state, interleaved: +9.4% to +11.1% over 0.10.0 by geometric mean (per stop +4.3% to +18.5%); guest memory identical to 0.10.0 at all 31 checkpoints. Contributors: Steam Deck +3.05% (#553 M1), +1.95% (M2), +1.7% (#531), +1.4% (#503); Pixel 8a 3.6% fewer instructions (#553) | Windows and the Deck by hand |
| **Faster calls inside the game's code, on by default** on Windows, Linux and the Mac (#552, #567; irregularthanis) | #215, #427 | Windows, Linux, Mac (iPhone, iPad and Android: opt-in) | Mac: +12.9% gate open against closed in one build, every open run ahead at every stop, identical checkpoints. Contributors: Windows +12.3%, Linux desktops +12.1% to +16.1% (pdale-boop), Steam Deck +5.5%, Pixel 8a 13% fewer cycles (jkoehler11) | The Windows release build (WINDOWS_BUILD_0.11.0.md step 3) |
| **Game code on large memory pages**, Linux, opt-in (#554) | #215 | Linux | Deck +4.47%; i5-6500 +3.7%, about 300 MB more memory (pdale-boop) | |
| **The stage select costs less**: −10% → −3.6% with it on (#524) | | Every platform | Deck; with it on, the gate stays closed | |
| **Efficient soft effects**, a Display switch, off by default (#504, RecompCore#49) | | Every platform but Android | Deck: 12.6% less GPU time at Outset | The Mac by eye |
| **Save states keep working features** (#543, pdale-boop): the stage select, the saving lock, HD sky and mod settings | | Every platform | Linux (pdale-boop) | F7 after loading a state on the Mac, by hand |
| **The first run shows its stages** on every platform; the Linux setup window, with an install folder; Linux builds no longer carry the game files (#525, jkoehler11) | #523 | Linux, Windows, iPhone, iPad | Linux with a real disc (jkoehler11) | Windows (WINDOWS_BUILD_0.11.0.md check b), the iPad |
| **F9 performance panel** (#537, jkoehler11) | #535 | Windows, Mac, Linux, Android | Linux, Pixel 8a; the Mac app builds with it (#568) | Windows (check c), the Mac by eye |
| **Android first run** on the phone: pick, check and verify the disc, keep a crash's log (#487 with #488, #460, #477; jkoehler11); present without blocking (#557) | #476 | Android (experimental, own disc) | Pixel 8a | Other phones |
| The skinning native hands back to the translated epilogue, exact on every build (#541) | | Every platform | Deck; differential test | |

Tools and records, nothing for players: the inliner and the direct-call rewrite read one watch list (#547), the
comparison harness answers like the host (#548), the native generators run on a current tree (#564), `_savegpr_29`
inlined again (#565), the benchmark tour's `--env` (#569), and the Clang, training and dispatch-loop records (#437,
#529, #530, #518).

## Not in 0.11.0

| What | Why it waits |
| --- | --- |
| Wind Waker HD's light and shadows (#533, RecompCore#51) | Drafts |
| GXDrawDone overlap (#551, RecompCore#52) | A desktop run first |
| First-run Cancel (#540) | Draft, rebasing on #525 |
| Android's performance bar (#559) | Its notes folder comes out first |
| The stage select's slot filter (#562), render-scale plan (#563), module art addresses (#549) | Plans and research |
| The Android HD import (#347), the cold first Picto Box photo (#342) | Drafts waiting on hardware |
| Mac slow-chunk compile flags (#392) | Closed: about 1.5% slower with today's Apple defaults |
| Faster calls on iPhone and iPad | Not measured there yet |

## Evidence notes

- **The Apple test (#556):** `79566dd` (#554's stack with #524, #541, `main` and #567's first commit), built on an M3 Max
  with `--direct-gate`, local training 48 min, module compile 22 min; the FP-checks pass removed 296,188 tests in 811
  chunks. Benchmark tour, headless, unpaced, 1,800 retraces a stop, every run from the 0.10.0 record's setup state and
  Chris's card `4b54ec25…`.
  - 0.10.0 (the record's candidate build) against 0.11.0 with the gate closed, nine runs interleaved A B C C B A A C B:
    Outset 107.0 → 117.5 (+9.9%), Windfall 113.3 → 118.2 (+4.3%), Dragon Roost Cavern 162.7 → 181.1 (+11.3%), Forest
    Haven 86.1 → 102.0 (+18.5%), the sea by the Fortress 78.6 → 87.9 (+11.8%), medians; cycles, MEM1, MEM2 and aliases
    identical to 0.10.0's at all 31 checkpoints; the CPU hash differs through `cycle_observation_suffix` (#553, #481).
    That run's "gate on" side was also closed: the tour clears inherited `BLUEWAKE_` variables, which #569 fixes.
  - The gate open against closed in that build, six runs interleaved with `--env`, each log naming its state: Outset
    +10.8%, Windfall +12.6%, Dragon Roost Cavern +14.9%, Forest Haven +11.0%, the sea by the Fortress +15.5%; geometric
    mean **+12.9%**; every open run ahead of every closed run at every stop; one set of checkpoints across all six.
  - pdale-boop, M2 Mac mini, rendered (#567): +7.5% gate closed and about +12% gate open against `main`, pictures and
    checkpoints identical, the module 425 MB against 451 MB.
- **The freeze build:** `0d84fa1` rebuilt into the same folder with no options (it retrained: the Mac now trains with
  the gate open by default). A tour with nothing set logs `gate=on` and matches the test build at all 31 checkpoints,
  every field.
- **Stage sweep:** all 468 places on the freeze build from Chris's card: 0 end differently from 0.10.0's sweep, 461
  reach Link and 446 run to the end as 0.10.0's did (the same test rooms, Wind Temple R04 and R08, and the Ending), and
  the card is unchanged after all 468.
- **iPad Pro (M2):** #567's head with `main` (version still 0.10.0 build 9 in the app), built from Chris's disc with
  local training, signed for development and installed over the app; the app's Documents were backed up first
  (2.9 GB) and the card hashed `4b54ec25…` before and after. The October 20 iPad tour harness, headless, gate closed /
  open, two runs each: Outset 111.1, 112.1 / 121.3, 121.5; Windfall 111.1, 112.0 / 121.2, 121.7; Dragon Roost Cavern
  162.7, 165.0 / 184.2, 184.8; Forest Haven 94.0, 93.4 / 103.8, 103.9; the sea by the Fortress 82.4, 82.6 / 89.6, 89.9,
  against 0.10.0's 98.8-99.8, 100.0-100.8, 146.7-150.2, 83.4-85.2 and 75.0-76.1 on October 31 (+9 to 12% closed, and
  the gate +9 to 13% more). Every stop loaded with no dips. The iPad route starts from a card continue, which two runs
  of one build don't repeat, so these support the Mac's exactness result rather than prove it; iPhone and iPad keep the
  gate closed in 0.11.0.
- **PadMint player check before tagging:** PadMint `main` `d200545` (0.5.32), Apple Python 3.9.6, an empty PadMint
  folder, a clean clone of `0d84fa1` and the release IPA: passed in 10,422 s (2 h 54 min). Its record names `0d84fa1`,
  status completed, checkout check before-record-passed, publication personal-only (correct). The personal build was
  deleted.
- **CI:** every merged pull request passed Linux host and AppImage, Windows host, the macOS audit, sanitizers and
  attribution; the freeze (#570) too.

## Checks before publishing

| Check | Status | Evidence |
| --- | --- | --- |
| Frozen commit and version | Passed | `0d84fa1`, 0.11.0 build 10; the IPA names it |
| Mac: the Apple test, tour, sweep | Passed | Above |
| iPad | Passed (tour); hands-on play not done | Above |
| Save states (#543) on the Mac | Loading a state at launch works (every tour run); F7 after it not checked by hand | |
| Windows build, speed and play checks | Pending | WINDOWS_BUILD_0.11.0.md |
| Linux AppImage build and audit | Pending | |
| Content gate on every artifact | Apple, source and recipe passed; Windows and Linux pending | |
| PadMint `audit` | Passed | IPA, source zip, recipe |
| PadMint `player-check.sh` before tagging | Passed | 2 h 54 min, from `0d84fa1` with the release IPA |
| Draft state | Draft, untagged, three assets | |
