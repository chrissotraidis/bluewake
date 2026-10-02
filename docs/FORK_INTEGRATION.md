# Source-fork integration

The current fixed donor baseline and outstanding acceptance checks are in the
[October 2 reconciliation ledger](status/FORK_RECONCILIATION_2026-10-02.md).
The current cumulative review is [PR #37](https://github.com/chrissotraidis/bluewake/pull/37),
with executable evidence at `3392854` and maintained runtime `18ba3b64`.
It includes the later Windows preparation/native work, post-texture renderer
correction and haptics; see the ledger for acceptance and intentional differences.
The sections below describe the earlier October 1 import and retain their
original source/platform limits. Their then-open implementation items are not
the current migration checklist.

BlueWake integrates GPL-covered source changes from
[elliotttate/Wind-Waker-Recomp](https://github.com/elliotttate/Wind-Waker-Recomp), whose source is
public as checked on October 1, 2026. Chris and Elliott have agreed to consolidate
their work in BlueWake, with Elliott transitioning his development here. The
reconciliation directly imports his enhancements and adapts them to preserve
BlueWake's existing platform support. The imported commits retain Elliott's authorship. BlueWake's
release restrictions, game-module privacy rules, PadMint workflow, bundle identifier and saves remain
the project's own.

## Shared runtime and iOS

The integration imports the main branch's feature commits through `b39bd0dc9d18`:

- Renderer interpolation, matching of scenery and particles, steady 60/120 Hz presentation,
  batching, staging-buffer uploads and overload pacing.
- Optional jump, sprint, shorter fades, acceleration while a scene is fully black, and quick doors.
- Desktop mouse camera and direct right-stick camera, with aiming, zoom and collision checks.
- Fifteen Better Wind Waker settings selected inside newly translated modules, plus widescreen 16:10.
- Guest-alias synchronization/cache, actor-search budget optimization, a larger REL scratch window,
  and contextual frame-dip logging.

BlueWake adds touch Jump/Run buttons using its existing layout editor, forwards iOS keyboard jump
events, preserves the existing Smooth Motion preference and ProMotion choice, and leaves the new
gameplay options off until enabled. The iOS camera retains its original behavior by default.
The alias-cache epoch is atomic across the game and graphics threads. Existing dependency checkouts
fetch the parent without recursively fetching a new submodule from its old remote, then synchronize
the translator's remote before updating it.

RecompCore was pinned at `8ab24daee9c641634fda5cac30389ad4b2cfda5e` for this import (now
`chrissotraidis/RecompCore` `bluewake-next`, which adds the fork's 9618e9d and the Windows DSP fix) and DolRecomp at
`b8b534591cba8ca7cd43943a655ee6e2591cf5de`. These are the fork's changes over BlueWake's previous
`2d60636` and `5c91d6e`. Base translation must still satisfy the existing composite digest.
New Better Wind Waker options and 16:10 require rebuilding the personal module. App-only upgrades
keep the previous Better Wind Waker disc path when its DOL matches the old module's known format;
otherwise that mod is safely skipped with a rebuild message, without changing the saved preference.

## Checks performed here

- iOS app-only build and native Apple Silicon Mac host build pass.
- All 24 registered BlueWake host tests pass, including a new fast-transition test covering black
  menus, fade ordering, return to rendering, timeout, and disabled settings.
- Save-state container tests cover roundtrip, malformed/truncated fields without partial writes,
  unsupported versions, missing END, duplicate chunks and trailing bytes. Address/undefined-behavior
  sanitizers pass. Decompressed input is capped at 256 MiB and 4,096 chunks, and host/alias streams
  are checked before replacing guest memory. Save states remain experimental debug tools, not a
  replacement for memory-card saves or an arbitrary-file security guarantee.
- Actor-search equivalence also passes with undefined-behavior sanitization.
- The composite generator's 18 synthetic fixtures pass. The separate CPU ABI script needs the
  absent `generated/full/composite-lib` developer fixture; it was not counted as a pass.
- A fresh translation from the personal GZLE01 disc matches the existing `54f54434...` digest.
  Building the mod source produces 22 variant chunks for each widescreen shape, 15 Better Wind
  Waker variant chunks, 15 options and 40 option writes. Full optimized module compilation/training
  and gameplay with each new option remain unverified.
- Repository audit and public-assets check on the app-only IPA pass. This does not authorize a release.

The broader CTest discovery includes donor tests that were not built and placeholder tests named
`*_NOT_BUILT`; those were reported as not run, separate from the registered BlueWake tests.
Imported Mac performance figures in the status ledger are the author's measurements.

## Physical iPad check (October 1)

The updated signed app was installed in place on the connected M2 iPad Pro, retaining the existing
personal module. Save and preferences were backed up through `afcclient` after CoreDevice copies
timed out; readback after installation and gameplay matched both original files byte for byte.
Wired QuickTime snapshots showed gameplay on Outset, including the new Jump and Run controls.
Scripted touch events passed through the real overlay: Jump invoked the guest jump procedure,
and Run changed the speed cap from 17 to 25.5 and restored 17 on release. The log also showed
fast-forward ending before the fade returned to the visible scene.

The short test reported about 60 displayed frames / 30 game frames and full speed, thermal state 0.
This is a specific scene check, not a performance comparison, 120 Hz acceptance, a long-session
thermal test, controller acceptance, physical door test, or independent speaker-audio evaluation.
A second launch with the original saved settings reached gameplay. New gameplay options remain off
by default; the launch-only test overrides did not change the stored preferences.

## Desktop work

The native Mac host now contains the fork's desktop controls and options menu. The fork's public
Mac packaging scripts bundle translated game code; that packaging model is not imported.
The Windows foundation port (`4b01c6b`) and settings overlay (`4fbcc7f`) are integrated. BlueWake adds
the new shared feature sources and opt-in gameplay settings, keeps Windows' own options menu, and
uses portable C thread-local declarations. A macOS-hosted LLVM-MinGW check passes for all 27 shared
C host sources, two disc-tool sources, the entry shim, settings overlay and Win32 compatibility layer.
The native Windows clang/MSVC-toolchain/Dawn compile and link passed in
[CI run 36796926528](https://github.com/chrissotraidis/bluewake/actions/runs/36796926528) at `756e3aa`.
Direct3D/gameplay gates remain open. The fork's later Windows
code-generator changes and experimental 60 Hz simulation are not included. No ready-made donor app
or game module was downloaded. Parallels remains off at the user's request.

Native Apple Silicon Mac gameplay was visually checked on Outset with Smooth Motion, around
60 displayed / 30 game frames in this short scene. Intel Macs, packaged fresh-machine installation,
speaker audio, long-session stability and 120 Hz remain separate acceptance gates.

The latest source also imports experimental desktop save states and opt-in wall climbing, plus
the newer derived-pipeline cache and graphics-worker timing work. In a headless Outset scripted walk,
saving around retrace 1,000, restarting from the state, and continuing to 1,500 produced identical
CPU, MEM1, ARAM, aliases, VI clock, host fields, loop fields and DSP chunks against an uninterrupted
run. A Metal-window save/load also restored its GX chunk and completed normally. These are local
personal-build tests; no states or game artifacts are published. Wall climbing is off by default
and not independently gameplay-validated here; iOS has no new climbing/states settings UI yet.

Windows gains save/load buttons in its Game tab and F6/F8 shortcuts, preserving F9 for FPS.
States default to `%APPDATA%\\BlueWake\\states`. The new GitHub Windows host workflow compiles
and links only public runtime/app source, without any disc, translated module, release or artifact
upload. That source-only result does not establish gameplay or fresh-machine packaging compatibility.
The latest iOS source builds, but has not replaced the physical-iPad build described above.

The Mac app now packages its icon from the same tracked BlueWake wave artwork as iOS. Both desktop
settings menus use a scoped navy/cyan BlueWake theme. Mac typography uses the system font without
changing the game/FPS overlay. The menu keeps tabs and actions visible while settings scroll.
Mouse tab selection, scrolling and Resume passed in the first native check. Later automation of the
pinned-tab layout delivered SDL clicks at 183,232 but ImGui's global-pointer fallback reported
1342,410, outside the window. Reliable pointer/controller interaction with the final layout remains
a hands-on gate; no input-engine workaround was added for that automation mismatch.
A timed native open/close test left an explicit synthetic preferences file byte-identical and
resumed the game normally. New Mac preferences use Application Support/BlueWake;
the legacy folder is only read as a fallback and is never renamed, deleted or overwritten.
The Mac bundle also ships the 404-entry starter cache; a native launch logged 404 rows merged and
zero skipped. This checks packaging/format compatibility, not a measured first-visit performance gain.

## Apple TV and starter cache

At the user's October 1 request, this branch integrates #6 (including Ian MacFarlane's original
credited #3 commit) and #8's 404-entry starter pipeline cache. Both remain source integrations on
this draft branch, not merges to main or public releases.

The current tvOS app compiles and links with the pinned runtime; its executable reports Mach-O
platform TVOS, minimum OS 17.0, and its bundle uses device family 3. The iOS app still builds as iOS
with families 1 and 2. Seven synthetic signing tests preserve iOS wildcard identity and unrelated
entitlements, specialize tvOS's identifier, reject mismatched profiles, and leave input profiles
unchanged. Import failures now preserve the transferred ISO and wait for a retry or changed file.
The first-run polling no longer repeatedly imports a known failing input. Existing mismatched Dawn
tvOS caches are refused instead of recursively removed.

The connected-device inventory contains no physical Apple TV. Ian reported title-screen boot on
Apple TV 4K; this integration has no independent physical TV gameplay, audio, controller or saves
acceptance. The retagged Dawn dependency, purgeable tvOS Caches data and lack of a couch-friendly
in-game settings/backup shell remain explicit developer-preview limits. PadMint remains iOS only.
See [Apple TV build](status/TVOS_BUILD.md). No personal build or game data is published.

## Contribution path

The October 1 GitHub check found no open BlueWake issues. Open BlueWake PRs include Smooth Motion
(#5, covered by this integration), the Apple TV work (#3/#6) and a larger pipeline seed (#8).
Apple TV and pipeline-seed work are now integrated into this working branch as requested; their
original PRs remain open until the maintainer decides how to land the combined work.
The donor's open issues concern intro music, Switch Pro A/B mapping, Linux support and a Wii U HD-style
UI. They are follow-up reports/requests, not verified fixes in this branch. Existing BlueWake controller
remapping is retained on iOS; desktop and Apple TV remapping still need their own implementation
and hardware checks. Intro-audio reports need targeted reproduction before changing DSP behavior.
The focused stability loop now checks fresh-save intro PCM in HLE/LLE and original/fast transitions,
profiles rendered Outset, and fixes a concurrent first-read race in Windows' environment cache.
The FPS diagnostic now respects the live 30/60/120 presentation setting instead of reporting healthy
30 FPS play with Smooth Motion off as a late-present failure. This is a logging fix, not an FPS gain.
See the [October 1 stability ledger](status/STABILITY_2026-10-01.md) for evidence and remaining gates.

Next priorities from that review:

| Request | Current boundary / next check |
| --- | --- |
| Apple TV | Developer target builds; add a controller-first settings/save-backup shell, then test a real TV |
| Switch Pro / Xbox face buttons | iOS remapping exists; desktop/TV need user-selectable layouts and hot-plug tests |
| No intro music | Reproduce the narrated intro with audio capture; do not infer correctness from Outset audio |
| Linux | No Linux build/play acceptance here; validate a source-only native host before claiming support |
| HD-style UI | BlueWake's desktop shell is restyled; the retail game's HUD is unchanged |

BlueWake is the intended maintained home for the combined project. Direct imports
retain Elliott Tate's original Git author identity. Commits that combine his
implementation with BlueWake adaptations should include
`Co-authored-by: Elliott Tate <elliotttate@gmail.com>`, using the identity recorded
in his source commits. Preserve these credits when merging or squashing the
integration PRs; separate BlueWake-only fixes and tests retain their own authorship.
The README also credits his rendering, desktop, gameplay and performance work.

The source stack and remaining acceptance checks are tracked in the current
reconciliation ledger. The fork stays intact during this migration; any retirement
notice follows acceptance of the consolidated BlueWake candidate. Repository
access changes are separate from source integration.
