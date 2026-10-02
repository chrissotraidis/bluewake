# BlueWake cloud-agent handoff, October 1, 2026

Local engineering stopped at the maintainer's request. The unbudgeted stability goal is
paused, not complete. This document is source-only: private inputs and build artifacts are
not on GitHub. Do not expect the cloud environment to contain the developer's disc/module.

## Prompt for the incoming agent

You are taking over BlueWake at https://github.com/chrissotraidis/bluewake.
Your goal is to resolve open and known issues, integrate worthwhile enhancements with
proper contributor credit, fix reported narrated-intro audio, improve measured performance,
and earnestly qualify the game's stability. Stability, saves and playable progression
take priority over new features, broad platform claims or finishing quickly.

Read AGENTS.md first. Public releases are paused until the maintainer's private audit marks
BlueWake Clear. Do not publish releases, app builds or download links. Never upload or
commit personal discs, translated modules/source, game-containing apps, profiles, saves,
signing material or captured game/audio data. Work with public source and synthetic tests;
request authorized private/hardware validation where essential instead of fabricating proof.
Keep the product named BlueWake and the player workflow named PadMint. Preserve GPL and
upstream/mod attribution. Do not restart the maintainer's Parallels VM or delete player data.

Start from the latest codex/fork-parity branch, not stale main. Existing draft PR #10 contains
the credited integration and stability work. Create a codex-prefixed continuation branch
from that head if isolation is needed; avoid overwriting concurrent work. Read these current
ledgers before the much older historical status logs:

- docs/status/RELEASE.md: next-candidate scope and outstanding acceptance gates.
- docs/status/STABILITY_2026-10-01.md: experiments, regressions, exact limits and failed approaches.
- docs/FORK_INTEGRATION.md: integration, physical iPad checks and platform boundaries.
- docs/WINDOWS.md and docs/status/TVOS_BUILD.md: experimental platform foundations.
- docs/BUILD_YOUR_OWN.md: player-owned-disc builder and personal-build rules.

### Phase 1: inventory and core stability

Refresh all open issues and PRs on BlueWake and the public contributor fork,
https://github.com/elliotttate/Wind-Waker-Recomp. BlueWake had no open issues at the handoff;
that is not evidence that the game has no bugs. Review PRs #3/#5/#6/#8 against their source
already integrated into #10, retaining credit and avoiding duplicate cherry-picks. Review
the separate SDK-preflight PR #11 before integrating it. codex/skip-black is an older
diagnostic branch backed up for reference, not a proposed stable feature.

Create a concise issue/evidence list with severity, reproduction, affected platform/build,
current owner, acceptance test and disposition. Separate confirmed defects from unverified
reports and enhancement requests. Reproduce, make the smallest justified fix, add a regression,
validate, and repeat. Do not close issues solely because the build passes. After repeated
identical failures, change the experiment rather than rerunning indefinitely.

Latest fork leads at the handoff:

- #1: no music during narrated intro; platform/build/settings unspecified.
- #5: Windows 11 build 0.2.1 crashes after settings changes and won't relaunch. Attached log
  ends in 0xC0000005 at address zero just after startup with LLE and several mods active.
  No stack identifies the failing function; settings/LLE are leads, not established causes.
- #6: opening bird scene very slow; confirm scene/device/settings before benchmarking.
- #2: Switch Pro A/B and Xbox face-button mapping; iOS remapping exists, desktop/TV need work.
- #3/#4/#7: Linux, HD-style UI/portable mode/controller menu navigation, European discs.
  Treat these as enhancement requests. USA GZLE01 revision 0 is the only validated input;
  do not bypass disc checks to claim PAL support.

### Phase 2: audio and performance with actual evidence

The tested Mac intro PCM contains the expected music, including HLE/LLE and original/fast
transitions. This does not prove affected-platform speaker playback. Reproduce with actual
platform/build/settings and check the complete stream-to-output path. Test fresh-card intros
with mods disabled, Better Wind Waker defaults and instant text disabled, keeping intro skip
off. Validate both mono and stereo music comparisons plus actual output-device audibility.

An identified stereo-order defect was fixed at c984ceb: guest DMA is big-endian R,L and the
host now normalizes pairs to L,R once before capture/playback. Its Mac sanitizer and Windows
regressions pass; corrected live playback remains pending. This is NOT a demonstrated fix
for the missing-music report. BMG instant-text bounds/no-partial-write tests pass at f2f6486,
but latest-module gameplay remains pending. Read the ledger rather than changing DSP speculatively.

Profile reproducible opening/Outset and a second busy-area workload. Match device, settings,
training mode, thermal state and route; use repeat/control order, frame-time tails, game speed,
stalls, CPU/GPU costs and audio interruptions. Separate 30 Hz game updates from 60/120 displayed
interpolated frames. Do not claim interpolation makes the simulation run faster. The dispatch
filter reduced retired instructions by 0.96%/0.81% in two Outset checks, not general FPS. Failed
GX replay and scene-unmatched Dolphin captures are not valid baselines. Do not relax failed checks.

### Phase 3: qualification, then platform completion

First qualify the integrated core/enhancements: latest build and regressions, fresh boot,
opening-to-play progression, settings/restart, optional touch controls on/off, controller
hot-plug, audible sound, memory-card save/reload, upgrade preservation and sustained gameplay.
Use the candidate checklist, including a matched 30-minute session and player-builder check.
Record exact tested source/app/module identities and limitations. If hardware or private inputs
are unavailable, mark those gates unverified and request the missing validation; do not substitute
CI, mocks, screenshots or synthetic PCM for gameplay acceptance.

Only once the core is demonstrably stable and worthwhile integrated enhancements pass their
acceptance tests should you finish Windows, tvOS and Linux as supported platform candidates:

- Windows: native personal-module build, Direct3D 12 gameplay, settings/restart crash testing,
  speaker audio, controller remapping/hot-plug, save persistence and fresh-machine dependencies.
  Current source-only native app linking plus seven regressions passes, not Windows gameplay.
- tvOS: native dependency/build path, controller-first settings and safe save backup/storage,
  real Apple TV progression/audio/controller/performance. Current target builds; retagged Dawn
  and purgeable Caches remain limitations, not production acceptance.
- Linux: establish a source-only native build/CI first, then an authorized personal-module path,
  actual supported graphics/audio/controller gameplay, packaging and durable saves. There is no
  accepted Linux build here. Proton testing does not establish native Linux support.

### Deliverables and completion standard

Keep patches focused and tests proportionate; no unnecessary redesign or broad default changes.
Push source-only continuation work and open/update an appropriate PR. Keep a concise evidence
ledger: fixed, reproduced-but-open, enhancement/deferred, and hardware/input-blocked. Deliver a
credible next-candidate recommendation and honest known-issues/platform matrix. Do not mark
the whole game stable, all issues resolved, a platform supported or a release ready while its
required gates are unverified. Publication remains separately paused even after engineering passes.

## Local-only resume note

The separate optimized arm64 personal module compile was intentionally interrupted at 661/822
steps, using O2, three existing O1 fallback sources and two jobs. Its queued Mac build/28 CTests,
three new-option intro captures, and latest iOS/tvOS target builds did not run to completion.
Completed objects and private fixtures remain in the maintainer's ignored build/fork-parity tree.
They were not deleted or uploaded. A cloud agent must not assume access to them; reconstruct only
from authorized player inputs or hand the exact remaining tests back for local/device execution.
