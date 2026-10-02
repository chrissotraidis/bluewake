# Fork reconciliation ledger, October 2, 2026

Engineering reconciliation is **in progress**. This ledger owns the remaining
parity/build/gameplay work; older ledgers retain their exact historical evidence.
Source integration and synthetic checks are not gameplay acceptance. Releases
remain paused; personal game code, builds, inputs and captures remain local.
Do not publish a fork redirect or close the donor while this work is incomplete.

## Fixed baseline

Live refs were checked on October 2 JST with `git ls-remote` and GitHub PR/issue
reads. Reconcile these snapshots; record later donor updates separately.

| Repository / ref | Exact commit / disposition |
| --- | --- |
| BlueWake main | `72138a1eccd1800f4d85bf9718913b1d743cda32`; not the integrated candidate |
| BlueWake PR #10, `codex/fork-parity` | `22dca5d2a4e8d9c3589b22ec4163f5111a5cceaa`, open draft |
| BlueWake PR #12, `claude/git-commit-author-config-d4sdwt` | Initial `ad7f5512bdd04a0d923f77c98788a37421b3c917`; checkpoint `736f178` adds the previously unfinished runtime pin/patch/CI registration; stacked on #10 |
| Donor `main` | `e021b71bcb560109fb5c1b64cfb74f3801f6d189` |
| Donor `windows-release` | `f960ca344814fa10cb9ad692402822851b3ffeaa` |
| Donor RecompCore `windows-release` | `634895470af6e61e601f06a351f3a72888215159` |
| Donor DolRecomp `bluewake` | `b8b534591cba8ca7cd43943a655ee6e2591cf5de` |
| Maintained RecompCore `bluewake-next` / build-selected pin | `99e4748002d42c1a86fdcb33a47cd0e97292acff`; retains BlueWake safety fixes through patch 0130 |
| Build-selected DolRecomp submodule / profile / lock | `b8b534591cba8ca7cd43943a655ee6e2591cf5de`, matching the donor |

PR #11's two SDK-preflight commits require reconciliation against #12's existing
preflight behavior; do not duplicate them by assuming they are absent. PRs #3/#5/
#6/#8 already have integrated source/credit in #10; leave the originals open.
The existing checkout was used. Its eight unfinished source changes were backed
up locally, reviewed, checked and checkpointed in #12 without dropping them.
No additional clone/worktree was created or existing worktree removed.

## Required parity inventory

**Integrated** means code is present with bounded evidence, not fully accepted.
**Open** means missing implementation or required validation. Native/translated
optimizations must be audited for source provenance before copying them; game
translation belongs in each player's private workspace.

| Feature / platform | Donor source / dependency | BlueWake state / difference | Required acceptance / status |
| --- | --- | --- | --- |
| Interpolation, batching, upload staging, worker/cache improvements; Apple/Windows | main `b6f87e0`, `7f4f1a0`, `e5c6ae4`, `b39bd0d`; runtime through `9618e9d` | Integrated in #10 plus newer synchronization fixes in #12 | Integrated; final same-scene images, 60/120 pacing and sustained play open |
| Swapchain/fullscreen and orderly restart; Windows | `db2944e`, `1ce29ac`; runtime `9618e9d` | Swapchain fix already in maintained runtime; #12 orderly quit/relaunch replaces donor process-exit approach | Integrated; actual Windows F11/Restart/settings/startup recovery open |
| Disc picker, remembered disc, compressed import and recovery; Windows | `0b463cc`, `61e891e`, `640094e`; shared `disc_import.c`, nodtool | Missing Windows launcher flow; builder supports ISO/compressed input. Donor deletes converted images after rejection and can overwrite the prior converted disc; preserve input/recovery instead | Open; supported/rejected/missing disc, interruption/resume, Unicode paths and preserved prior files |
| Disc picker / launcher; Apple Silicon Mac | main `561ddbf`, `7a2ac23`, `5eaa981` | Developer host route exists; donor bundled-game packaging is intentionally excluded | Open; personal clean source build and app-only launcher with player's generated module, save/reload and installation |
| Camera, right-stick aiming/zoom/collision; desktops | main `887c26d`, windows `9511241` | Latest SDL queue timing integrated with window-scoped filtering; prior BlueWake camera/menu code retained | Integrated; actual SDL queue regression passes; real mouse/controller/camera timing acceptance open |
| Jump, sprint, quick doors, transitions; Apple/Windows | main `22fa284`, `d55ce11`, `2c9f16c`, `df62ae0` | Integrated; Apple touch controls use existing editor, new options remain opt-in | Integrated; final new-module progression and controls on/off open |
| Fifteen Better Wind Waker options, 16:10; all claimed targets | main `b6f87e0`; DolRecomp `b8b5345` | Integrated; same verified base-source digest; new options require module rebuild, legacy fallback retained | Integrated; clean player-generated module, all exports/options and relevant gameplay open |
| Desktop save states and climbing | main `b39bd0d`, windows `3ba8599`, `1510ed1` | Integrated; #12 adds checked/atomic serialization; states experimental, climbing off | Integrated; current module state compatibility, real save/load and climbing acceptance open; no new Apple touch state UI claimed |
| Controller face layouts/navigation | donor reports #2/#8/#14; existing SDL controls | #12 adds A/B and X/Y swaps, navigation/game-input isolation and virtual-controller checks | Integrated; real Switch Pro/Xbox/8BitDo hot-plug/menu/closing-input checks open; arbitrary remap is separate scope |
| Prepared-block/global-register module optimizations; Windows | `f319afa`, `8435ec7`, `16fabda`; scripts/windows transformers and cmake/composite helpers | Absent; donor native batches include experiments and recovered-source replacements requiring provenance review | Open; isolate generic transforms from native/decomp work, private generated-code correctness and matched before/after performance |
| Native math/J3D/skin/vector replacements; Windows | `689f042`, `845a589`, `833e6fb`, `1e97dca` | Absent; contributor's certification/benchmarks do not transfer to BlueWake | Open; public-source provenance, per-function exactness/fallback, same-module state/image checks and matched measurements |
| Global MEM1 / inline guest-memory accesses; Windows | `f70305c`, `6def7cd`; runtime `460b5b84`, `63489547` | Absent; current CPU/module ABI and safety fixes must survive | Open; aliases/MMIO/reservations/journaling/bounds/dispatch checks, clear compatible-module/rebuild behavior and matched measurements |
| Overload suspension and display-rate interpolation, up to 240 | `f70305c`; runtime `63489547`; UI `4fedbfc` | Only 30/60/120 presentation exists; 30 Hz logic and interpolation off remain defaults | Open; runtime pacing regressions, window/display-change tests, preference preservation, low-end workload and images |
| Save durability, failed startup/audio recovery; all claimed targets | Current reports; donor initial mechanisms | #12 adds stronger atomic card/settings/state writes, lock/recovery, crash logs, launch marker, sink audio recovery; retained over donor files | Integrated; real error/restart/output-device/upgrade acceptance open, no power-loss guarantee |
| Apple identity, touch, saves, settings, tvOS | BlueWake contributions incl. Ian MacFarlane #3, #6, #8 | Preserve existing Apple shell and BlueWake styling; tvOS remains preview with separate hardware/storage gates | Integrated; latest iPhone/iPad candidate acceptance open; TV parity only where claimed, no physical TV evidence |
| Source build from owned USA rev-0 disc | BlueWake shell builder; donor Windows builder | iOS local training defaults; Windows training/optimization path incomplete; developer artifacts are not clean-build evidence | Open; fresh output, pinned public sources, validation/translation/mods/train/compile/package, interruption/resume, local personal run/save/reload |
| PadMint | `padmint.json`, docs/PADMINT_HANDOFF.md; actual selected PadMint adapter/revision must be refreshed | Manifest: experimental iOS on Apple Silicon, macOS planned; no Windows/tvOS adapter claim | Open; full clean PadMint iOS build, selected source/pins/provenance, audit, in-place device save/reload and matched trained performance |
| Public app-only candidate and compatibility | BlueWake `--app-only` / `--app` model | Donor game-containing distribution excluded; keep own-disc modules local | Open; exact app-only audit, compatible old-module update vs explicit rebuild, no download/publication while audit paused |

Experimental 60 Hz simulation (`8435ec7`, `4b6b268`) is optional work. If imported,
it must default off, require explicit selection and reject incompatible modules.
Linux, broader disc support and broader decompilation are follow-up scope.
Never infer simulation speed from displayed/interpolated FPS.

## Current report reconciliation

Read issue bodies/comments on October 2; these are leads, not diagnoses/fixes.
No external issue comments or messages were sent during this inventory.

| Report | Evidence / disposition | Next discriminating check |
| --- | --- | --- |
| [BlueWake #13: Pictobox](https://github.com/chrissotraidis/bluewake/issues/13) | Windfall shutter closes, picture stops, sound continues; reports Windows 11/120 FPS but exact product/module unclear. Maintainer already asked for identity | Establish exact build; isolated Windfall save, interpolation off/on, capture GPU/EFB copy and game-thread progress. No reproduced cause |
| [Donor #16: startup freeze](https://github.com/elliotttate/Wind-Waker-Recomp/issues/16) | 0.2.2 Windows logs reject PAL, then accept USA, initialize D3D12 on RTX 2050; stop after gxcore initialization. Also occurs on integrated GPU | Symbolized/blocked-thread evidence at first-frame boundary with exact app/module/pins; avoid assuming disc/settings/GPU cause |
| [Donor #5: settings/startup crash](https://github.com/elliotttate/Wind-Waker-Recomp/issues/5) | Earlier access violation at zero; #12 startup/restart safeguards do not establish a fix | Windows HLE/LLE and real Restart/settings reproduction; bounded crash/safe-mode recovery |
| [Donor #12: scripted music](https://github.com/elliotttate/Wind-Waker-Recomp/issues/12), [#1: intro](https://github.com/elliotttate/Wind-Waker-Recomp/issues/1) | #12 identifies Windows x86/0.2.2, ambient-only intro and bird/Zelda/sister/Ganon cues; Mac intro stream/sink evidence covers only its stated scene/build | Fresh-card and matching scripted-scene Windows guest/sink/output comparison, then audible output; do not extrapolate Mac intro captures |
| [Donor #6: bird slowdown](https://github.com/elliotttate/Wind-Waker-Recomp/issues/6) | Scene report; device/settings and matched baseline still needed | Matched quiet-device bird/Outset/busy-area serial measurements, frame-time tails, stalls/audio and 30-minute run |
| [Donor #2](https://github.com/elliotttate/Wind-Waker-Recomp/issues/2), [#8](https://github.com/elliotttate/Wind-Waker-Recomp/issues/8), [#14](https://github.com/elliotttate/Wind-Waker-Recomp/issues/14): controllers | Layout swaps cover part of request; unsupported 8BitDo mapping and arbitrary remap are not accepted | Real controllers, per-device layout/hot-plug/navigation/save preference checks |
| Donor #3/#18 Linux, #7 PAL, #4 HD HUD, #9 Switch, #10/#11 portable | Follow-up platform/UI requests; explicit data-directory override exists in Windows source | Record supported matrix; preserve unsupported-disc rejection, do not broaden claims |

## Evidence and next iteration

- Checkpoint `736f178` selects runtime `99e47480` consistently in profile/lock/
  device docs; exported patch 0130 is byte-identical to the runtime commit.
  Actual runtime test includes the 38 truncated-vertex cases. On this M3 Max,
  all **243/243 Mac CTests passed** (2.78 seconds) and the repository audit passed.
  This reused built test binaries; the prior overnight ledger records their
  builds. Native Windows CI at this checkpoint is pending; earlier 26/26 at
  `ad7f551` does not cover patch 0130.
- Earlier physical iPad bridge/bird checks and Mac intro/skip checks are precisely
  scoped in [overnight](OVERNIGHT_2026-10-02.md) and
  [local stability](LOCAL_STABILITY_2026-10-01.md). They are not acceptance of
  the whole current source or the donor Windows optimizations.
- Next: import the missing desktop input/disc foundation in focused batches,
  then reconcile the two newer runtime commits with BlueWake safety fixes.
  Qualify generic module transforms separately from native math and 60 Hz work.
  Use clean player build directories rather than old developer module caches.
- Migration preparation stays a proposal: credited unified docs and issue map,
  then a proposed notice only after every required row has acceptance evidence.

No gameplay, performance parity, complete player-build acceptance, migration
acceptance or public release readiness is claimed by this inventory.

## Desktop mouse input batch

Imported `9511241868095a609202d00ccf504da3a90f6135` with Elliott Tate's
authorship. Camera updates pump fresh pointer motion only while captured and
unblocked. BlueWake filters only motion for the captured game window, retaining
other-window motion and key/wheel/quit events in queue order. This avoids the
donor's all-window drain. `BLUEWAKE_MOUSE_FRESH=0` retains the prior timing;
test-queue/latency switches are opt-in and do not change stored preferences.
Apple touch behavior remains guarded out.

M3 Max: native Mac host and iOS app compile/link. The real SDL event-queue test
covers 81 game-window motion events, another window's motion, key/wheel/quit
order, no double consumption, disabled/uncaptured/blocked/zero-window states.
The mouse-motion, virtual-controller and face-layout CTests all pass (3/3).
Native Windows compilation/test is registered in source-only CI and pending.
This does not establish a measured latency improvement, rendered aiming or
physical controller/mouse acceptance.

## Windows disc foundation batch (isolated while the player build runs)

Branch `codex/windows-disc-parity` continues the reconciliation branch in a
managed `.codex/worktrees/bluewake-disc-parity/bluewake` checkout. It imports
Elliott Tate's Windows disc UI from donor `0b463cc`/`61e891e`/`640094e`, retaining
his authorship. The current shared importer and BlueWake startup/crash/safe-mode,
settings, saves and defaults are retained. No personal or donor binary is copied.

BlueWake adaptations: each conversion/preparation gets a new directory; failed
converted inputs are retained; previous prepared files survive a failed attempt;
atomic text records select only a completed preparation. A missing REL retries
preparation, and explicit compressed disc input reaches the converter. A missing
player-generated module gets an actionable source-build message before any import.
The builder includes its existing nodtool only if available. No default experiment
is enabled.

The native Windows fixture compiles the actual launcher with synthetic importer
results and uses actual Win32 file/process APIs. It covers cancel/missing/rejected
input, cached reuse, failed preparation preserving the old selection and files,
retry, incomplete REL cache, and a real child converter whose rejected synthetic
output cannot replace/delete the previous converted disc or original input.
The source audit and whitespace checks pass; native compilation/execution is
pending CI. This does not accept the real importer/UI/gameplay or Unicode paths.

The first native Windows run compiled/linked the complete app but the disc
fixture timed out at cancellation (28 other regressions passed). Windows
`_putenv_s(name, "")` removes the variable, so an empty picker override reached
a modal despite the no-dialog flag. The picker now treats no-dialog/no-choice
as cancellation. The same regression remains enabled with its 30-second limit;
the timeout was not extended. Follow-up native execution is pending.

## Fresh player-source and app-only checks

On the M3 Max, the direct builder generated source from a supported personal
USA rev-0 disc in a new `build/reconciliation/player-source` directory. It
recompiled the translator/tool outputs, translated 206 DOL chunks and 415 RELs,
and generated 748 chunks / 417 ranges with the expected `54f54434…770a` digest.
The canonical old developer disc path was absent; the preserved personal disc
backup was used read-only. No player save or unpublished profile was an input.

PadMint source `018a9f0fdd2030d2fa9f328617d6541fd83c9af1` (0.2.8) reproduced a
manifest bug at BlueWake `491b375`: source-only passed an empty `--app` and
failed before dependencies. Moving `--app` to the full mode fixes it. A new
PadMint workspace completed source generation at BlueWake
`95adeed0f32a39739341dd9381c50756c5247bf4` in 16.6 seconds; the local record
says completed/source-only and the expected digest matches. PR #11's SDK
manifest prerequisites and disk guidance were cherry-picked with authorship;
no duplicate source preflight implementation was invented.

A new local app-only iOS build at `491b375` / runtime `99e47480` compiled and
packaged with no game module. Executable SHA-256:
`b32ed8688cb10232b591c770cf1ce05f3e442e71d985b0b9a2ccd6e605bc7b48`.
Both `scripts/release/check_public_assets.sh` and PadMint's audit passed.
The app-only candidate remains local; this is not publication authorization or
physical gameplay acceptance.

Native Windows source-only app linking and **28/28** regressions pass at
`95adeed` in [run 36946613444](https://github.com/chrissotraidis/bluewake/actions/runs/36946613444).
This includes the camera queue regression and runtime patch 0130; it does not
cover the later disc batch or prove Windows gameplay.

The direct default-training full builder is running separately against fixed
BlueWake `95adeed` / runtime `99e47480`, with fresh generated mods/training
module and an isolated new card. No old generated game module or developer game
profile is reused. Record terminal training/compilation/package outcomes before
counting the full build, then run PadMint's complete path serially. Partial
training, an app-only IPA and source generation are not full acceptance.

## Newer runtime foundation in isolation

[RecompCore draft PR #1](https://github.com/chrissotraidis/RecompCore/pull/1)
rebases donor `460b5b84` (optional global MEM1) and `63489547` (display-rate /
slow-game interpolation handling) onto maintained `99e47480`, preserving the
existing safety fixes and Elliott's authorship. Its branch is
`codex/bluewake-runtime-parity`, current `45364adf`; the primary build/pin is
unchanged pending validation. The nested runtime worktree is inside the managed
disc worktree; retain both until their source/evidence is reconciled.

A new actual-header probe reproduced acceptance of an oversized guest memory
span because unsigned capacity-minus-size underflowed. The fix checks size
before subtraction in plain/global MEM1 and MEM2. Both plain/global variants
pass ASan/UBSan and 2/2 CTests against a fresh core build, covering bounds,
mirrors, aliases and empty banks. This is not a diagnosis of Pictobox/startup
reports. Full renderer/host compilation and interpolation checks are running
in the isolated tree; the expanded display behavior and performance still need
matched gameplay evidence. Existing modules and experiments-off defaults remain.
