# BlueWake next-candidate readiness

**Current migration status:** use the [reconciliation ledger](FORK_RECONCILIATION_2026-10-02.md)
and [goal loop](../GOAL_LOOP.md). Cumulative draft [PR #37](https://github.com/chrissotraidis/bluewake/pull/37)
is the current source review. The October 1 snapshot below is historical; its
then-open checks and PR references do not override the current ledger. The
private release audit and personal-build restrictions still apply.

Reviewed October 1, 2026. **Internal candidate only; not ready for public release.**
The maintainer's private release audit must mark BlueWake **Clear** before any public
release or app upload. Passing CI or a content scan does not clear that restriction.
Personal translated modules, apps containing them, discs, profiles and saves stay private.

## Recommended scope

Prepare a small stability update for the existing iPhone/iPad experience first, with
Apple Silicon Mac as a separately validated developer preview. Do not hold that update
for Linux, European discs, an HD-style game HUD or a public Apple TV/Windows launch.
Keep the imported features credited; their presence in source is not acceptance of
every option on every platform. Do not promise a general FPS increase or 120 Hz support.

The draft integration is [PR #10](https://github.com/chrissotraidis/bluewake/pull/10),
not main. The current version file is `0.1.0`, build `2`; it has **not** been bumped.
Freeze an exact source revision and dependency/module identities after candidate checks,
then choose the next version. Never move an acceptance result silently to a newer build.

Prioritized changes already in the draft:

- Normalize GameCube DMA R,L samples to L,R before playback/capture. Synthetic Mac
  sanitizer and native Windows regressions pass; live corrected playback is pending.
- Bound instant-text message parsing and prevent partial writes on rejected input.
  Synthetic Mac sanitizer and native Windows regressions pass; new-option gameplay is pending.
- Preserve replaced Mac launcher cards, fix Windows environment-cache/clock races,
  and make slow-frame logs respect the selected 30/60/120 presentation mode.
- Retain optional movement/touch controls and the BlueWake icon/menu work, without
  enabling extra gameplay options in existing player preferences.
- Retain the measured dispatch cost reduction. The two Outset checks reduce retired
  instruction work by 0.96% and 0.81%; they do not demonstrate a general FPS improvement.

Detailed evidence and failed experiments: [stability ledger](STABILITY_2026-10-01.md)
and [integration/platform evidence](../FORK_INTEGRATION.md).

## Current platform boundary

| Platform | Evidence available | Gate before claiming the next candidate works |
| --- | --- | --- |
| iPhone / iPad | Earlier app builds and a short physical M2 iPad Outset/control check | Latest host/module build, in-place upgrade/save readback, audible intro, controls and sustained gameplay; separately test an iPhone before making iPhone performance claims |
| Apple Silicon Mac | Earlier native Metal gameplay and headless intro/music captures | Latest candidate app/module, audible intro, hands-on final menu/input, fresh launch and save/reload, clean performance comparisons |
| Windows | Native source-only full app linking and seven regressions pass | Personal module build, Direct3D gameplay, speaker output, settings/restart, controller and save/reload on a real Windows machine; no Parallels restart |
| Apple TV | tvOS target builds; contributor reported title-screen boot | Real TV gameplay/audio/controller, usable settings and durable save backup; purgeable Caches and retagged Dawn remain preview limits |

The latest BMG/stereo local host and iOS/tvOS builds still await completion of the personal
module compile. At the maintainer's request, the build was interrupted at 661/822 steps
and its sequential validation queue was stopped on October 1. Completed objects and
private inputs remain local for a later incremental resume. Older physical checks do not
cover those changes. No all-platform release is currently supported by the evidence.

## Latest public reports to investigate

BlueWake had no open issues at this review. These are reports against the contributor's
fork, **not established BlueWake defects**:

- [Windows settings/startup crash (#5)](https://github.com/elliotttate/Wind-Waker-Recomp/issues/5):
  the attached log ends with access violation `0xC0000005` at address zero just after
  startup. LLE and several mods were active. The log has no stack identifying the
  failing function; neither a settings cause nor a BlueWake fix is established.
- [Opening bird-scene slowdown (#6)](https://github.com/elliotttate/Wind-Waker-Recomp/issues/6):
  get device/build/settings and confirm the same scene before comparing frame times.
- [Narrated-intro music (#1)](https://github.com/elliotttate/Wind-Waker-Recomp/issues/1):
  expected music is present in tested Mac PCM, but affected-platform speaker reproduction
  is still missing. Stereo correction is **not** a demonstrated missing-music fix.
- [Switch Pro / Xbox mapping (#2)](https://github.com/elliotttate/Wind-Waker-Recomp/issues/2):
  iOS remapping exists; desktop/TV face-button mapping and hot-plug acceptance remain open.

Linux, European-disc support, portable mode and HD-style HUD requests belong in the
follow-up backlog. Do not label unsupported disc variants as supported or remove USA validation.

## Acceptance checklist for the frozen candidate

All entries below are **pending for the next frozen candidate**, even where older checks passed.

- [ ] Record exact source/dependency revisions, app build, personal module identity,
  training mode and settings. Complete the native Mac host/28-test queue and iOS/tvOS
  target builds; run native Windows source-only regressions on that revision.
- [ ] Confirm all new module option exports and fifteen options, then run fresh-card
  intros with mods off, Better Wind Waker defaults, and instant text off. Preserve
  expected-track comparisons with both mono and stereo metrics; keep intro skipping off.
- [ ] Hear the narrated-intro music through the actual output device on each accepted
  platform. Check pause/resume and a scene transition; log/audio queue success is insufficient.
- [ ] On the physical iPad, back up saves/preferences, install in place with the same
  identity, read back preserved state, and test fresh boot plus an existing save.
- [ ] Test optional Jump/Run on and off, controller input and hot-plug, final menu navigation,
  options restart and normal memory-card save/reload. Never uninstall to repair an update.
- [ ] Compare a baseline and candidate on the same device, trained-module mode, settings,
  scene and thermal conditions. Include the opening bird scene, Outset and a second busy
  area, with a sustained 30-minute session. Record game speed, displayed/game FPS, frame-time
  distribution, worst stalls, audio interruptions and crashes, not only an average FPS.
- [ ] Rebuild through the documented own-disc player workflow and confirm which exact
  source the builder/PadMint selects. A local developer profile is not player-build proof.
- [ ] Review unresolved reproducible crashes, save failures, audio and progression blockers.
  Fix them or narrow the accepted platform/scope; keep known limitations in release notes.
- [ ] Obtain private **Clear**, then audit the exact proposed public source/app-only files
  using `scripts/release/check_public_assets.sh <artifact>...` and PadMint audit where
  applicable. A failure stops publication. Never publish a personal game-containing build.

Once these gates pass, announce only demonstrated fixes, tested platforms/settings and known
limits, with contributor credit. No publication, version bump or merge is implied by this checklist.

The ordered save/crash/audio/control follow-up and its remaining hardware gates are
recorded in [LOCAL_STABILITY_2026-10-01.md](LOCAL_STABILITY_2026-10-01.md).
This does not change the release pause or establish release readiness.
