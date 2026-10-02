# Fork reconciliation ledger, October 2, 2026

**In progress.** Chris and Elliott have agreed to consolidate development in
BlueWake. This migration directly imports Elliott's enhancements, preserves his
authorship and credits combined implementations with co-author trailers.
Source integration is reviewable in stacked draft PRs; required
performance, player-build and gameplay acceptance remains open. No release,
fork redirect, issue closure or migration acceptance is authorized. Personal
modules, generated source, discs, profiles, builds, saves and captures stay local.
Public app-only candidates still require a Clear private audit and the content gate.

This is the authoritative current ledger. The
[evidence appendix](FORK_RECONCILIATION_EVIDENCE_2026-10-02.md) preserves the earlier
experiments, failed approaches and exact historical checkpoints. “Integrated”
below means source is present with bounded evidence, not that parity is accepted.

## Fixed baseline and selected dependencies

Reconcile the fixed donor main **and** windows-release snapshots below. Track
later donor changes separately. Baseline refs were verified October 2 JST.

| Repository / ref | Exact commit / disposition |
| --- | --- |
| BlueWake main | `72138a1eccd1800f4d85bf9718913b1d743cda32`; not the integrated candidate |
| BlueWake PR #10, `codex/fork-parity` | `22dca5d2a4e8d9c3589b22ec4163f5111a5cceaa`, open draft |
| BlueWake PR #12, `claude/git-commit-author-config-d4sdwt` | Initial `ad7f5512bdd04a0d923f77c98788a37421b3c917`; checkpoint `736f178` adds the previously unfinished runtime pin/patch/CI registration; stacked on #10 |
| Donor `main` | `e021b71bcb560109fb5c1b64cfb74f3801f6d189` |
| Donor `windows-release` | `f960ca344814fa10cb9ad692402822851b3ffeaa` |
| Donor RecompCore `windows-release` | `634895470af6e61e601f06a351f3a72888215159` |
| Donor DolRecomp `bluewake` | `b8b534591cba8ca7cd43943a655ee6e2591cf5de` |
| Baseline maintained RecompCore (PR #12) | `99e4748002d42c1a86fdcb33a47cd0e97292acff`; retains BlueWake safety fixes through patch 0130 |
| Build-selected DolRecomp submodule / profile / lock | `b8b534591cba8ca7cd43943a655ee6e2591cf5de`, matching the donor |


The current candidate's lock and builder profile both select maintained runtime
`c2905b7a6b2752611a4e1ccb2ac9e3cda1857462` (runtime PR #2, based on #1 / `70bc9957`).
Patches through 0135 preserve BlueWake's safety changes. The translator remains
`b8b534591cba8ca7cd43943a655ee6e2591cf5de`. Ordinary modules use ABI 3;
fixed-CPU modules declare ABI 4; fixed-CPU plus module-owned RAM declares ABI 5.
Old hosts reject unsupported ABIs before player storage. Apple shell support for
the new storage ABIs is not claimed.

## Source checkpoints and integration order

All listed BlueWake PRs remain drafts. Source checkpoints are evidence identities;
a later documentation-only commit may change the PR head. Preserve the original
contributor authorship, eight pre-existing edits checkpointed in #12, and other PRs.
PR #11's SDK preflight is reconciled by #14; do not import it again.

| PR | Source checkpoint | Scope / next gate |
| --- | --- | --- |
| [#10](https://github.com/chrissotraidis/bluewake/pull/10) | `22dca5d` | Earlier fork integration; base for #12 |
| [#12](https://github.com/chrissotraidis/bluewake/pull/12) | `736f178` | Preserved stability work and runtime safety pin |
| [#14](https://github.com/chrissotraidis/bluewake/pull/14) | `95adeed` | Fresh mouse queue, SDK preflight and source-only PadMint repair; primary player-build baseline |
| [#15](https://github.com/chrissotraidis/bluewake/pull/15) | `54bbb26` | Windows disc import/recovery foundation; real UI/disc acceptance open |
| [#16](https://github.com/chrissotraidis/bluewake/pull/16) | `5c8d348` | Maintained runtime/display-rate policy; physical pacing/play open |
| [#17](https://github.com/chrissotraidis/bluewake/pull/17) | `5208567` (docs `05df605`) | Conservative prepared blocks; O2 pair compiling, measurements open |
| [#18](https://github.com/chrissotraidis/bluewake/pull/18) | `6532973` (docs `f1c1c7a`) | Fixed CPU / ABI 4; bounded arm64 O0 checks pass |
| [#19](https://github.com/chrissotraidis/bluewake/pull/19) | `3222b91`, fixture `0e78428` | Module RAM / ABI 5 and extended-alias fix; bounded arm64 O0 checks pass |
| [#20](https://github.com/chrissotraidis/bluewake/pull/20) | `9d3729a` | Corrected inline FP; native instruction tests and arm64 O0 module/boot pass |
| [#21](https://github.com/chrissotraidis/bluewake/pull/21) | `7b7e530`, Windows includes `9303569` | Active Release assertions and restored GroundCross observation; bounded host check and native Windows pass |
| [#22](https://github.com/chrissotraidis/bluewake/pull/22) | `69426b2` | Per-turn consecutive dispatch counter; unit/sanitizer, strict module boot and native Windows checks pass |
| [#23](https://github.com/chrissotraidis/bluewake/pull/23) | `b85f931` | Gather/inline-memory contract foundation; two reproduced guards, local contracts and all 41 native Windows tests pass; module/renderer/performance qualification open |
| [#24](https://github.com/chrissotraidis/bluewake/pull/24) | `608edaa` | Independent gather-module preparation; nine cache tests, strict function/boot comparisons and 41 native Windows tests pass; host batching not selected |
| [#25](https://github.com/chrissotraidis/bluewake/pull/25) | `80eb353` | Explicit host writer/batching; 42 native Windows tests and bounded Mac route/card/intro-image comparisons pass; 23,000-retrace route/card match; player-control trigger not reached |
| [#26](https://github.com/chrissotraidis/bluewake/pull/26) | `9de9cf2` | Optional complete-coverage checkpoint comparisons; six CPU/RAM/REL hash checkpoints match over 6,000 headless retraces; Aurora checkpoints and two intro frames match; native Windows host build passes |

Runtime [PR #1](https://github.com/chrissotraidis/RecompCore/pull/1) at `70bc9957`
rebases donor global MEM1/display overload work with bounds checks. Runtime
[PR #2](https://github.com/chrissotraidis/RecompCore/pull/2) at `c2905b7a` fixes
alias tracking above retail 24 MiB. Both are drafts; all public checkpoints are
source only. No source merge or test result implies release readiness.

## Required parity inventory

| Feature / platform | Donor source / dependency | BlueWake state / difference | Required acceptance / status |
| --- | --- | --- | --- |
| Interpolation, batching, upload staging, worker/cache improvements; Apple/Windows | main `b6f87e0`, `7f4f1a0`, `e5c6ae4`, `b39bd0d`; runtime through `9618e9d` | Integrated in #10 plus newer synchronization fixes in #12 | Integrated; final same-scene images, 60/120 pacing and sustained play open |
| Swapchain/fullscreen and orderly restart; Windows | `db2944e`, `1ce29ac`; runtime `9618e9d` | Swapchain fix already in maintained runtime; #12 orderly quit/relaunch replaces donor process-exit approach | Integrated; actual Windows F11/Restart/settings/startup recovery open |
| Disc picker, remembered disc, compressed import and recovery; Windows | `0b463cc`, `61e891e`, `640094e`; shared `disc_import.c`, nodtool | Launcher source integrated in #15; unique import folders retain original/converted/previous files; candidate native 29/29 regressions pass | Integrated; real supported/rejected/missing disc and UI/Unicode interruption/recovery checks open |
| Disc picker / launcher; Apple Silicon Mac | main `561ddbf`, `7a2ac23`, `5eaa981` | Developer host route exists; donor bundled-game packaging is intentionally excluded | Open; personal clean source build and app-only launcher with player's generated module, save/reload and installation |
| Camera, right-stick aiming/zoom/collision; desktops | main `887c26d`, windows `9511241` | Latest SDL queue timing integrated with window-scoped filtering; prior BlueWake camera/menu code retained | Integrated; actual SDL queue regression passes; real mouse/controller/camera timing acceptance open |
| Jump, sprint, quick doors, transitions; Apple/Windows | main `22fa284`, `d55ce11`, `2c9f16c`, `df62ae0` | Integrated; Apple touch controls use existing editor, new options remain opt-in | Integrated; final new-module progression and controls on/off open |
| Fifteen Better Wind Waker options, 16:10; all claimed targets | main `b6f87e0`; DolRecomp `b8b5345` | Integrated; same verified base-source digest; new options require module rebuild, legacy fallback retained | Integrated; clean player-generated module, all exports/options and relevant gameplay open |
| Desktop save states and climbing | main `b39bd0d`, windows `3ba8599`, `1510ed1` | Integrated; #12 adds checked/atomic serialization; states experimental, climbing off | Integrated; current module state compatibility, real save/load and climbing acceptance open; no new Apple touch state UI claimed |
| Controller face layouts/navigation | donor reports #2/#8/#14; existing SDL controls | #12 adds A/B and X/Y swaps, navigation/game-input isolation and virtual-controller checks | Integrated; real Switch Pro/Xbox/8BitDo hot-plug/menu/closing-input checks open; arbitrary remap is separate scope |
| Prepared-block/global-register module optimizations; Windows | `f319afa`, `8435ec7`, `16fabda`; scripts/windows transformers and cmake/composite helpers | Generic prepaid-block transform and portable strict A/B fixture imported; fixed-CPU preparation from `4b6b268` added separately with a declared module ABI and explicit builder opt-in. Both default off; module-owned MEM1 is integrated separately in #19, native batches remain open | Open; isolate generic transforms from native/decomp work, private generated-code correctness and matched before/after performance |
| Inline floating-point interpreter operations; Windows | `4b6b268`; `inline_fp.h`, chunk header preparation | Separate `--inline-fp` opt-in, off by default; no game-native replacements or ABI change | Open; Corrected helper: arm64/Rosetta and native Windows checks pass; 30,000 module cases and 6,000-retrace arm64 boot match. Optimized module/performance/gameplay open |
| Direct cross-chunk/indirect calls and inline GPR save/restore; Windows | `4b6b268`, `742f1a0`, `76c688d`, `7576dc9` | Imported directly with Elliott's authorship in `d176f3e`; candidate adds versioned host readiness, shared feature predicates and guarded/certified register inlining. Builder options remain independent and off by default | In progress; local runtime/preparation/cache tests pass and all 823 private O0 module units link. Active function/route/image comparisons, native Windows and matched timing remain required |
| Gather-pipe batching and inline memory wrappers; Windows | `3e14287`, `6def7cd` | Generic helper foundation imported with original authorship; separate default-off `--gather-pipe` prepares modules. Host direct writer/batching now has separate default-off selection; bounded Mac route/card/intro-image comparisons pass. Undersized-RAM and pipe-alias precedence regressions fixed | Open; source-only memory/flush fixtures and nine cache tests pass locally; 813 chunks prepare reproducibly and unprepared inputs are rejected. Arm64 O0 function/boot comparisons pass with the host writer disabled; optimized modules, sustained renderer/gameplay qualification and matched timing remain required |
| Native math/J3D/skin/vector replacements; Windows | `689f042`, `845a589`, `833e6fb`, `1e97dca` | Absent; contributor's certification/benchmarks do not transfer to BlueWake | Open; public-source provenance, per-function exactness/fallback, same-module state/image checks and matched measurements |
| Global MEM1 / inline guest-memory accesses; Windows | `f70305c`, `6def7cd`; runtime `460b5b84`, `63489547` | Optional runtime global MEM1 rebased in #16; explicit ABI-5 module storage adoption now in a separate default-off candidate. Extended alias guard fixed in maintained runtime `c2905b7a`; inline module wrappers imported and separately qualified in #23/#24 | Open; remaining MMIO/reservations/journaling/dispatch, module compatibility and matched measurements |
| Overload suspension and display-rate interpolation, up to 240 | `f70305c`; runtime `63489547`; UI `4fedbfc` | Runtime and Mac/Windows display settings source integrated in #16; three/seven-step regressions and rate/preference policy pass; 30 Hz logic and interpolation off remain defaults | Open; Off/120/Off/relaunch UI persistence passes on Mac; real display changes, pacing, slow-game workloads, images and matched performance remain open |
| Save durability, failed startup/audio recovery; all claimed targets | Current reports; donor initial mechanisms | #12 adds stronger atomic card/settings/state writes, lock/recovery, crash logs, launch marker, sink audio recovery; retained over donor files | Integrated; real error/restart/output-device/upgrade acceptance open, no power-loss guarantee |
| Apple identity, touch, saves, settings, tvOS | BlueWake contributions incl. Ian MacFarlane #3, #6, #8 | Preserve existing Apple shell and BlueWake styling; tvOS remains preview with separate hardware/storage gates | Integrated; latest iPhone/iPad candidate acceptance open; TV parity only where claimed, no physical TV evidence |
| Source build from owned USA rev-0 disc | BlueWake shell builder; donor Windows builder | Clean owned-disc iOS build/package and bounded resume pass at `95adeed`; final integrated-source build and actual run/save/upgrade still required. Windows training/build path incomplete | Open; fresh output, pinned public sources, validation/translation/mods/train/compile/package, interruption/resume, local personal run/save/reload |
| PadMint | `padmint.json`, docs/PADMINT_HANDOFF.md; actual selected PadMint adapter/revision must be refreshed | Manifest: experimental iOS on Apple Silicon, macOS planned; no Windows/tvOS adapter claim | Open; Fresh PadMint iOS translation/training, module compilation, assembly and provenance checks pass at the baseline; player signing/install, final-source selection, device save/reload and matched performance open |
| Public app-only candidate and compatibility | BlueWake `--app-only` / `--app` model | Donor game-containing distribution excluded; keep own-disc modules local | Baseline and candidate runtime app-only audits pass. Compatible old-module update vs rebuild and physical acceptance open; no publication |
| Consecutive non-advancing dispatch bound; desktop modules | Existing BlueWake loop, found during #21 assertion audit | #22 makes the counter per-call and resets after progress; stuck guest still yields on ninth non-advancing successor | Open; Release and ASan/UBSan regression, strict module boot and all 38 native Windows checks pass; optimized/performance/gameplay acceptance open |

Original 30 Hz game logic, Smooth Motion Off and experimental 60 Hz simulation
Off remain defaults. Explicit preferences are preserved. New module transforms
remain independent opt-ins. Displayed FPS is not simulation speed. Experimental
60 Hz simulation, Linux, broader discs and wider decompilation are separate
scope unless required by the agreed current parity target.

## Current acceptance evidence and limits

**Trace coverage correction:** the legacy 600 `[guest-state]` records all occur
before retrace 8, ending at cycle 60,001,200. They contain selected startup fields,
not the full CPU or RAM, and do not cover the full 6,000-retrace route. Earlier
wording saying "600 exact guest states" overstated this evidence; this correction
also applies to the historical appendix. Route/card, independently checked timing,
function-fixture and image results retain their stated scopes. The 30,000-case
function fixtures do compare full CPU/RAM within those cases. Broader optional
checkpoint instrumentation and its local fixtures pass; six headless checkpoints
now match across the 6,000-retrace Off/direct pair. This is sampled coverage, not
a claim of identical state at every intervening instruction. The existing route comparator remains unchanged.

- **Host/runtime regressions:** all 248 Mac tests pass at `7b7e530` (67.65 s).
  Eight older assertion-based fixtures were previously disabled by Release's
  `NDEBUG`; earlier 247-test counts include those ineffective passes. The repaired
  fixtures are active, and five portable ones now run on Windows. At `9303569`,
  Windows builds the app and passes all 38 tests (35.49 s;
  [run 36970389671](https://github.com/chrissotraidis/bluewake/actions/runs/36970389671)).
- **Prepared blocks:** the donor-style broad transform fails the controlled
  route despite passing the function fixture; retaining PC stores alone also
  fails (22/600 selected startup records differ). The conservative successor preserves
  PC/suffix observations and leaves refund/unknown prepaid-state blocks alone.
  It passes 30,000 function cases and 6,000 retraces: 1,050 canonical/card records,
  600 matching selected startup records and delivery/clock timing match. This is bounded O0
  correctness, not full donor optimization/performance parity. Matched O2 builds
  remain running; preserve the failed candidates and unchanged comparator.
- **Fixed CPU and module RAM:** each independent O0 candidate passes the same
  30,000 function cases and strict 6,000-retrace pair. The RAM fixture also checks
  assigned pointer identity; plain/global alias, reservation, journal, MMIO,
  endian and bounds checks pass sanitizers. Full storage ABI adoption/cleanup and
  actual old-host rejection pass. Whole-module optimized x86 and gameplay remain open.
- **Inline FP:** the strengthened fixture exposed 9,526 native Windows division
  differences. A zero/subnormal divisor guard fixes the host-DAZ case; deterministic
  tests fail before and pass after. The corrected helper passes 38 million operation
  comparisons on arm64, x86-64/Rosetta and native Windows. Corrected arm64 module
  `9d3729a` passes 30,000 function cases and the strict 6,000-retrace pair with
  1,050 matching records/cards, 600 matching selected startup records and zero schedule drift.
  The host is `3222b915`, both modules use runtime `c2905b7a`. Optimized/performance
  and gameplay acceptance remains open.
- **Restored observation:** #21's old/new-host pair uses the same ordinary module.
  Exactly one canonical record changes: GroundCross visits rise from 0 to 1,974,
  with valid and sentinel values observed. The strict comparator correctly rejects
  equality; preserve that result. All other 1,049 records, cards, 600 selected startup records,
  1,024 delivery cycles and route clock match. The explicit intended diagnostic
  difference is recorded without weakening the comparator. This is bounded
  headless evidence, not gameplay acceptance.
- **Counter repair:** #22's new test fails on the old loop and passes through both
  entry points after the repair. It checks alternating progress, interrupted runs,
  independent turns/CPUs and bounded stuck execution. The candidate module links;
  all 819 other object hashes match the baseline, with only module-export/dispatch
  objects changed. The strict 6,000-retrace pair passes with 1,050 matching
  records/cards, 600 matching selected startup records and zero scheduling drift. Native Windows
  passes all 38 tests (31.87 s;
  [run 36970729454](https://github.com/chrissotraidis/bluewake/actions/runs/36970729454)).
  The incremental build is not clean player-build evidence.
- **Inline-memory module:** #24 at `608edaa` builds all 823 O0 units and passes
  30,000 strict function cases against #22, including 264 partial returns and
  12,539 deadline cases. The same `7b7e530` host / `c2905b7a` runtime passes the
  6,000-retrace pair: 1,050 canonical/card records, 600 matching selected startup records and zero
  delivery/clock drift. The optional host FIFO writer is **disabled** in both
  runs. This does not qualify GPU batching, optimized performance, gameplay or
  a clean player build. Native Windows at `d0859b3` passes all 41 tests
  (40.03 s; [run 36974344310](https://github.com/chrissotraidis/bluewake/actions/runs/36974344310)).
- **Host writer/batching:** #25 at `80eb353` passes all 42 native Windows tests
  (43.99 s; [run 36975727076](https://github.com/chrissotraidis/bluewake/actions/runs/36975727076)).
  Same-host/module Mac O0 pairs compare writer Off/direct headless and Off/batch
  with Aurora over 6,000 retraces. Each passes 1,050 canonical/card records,
  600 matching selected startup records and zero delivery/clock drift. The
  renderer pair also has two identical nonblank 1920x1440 intro frames at
  retraces 2,000/4,000. An older module without writer exports stays Off when
  batching is requested. Both 23,000-retrace Aurora runs complete normally with
  matching route/card records and zero delivery/clock drift, but neither reaches
  the player-control trigger or captures player frames. The private driver's
  incorrect expectation of 2,300 legacy trace records fails; its original failure
  is retained. Empty capture sets do not establish image agreement. Player-control,
  sustained gameplay and performance acceptance remain open.
- **Checkpoint coverage:** optional `BLUEWAKE_GUEST_CHECKPOINT_INTERVAL=N`
  hashes normalized CPU fields, full MEM1/MEM2 and registered REL storage at each
  Nth retrace. Native addresses and padding are excluded; pointer presence is
  retained. The C fixture passes Release and arm64 ASan/UBSan. The separate
  `scripts/guest_checkpoints.py --interval 1000 --through 6000 A.log B.log`
  comparator requires every expected checkpoint, increasing cycles, one normal
  stop and the requested terminal retrace. Its fixtures reject missing, duplicate,
  malformed, truncated and differing evidence. Use it alongside route/card and
  image checks. It is disabled by default, adds measurement overhead when enabled,
  and does not cover GPU/peripheral state or execution between checkpoints.
  At host `9de9cf2` / module `608edaa` / runtime `c2905b7a`, all six checkpoints
  at retraces 1,000 through 6,000 match in the headless Off/direct pair, alongside
  route/card and zero delivery/clock drift. Comparing the new instrumented Off
  run with the previous host's Off run also preserves route/card/timing results.
  The corresponding Aurora Off/batch pair also matches all six checkpoints,
  route/card/timing and both nonblank intro frames. Native Windows at `91e0417`
  builds the host successfully (run 36980025091).
- **Menu persistence:** Mac `3222b915` / `c2905b7a`, isolated ordinary-module session:
  fresh Off, select 120, disable while retaining three steps, close/relaunch and
  visibly remain Off with byte-identical settings. Keyboard selection passes;
  automated pointer selection was unreliable. Pacing, display hot-plug, controller
  navigation, sustained play and audible output remain open.

A separate donor gather-wrapper probe overreads an undersized RAM buffer under
ASan; the maintained ordinary memory path does not. A second regression shows
the donor pipe shortcut bypassing registered guest aliases. The imported helper
foundation guards the subtraction and checks normal memory precedence before
using the writer. Native Windows at `ab8ff52` builds the app and passes all 41
tests (43.21 s; [run 36972986519](https://github.com/chrissotraidis/bluewake/actions/runs/36972986519)). Shared maintained memory contracts exercise ordinary/global
RAM, aliases, reservation, journal, endian and MMIO behavior. Additional tests
cover byte order, batching thresholds, writer changes, interpreter/MMIO flushes
and inline/exported dispatch boundaries. Sustained gameplay and optimized performance
qualification remain open. Explicit module preparation is now available independently of inline FP;
the host has explicit direct/batch opt-ins, with ordinary MMIO retained by
default and for diagnostics. The bounded host/renderer results above are now
available; direct-call/native batches remain open work.
All measured wall times under concurrent compilation are excluded from performance
acceptance. No donor benchmark is transferred.

## Player build and platform gates

| Route / platform | Current evidence | Still required |
| --- | --- | --- |
| Owned-disc source / iOS | Fresh `95adeed` / runtime `99e47480`: disc validation, translation, mods, locally generated PGO, full module/app link, personal packaging and signature/provenance checks pass. Controlled interruption/resume preserves prior objects/profile hashes | Repeat final integrated-source selection where changed; physical launch, fresh/existing save/reload and in-place upgrade preserving data |
| PadMint / iOS | Adapter `018a9f0` (CLI 0.2.8), new workspace, exact `95adeed`, matching audited app-only input: translation/mods, new 23,000-retrace local training, full compilation and personal assembly pass; package integrity, module hash and exact source provenance rechecked | Player signing/install (assembled module is unsigned), final integrated-source acceptance, device run/save/reload/upgrade and trained performance |
| App-only / iOS | Fresh matching candidate excludes translated code; PadMint audit and public-assets gate pass locally | Final-source compatible old-module update and device acceptance; no upload/publication |
| Owned-disc / Mac | Bundled development host, private module correctness and isolated menu checks pass | Complete documented fresh player route, installation/launcher, actual game/save/reload/upgrade and sustained performance |
| Owned-disc / Windows | Native app/source fixtures pass; disc/recovery and preparation scripts integrated | Full clean module build/training, real disc/Unicode/UI/recovery, Direct3D/fullscreen/restart, controllers/audio/gameplay and save-preserving upgrade |
| PadMint / Mac, Windows, tvOS | No complete adapter path accepted; Mac remains planned | Do not claim support; implement/test before widening the matrix |
| Apple TV | Existing preview source and contributor work preserved | Physical hardware/storage acceptance for any claimed feature; no new acceptance inferred |

The baseline personal iOS build cannot establish acceptance for every later
integrated change. Existing artifacts and private profiles are not prerequisites
for the clean player route. Exact hashes and private evidence stay local.

## Report reconciliation

Reports from both repositories are investigation leads, not diagnosed or resolved
bugs. No issue is closed or externally commented on by this work.

| Report | Evidence / disposition | Next discriminating check |
| --- | --- | --- |
| [BlueWake #13: Pictobox](https://github.com/chrissotraidis/bluewake/issues/13) | Windfall shutter closes, picture stops, sound continues; reports Windows 11/120 FPS but exact product/module unclear. Maintainer already asked for identity | Establish exact build; isolated Windfall save, interpolation off/on, capture GPU/EFB copy and game-thread progress. No reproduced cause |
| [Donor #16: startup freeze](https://github.com/elliotttate/Wind-Waker-Recomp/issues/16) | 0.2.2 Windows logs reject PAL, then accept USA, initialize D3D12 on RTX 2050; stop after gxcore initialization. Also occurs on integrated GPU | Symbolized/blocked-thread evidence at first-frame boundary with exact app/module/pins; avoid assuming disc/settings/GPU cause |
| [Donor #5: settings/startup crash](https://github.com/elliotttate/Wind-Waker-Recomp/issues/5) | Earlier access violation at zero; #12 startup/restart safeguards do not establish a fix | Windows HLE/LLE and real Restart/settings reproduction; bounded crash/safe-mode recovery |
| [Donor #12: scripted music](https://github.com/elliotttate/Wind-Waker-Recomp/issues/12), [#1: intro](https://github.com/elliotttate/Wind-Waker-Recomp/issues/1) | #12 identifies Windows x86/0.2.2, ambient-only intro and bird/Zelda/sister/Ganon cues; Mac intro stream/sink evidence covers only its stated scene/build | Fresh-card and matching scripted-scene Windows guest/sink/output comparison, then audible output; do not extrapolate Mac intro captures |
| [Donor #6: bird slowdown](https://github.com/elliotttate/Wind-Waker-Recomp/issues/6) | Scene report; device/settings and matched baseline still needed | Matched quiet-device bird/Outset/busy-area serial measurements, frame-time tails, stalls/audio and 30-minute run |
| [Donor #2](https://github.com/elliotttate/Wind-Waker-Recomp/issues/2), [#8](https://github.com/elliotttate/Wind-Waker-Recomp/issues/8), [#14](https://github.com/elliotttate/Wind-Waker-Recomp/issues/14): controllers | Layout swaps cover part of request; unsupported 8BitDo mapping and arbitrary remap are not accepted | Real controllers, per-device layout/hot-plug/navigation/save preference checks |
| Donor #3/#18 Linux, #7 PAL, #4 HD HUD, #9 Switch, #10/#11 portable | Follow-up platform/UI requests; explicit data-directory override exists in Windows source | Record supported matrix; preserve unsupported-disc rejection, do not broaden claims |


## Next actions and retained work

1. Finish the running prepared-block O2 pair and direct-call candidate comparisons,
   then verify their terminal artifacts. The short Aurora checkpoint comparison passes. The baseline full PadMint package is complete;
   player signing and final-source/device acceptance remain open. Complete broader guest
   checkpoints and correct startup-only trace claims throughout the current evidence. Run matched performance measurements on a quiet
   machine; do not derive performance from current loaded runs.
2. Continue directly importing the remaining Elliott direct-call/register and
   provenance-reviewed native enhancements, adapting BlueWake hooks and retaining
   contributor credit. Finish host FIFO batching qualification alongside them. Correctness,
   scheduling/hooks and matched measurements are separate gates.
3. Complete final-source clean player routes and the full gameplay matrix:
   fresh/existing saves, narrated intro, progression, scripted music/Pictobox,
   fullscreen/restart, real controllers, save/reload, upgrades and sustained play.
   Windows gameplay hardware and current physical iPad acceptance remain
   unconfirmed; continue independent work without counting those checks passed.
4. Reconcile the draft source stack into the intended maintained branch only with
   its acceptance evidence; finish unified instructions, credits and issue mapping.
   The [proposed migration notice](FORK_RECONCILIATION_EVIDENCE_2026-10-02.md#migration-proposal-not-published-to-the-donor)
   remains unposted. Keep the donor and its history intact until acceptance.

Primary `codex/fork-reconciliation` at `95adeed` stays fixed for PadMint. The
managed `bluewake-disc-parity` worktree at `05df605` stays fixed for the O2 pair.
The managed `bluewake-cpu-contract` worktree holds the current source stack and
private qualification artifacts; its nested runtime is `c2905b7a`. Both managed
worktrees contain needed unique artifacts and remain in use. No checkout is
removed or reset. Preserve saves, settings, inputs and signing material.
